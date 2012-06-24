/* CalSol - UC Berkeley Solar Vehicle Team
 * bms.pde - BMS Module
 * Purpose: Main code for the BMS module
 * Author(s): Steven Rhodes.
 * Date: May 1 2012
 */

//#define ENABLE_BALANCING
#define CRITICAL_MESSAGES
// #define CAN_DEBUG
// #define SPI_DEBUG
// #define VERBOSE
// #define FLAGS

#include <SPI.h>
#include <EEPROM.h>
#include "bms.h"

Song buzzer(OUT_BUZZER);

CarDataInt car_data;
Flags flags;
CarState car_state = CAR_OFF;

// If left on for 50 days, time will overflow and we will have undefined behavior.
long data_time;
long heartbeat_time;
long battery_time;
long song_time;

// Hackish, but I need to maintain a counter for this somewhere.  Used in loop().
byte lt_counter = 0;

// Only used to average out readings from LT boards
LTMultipleData averaging_data[NUM_OF_LT_BOARDS];

void setup() {
  Serial.begin(115200);
  InitPins();
  InitSpi();
  InitLtBoardData();
  InitEEPROM();
  InitCan();
  buzzer.PlaySong(kStartupBeep);
}

void loop() {
  long time = millis();

  if (time - data_time > DATA_TIME_LENGTH) {
    GetCarData(&car_data);
    GetFlags(&flags, &car_data);
    car_state = GetCarState(car_state, &flags);
    #ifdef CRITICAL_MESSAGES
      Serial.print("Car State: ");
      Serial.println(car_state);
    #endif
    switch(car_state) {
      case TURN_OFF:
        ShutdownCar(&flags);
        buzzer.PlaySong(kShutdownBeep);
        break;
      case TURN_ON:
        TurnOnCar();
        buzzer.PlaySong(kFullyBootedBeep);
        break;
      case DISABLE_CHARGING:
        flags.charging_disabled_too_hot = flags.too_hot_to_charge;
        flags.charging_disabled_too_full = flags.too_full_to_charge;
        DisableCharging();
        break;
      case ENABLE_CHARGING:
        flags.charging_disabled_too_hot = flags.too_hot_to_charge;
        flags.charging_disabled_too_full = flags.too_full_to_charge;
        break;
      case IN_PRECHARGE:
      case CAR_ON:
      case CAR_OFF:
      case EMERGENCY_SHUTOFF:
      default:
        break;
    }
    if (!buzzer.IsPlaying()) {
      if (flags.module_overvoltage_warning) {
        buzzer.PlaySong(kHighVoltageBeep);
      } else if (flags.module_undervoltage_warning) {
        buzzer.PlaySong(kLowVoltageBeep);
      } else if (flags.battery_overtemperature_warning) {
        buzzer.PlaySong(kHighTemperatureBeep);
      } else if (flags.missing_lt_communication) {
        buzzer.PlaySong(kMissingLtCommunicationBeep);
      }
    }
#ifdef ENABLE_BALANCING
    BalanceBatteries(car_data.lt_board);
#endif
    CarDataFloat float_data;
    ConvertCarData(&float_data, &car_data);
    SendGeneralDataCanMessage(&float_data);
    SendErrorCanMessage(car_state, &flags);
    data_time = time;
  }

  if (time - battery_time > BATTERY_TIME_LENGTH) {
    // We send out LT CAN messages one at a time to avoid overloading CAN
    if (car_data.lt_board[lt_counter].is_valid) {
      SendLtBoardCanMessage(car_data.lt_board + lt_counter, lt_counter);
    } else {
      #ifdef CAN_DEBUG
        Serial.print("CAN: no LT Board ");
        Serial.print(lt_counter, DEC);
        Serial.println(" comm, no msg sent");
      #endif
    }
    lt_counter = (lt_counter + 1) % NUM_OF_LT_BOARDS;
    battery_time = time;
  }

  if (time - song_time > SONG_TIME_LENGTH) {
    // makes the buzzer keep playing if it's already playing something
    buzzer.KeepPlaying();
    song_time = time;
  }

  if (time - heartbeat_time > HEARTBEAT_TIME_LENGTH) {
    #ifdef CAN_DEBUG
      Serial.print("CAN: RX=");
      Serial.print(Can.rxError());
      Serial.print(", TX=");
      Serial.println(Can.txError());
    #endif
    SendHeartbeat();
    heartbeat_time = time;
  }

  // TODO(stvn): Add ability to recieve messages over CAN
}

void InitEEPROM(void) {
  byte memory_index = EEPROM.read(0);
  if (memory_index > NUM_OF_ERROR_MESSAGES) {
    memory_index = 1;
  }
  #ifdef CRITICAL_MESSAGES
    Serial.print("Last shutdown: ");
    PrintErrorMessage((enum error_codes) EEPROM.read(memory_index));
  #endif
  #ifdef VERBOSE
    Serial.println("Earlier shutdown messages:");
    for (int i = memory_index - 1; i != memory_index; --i) {
      if (i == 0) {
        i = NUM_OF_ERROR_MESSAGES;
      }
      Serial.print(i);
      Serial.print(": ");
      PrintErrorMessage((enum error_codes) EEPROM.read(memory_index));
    }
  #endif
  memory_index = (memory_index % NUM_OF_ERROR_MESSAGES) + 1;
  EEPROM.write(0, memory_index);
  EEPROM.write(memory_index, POWER_LOSS);
}

void InitPins(void) {
  pinMode(OUT_BUZZER, OUTPUT);
  pinMode(BATTERY_RELAY, OUTPUT);
  pinMode(SOLAR_RELAY, OUTPUT);
  pinMode(LV_RELAY, OUTPUT);
  pinMode(LEDFAIL, OUTPUT);
  pinMode(LT_CS, OUTPUT);
  pinMode(FAN1, OUTPUT);
  pinMode(FAN2, OUTPUT);
  pinMode(KEY_SWITCH, INPUT);
  pinMode(V_BATTERY, INPUT);
  pinMode(V_MOTOR, INPUT);
  pinMode(C_BATTERY, INPUT);
  pinMode(C_SOLAR, INPUT);
  pinMode(C_GND, INPUT);

  digitalWrite(OUT_BUZZER, LOW);
  digitalWrite(BATTERY_RELAY, LOW);
  digitalWrite(SOLAR_RELAY, LOW);
  digitalWrite(LV_RELAY, LOW);
  digitalWrite(LEDFAIL, LOW);
  digitalWrite(LT_CS, HIGH);
  digitalWrite(FAN1, LOW);
  digitalWrite(FAN2, LOW);
  digitalWrite(KEY_SWITCH, HIGH);
}

void InitSpi(void) {
  #ifdef SPI_DEBUG
    Serial.println("SPI: Initializing");
  #endif
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
}

void InitLtBoardData(void) {
  for (byte i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    lt_config[i][0] = 0xE5;
    lt_config[i][1] = 0x00;
    lt_config[i][2] = 0x00;
    lt_config[i][3] = 0x00;
    lt_config[i][4] = LT_UNDER_VOLTAGE;
    lt_config[i][5] = LT_OVER_VOLTAGE;
    LTMultipleData *lt_avg = averaging_data + i;
    for (byte j = 0; j < NUM_OF_AVERAGES; ++j) {
      LTData *lt_board = lt_avg->data + j;
      for (byte k = 0; k < NUM_OF_VOLTAGES; ++k) {
        lt_board->voltage[k] = 0x00;
      }
      for (byte k = 0; k < NUM_OF_TEMPERATURES; ++k) {
        lt_board->temperature[k] = 0x00;
      }
      ++j;
      if (j < NUM_OF_AVERAGES) {
        lt_board = lt_avg->data + j;
        for (byte k = 0; k < NUM_OF_VOLTAGES; ++k) {
          lt_board->voltage[k] = 0x7fff;
        }
        for (byte k = 0; k < NUM_OF_TEMPERATURES; ++k) {
          lt_board->temperature[k] = 0x7fff;
        }
      }
    }
    lt_avg->ptr = 0;
  }
}

void InitCan(void) {
  Can.reset();
  Can.filterOn();
  Can.setFilter(1, 0x020, 1);
  Can.setFilter(1, 0x040, 2);
  Can.setMask(1, 0x7F0);
  Can.setMask(2, 0x000);
  Can.begin(1000, false);
}

void SendHeartbeat(void){
  #ifdef CAN_DEBUG
    Serial.println("CAN: sent Cutoff, BPS Heartbeat");
  #endif
  char data = 0;
  Can.send(CanMessage(CAN_HEART_CUTOFF, &data, 1));
  Can.send(CanMessage(CAN_HEART_BPS, &data, 1));
}

void GetCarData(CarDataInt *car_data) {
  byte config;
  
  SpiStart();
  config = STCVAD;
  SendToSpi(&config, 1);
  delay(15); //Time for conversions, approx 12ms
  SpiEnd();
  
  SpiStart();
  config = STTMPAD;
  SendToSpi(&config, 1);
  delay(15); //Time for conversions, approx 12ms
  SpiEnd();
  
  for (byte i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    GetLtBoardData(car_data->lt_board + i, i, averaging_data + i);
  }
  car_data->battery_voltage = GetBatteryVoltage();
  car_data->motor_voltage = GetMotorVoltage();
  car_data->battery_current = GetBatteryCurrent();
  car_data->solar_current = GetSolarCellCurrent();
}

void GetFlags(Flags *flags, const CarDataInt *car_data) {
  flags->battery_overvoltage = car_data->battery_voltage > OVERVOLTAGE_CUTOFF;
  flags->battery_overvoltage_warning = car_data->battery_voltage > OVERVOLTAGE_WARNING;
  flags->battery_undervoltage = car_data->battery_voltage < UNDERVOLTAGE_CUTOFF;
  flags->battery_undervoltage_warning = car_data->battery_voltage < UNDERVOLTAGE_WARNING;

  // TODO:(stvn): Test assumption that negative current == charging
  flags->discharging_overcurrent = car_data->battery_current > DISCHARGING_OVERCURRENT_CUTOFF;
  flags->charging_overcurrent = car_data->battery_current < CHARGING_OVERCURRENT_CUTOFF;
  flags->batteries_charging = car_data->battery_current < CHARGING_THRESHOLD;
  
  flags->motor_precharged = (car_data->motor_voltage > MOTOR_MINIMUM_VOLTAGE) &&
      (car_data->battery_voltage - car_data->motor_voltage < MOTOR_MAXIMUM_DELTA);
      
  flags->missing_lt_communication = false;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    if (!(car_data->lt_board[i].is_valid)) {
      flags->missing_lt_communication = true;
    }
  }

  if (!flags->missing_lt_communication) {
    int high_voltage = HighestVoltage(car_data->lt_board);
    flags->module_overvoltage = high_voltage > MODULE_OVERVOLTAGE_CUTOFF;
    flags->module_overvoltage_warning = high_voltage > MODULE_OVERVOLTAGE_WARNING;
    int low_voltage = LowestVoltage(car_data->lt_board);
    flags->module_undervoltage = low_voltage < MODULE_UNDERVOLTAGE_CUTOFF;
    flags->module_undervoltage_warning = low_voltage < MODULE_UNDERVOLTAGE_WARNING;
    float high_temperature = HighestTemperature(car_data->lt_board);
    flags->battery_overtemperature = high_temperature > OVERTEMP_CUTOFF;
    flags->battery_overtemperature_warning = high_temperature > OVERTEMP_WARNING;
    flags->charging_overtemperature = high_temperature > CHARGING_OVERTEMP_CUTOFF;
    flags->charging_temperature_warning = high_temperature > CHARGING_OVERTEMP_WARNING;

    if (flags->charging_disabled_too_hot && high_temperature < TEMPERATURE_OK_TO_CHARGE) {
      flags->too_hot_to_charge = false;
    } else if (!flags->charging_disabled_too_hot && high_temperature > OVERTEMPERATURE_NO_CHARGE) {
      flags->too_hot_to_charge = true;
    }

    if (flags->charging_disabled_too_full && high_voltage < VOLTAGE_OK_TO_CHARGE) {
      flags->too_full_to_charge = false;
    } else if (!flags->charging_disabled_too_full && high_voltage > OVERVOLTAGE_NO_CHARGE) {
      flags->too_full_to_charge = true;
    }
  }

  flags->keyswitch_on = IsKeyswitchOn();


  #ifdef FLAGS
    Serial.print(flags->battery_overvoltage, DEC);
    Serial.print(flags->battery_overvoltage_warning, DEC);
    Serial.print(flags->battery_undervoltage, DEC);
    Serial.print(flags->battery_undervoltage_warning, DEC);
    Serial.print(flags->discharging_overcurrent, DEC);
    Serial.print(flags->charging_overcurrent, DEC);
    Serial.print(flags->batteries_charging, DEC);
    Serial.print(flags->motor_precharged, DEC);
    Serial.print(flags->missing_lt_communication, DEC);
    Serial.print(flags->module_overvoltage, DEC);
    Serial.print(flags->module_overvoltage_warning, DEC);
    Serial.print(flags->module_undervoltage, DEC);
    Serial.print(flags->module_undervoltage_warning, DEC);
    Serial.print(flags->battery_overtemperature, DEC);
    Serial.print(flags->battery_overtemperature_warning, DEC);
    Serial.print(flags->charging_overtemperature, DEC);
    Serial.print(flags->charging_temperature_warning, DEC);
    Serial.print(flags->charging_disabled_too_hot, DEC);
    Serial.print(flags->charging_disabled_too_full, DEC);
    Serial.print(flags->too_full_to_charge, DEC);
    Serial.println(flags->keyswitch_on, DEC);
  #endif
}

CarState GetCarState(const CarState old_state, const Flags *flags) {
  // stvn: See state diagram to understand what this is doing
  switch(old_state) {
 
    case TURN_OFF:
      #ifdef CRITICAL_MESSAGES
        Serial.println("Car is shut down");
      #endif
      return CAR_OFF;
 
    case TURN_ON:
      if (IsCriticalError(flags)) {
        return EMERGENCY_SHUTOFF;
      }
      #ifdef CRITICAL_MESSAGES
        Serial.println("Car is on");
      #endif
      return CAR_ON;
 
    case DISABLE_CHARGING:
      if (IsCriticalError(flags)) {
        return EMERGENCY_SHUTOFF;
      }
      return CAR_ON;      

    case ENABLE_CHARGING:
      if (IsCriticalError(flags)) {
        return EMERGENCY_SHUTOFF;
      }
      return CAR_ON;      

    case CAR_ON:
      if (IsCriticalError(flags)) {
        return EMERGENCY_SHUTOFF;
      }
      if ((flags->too_hot_to_charge && !flags->charging_disabled_too_hot) ||
          (flags->too_full_to_charge && !flags->charging_disabled_too_full)) {
        #ifdef CRITICAL_MESSAGES
          PrintErrorMessage(S_DISABLE_CHARGING);
        #endif
        return DISABLE_CHARGING;
      }
      if ((!flags->too_hot_to_charge && !flags->too_full_to_charge) &&
          (flags->charging_disabled_too_hot || flags->charging_disabled_too_full)) {
        #ifdef CRITICAL_MESSAGES
          PrintErrorMessage(S_ENABLE_CHARGING);
        #endif
        return ENABLE_CHARGING;
      }
      if (!flags->keyswitch_on) {
        return TURN_OFF;
      }
      return CAR_ON;

    case EMERGENCY_SHUTOFF:
      if (!flags->keyswitch_on) {
        return CAR_OFF;
      }
      return EMERGENCY_SHUTOFF;

    case IN_PRECHARGE:
      if (IsCriticalError(flags)) {
        return EMERGENCY_SHUTOFF;
      }
      if (!flags->keyswitch_on) {
        return TURN_OFF;
      }
      if (flags->motor_precharged) {
        #ifdef CRITICAL_MESSAGES
          PrintErrorMessage(S_COMPLETED_PRECHARGE);
        #endif
        return TURN_ON;
      }
      return IN_PRECHARGE;
      
    case CAR_OFF:
    default:
      if (flags->keyswitch_on) {
        return IN_PRECHARGE;
      }
      return CAR_OFF;
  }
}

int IsCriticalError(const Flags *flags) {
  if (flags->missing_lt_communication) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(BPS_DISCONNECTED);
    #endif
    EEPROM.write(EEPROM.read(0), BPS_DISCONNECTED);
    return true;
  }
  // TODO(stvn): Calibrate current, then replace this with commented out line.
  if (flags->module_overvoltage) {
  // stvn: We possibly want to do this if we want to be able to turn on the car and it's too high
  // if (flags->module_overvoltage && flags->batteries_charging) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(BPS_OVERVOLT);
    #endif
    EEPROM.write(EEPROM.read(0), BPS_OVERVOLT);
    return true;
  }
  if (flags->module_undervoltage) {
  // if (flags->module_undervoltage && !flags->motor_precharged) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(BPS_UNDERVOLT);
    #endif
    EEPROM.write(EEPROM.read(0), BPS_UNDERVOLT);
    return true;
  }
  if (flags->discharging_overcurrent) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(S_OVERCURRENT);
    #endif
    EEPROM.write(EEPROM.read(0), S_OVERCURRENT);
    return true;

  }
  if (flags->charging_overcurrent) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(S_OVERCURRENT);
    #endif
    EEPROM.write(EEPROM.read(0), S_OVERCURRENT);
    return true;
  }
  if (flags->batteries_charging && flags->charging_overtemperature) {
    #ifdef CRITICAL_MESSAGES
      PrintErrorMessage(BPS_OVERTEMP);
    #endif
    EEPROM.write(EEPROM.read(0), BPS_OVERTEMP);
    return 1;
  }
  return false;
}

void ConvertCarData(CarDataFloat *out, const CarDataInt *in) {
  // Everything is in 10mV or 10mA, so we divide by 100 to scale
  out->battery_voltage = in->battery_voltage / 100.0;
  out->motor_voltage = in->motor_voltage / 100.0;
  out->battery_current = in->battery_current / 100.0;
  out->solar_current = in->solar_current / 100.0;
}

void DisableCharging (void) {
#ifdef CAN_DEBUG
  Serial.print("CAN: Not yet implemented, ");
  Serial.println("CAN: sent Prevent Regen Msg");
#endif
  // TODO(stvn): Send CAN message to prevent regen
  digitalWrite(C_SOLAR, LOW);
}

void EnableCharging (void) {
#ifdef CAN_DEBUG
  Serial.print("Not yet implemented, ");
  Serial.println("CAN: sent Allow Regen Msg");
#endif
  // TODO(stvn): Send CAN message to allow regen
  digitalWrite(C_SOLAR, HIGH);
}

signed int GetBatteryVoltage(void) {
  static signed int old_readings[] = {
    0xffff, 0x7fff, 0xffff  };
  static byte ptr = 0;

  // We need to make this a long to avoid rounding errors when we scale
  long reading = analogRead(V_BATTERY);
  old_readings[ptr] = CONVERT_TO_MILLIVOLTS(reading);
  ptr = (ptr + 1) % 3;
  #ifdef VERBOSE
    Serial.print("Battery Voltage: ");
    Serial.println(GetIntMedian(old_readings));
  #endif
  return GetIntMedian(old_readings);
}

signed int GetMotorVoltage(void) {
  static signed int old_readings[] = {
      0xffff, 0x7fff, 0xffff  };
  static byte ptr = 0;

  long reading = analogRead(V_MOTOR);
  old_readings[ptr] = CONVERT_TO_MILLIVOLTS(reading);
  ptr = (ptr + 1) % 3;
  #ifdef VERBOSE
    Serial.print("Motor Voltage: ");
    Serial.println(GetIntMedian(old_readings));
  #endif
  return GetIntMedian(old_readings);
}

signed int GetBatteryCurrent(void) {
  static signed int old_readings[] = {
      0xffff, 0x7fff, 0xffff  };
  static byte ptr = 0;

  signed long reading = analogRead(C_BATTERY) - analogRead(C_GND);
  old_readings[ptr] = CONVERT_TO_MILLIVOLTS(reading);
  ptr = (ptr + 1) % 3;
  #ifdef VERBOSE
    Serial.print("Battery Current: ");
    Serial.println(GetIntMedian(old_readings));
  #endif
  return GetIntMedian(old_readings);
}

signed int GetSolarCellCurrent(void) {
  static signed int old_readings[] = {
      0xffff, 0x7fff, 0xffff  };
  static byte ptr = 0;

  signed long reading = analogRead(C_BATTERY) - analogRead(C_GND);
  old_readings[ptr] = CONVERT_TO_MILLIAMPS(reading);
  ptr = (ptr + 1) % 3;
  #ifdef VERBOSE
    Serial.print("Solar Current: ");
    Serial.println(GetIntMedian(old_readings));
  #endif
  return GetIntMedian(old_readings);
}

byte IsKeyswitchOn(void) {
  static byte state = false;
  static byte prev_reading = HIGH;
  byte current_reading = digitalRead(KEY_SWITCH);
  if (current_reading == prev_reading) {
    // Only update the state if we have two identical readings in a row
    current_reading == HIGH ? state = false : state = true;
  }
  prev_reading = current_reading;
  return state;
}

signed int GetIntMedian(const signed int * array) {
  signed int high = array[0];
  signed int low  = array[1];
  if (high < low) {
    high = array[1];
    low = array[0];
  }
  if (low < array[2]) {
    if (high > array[2]) {
      return array[2];
    }
    return high;
  }
  return low;
}

void GetLtDataMedian(LTData *median, const LTMultipleData * array) {
  int avgs[NUM_OF_AVERAGES];
  for (int i = 0; i < NUM_OF_VOLTAGES; ++i) {
    for (int j = 0; j < NUM_OF_AVERAGES; ++j) {
      avgs[j] = array->data[j].voltage[i];
    }
    median->voltage[i] = GetIntMedian(avgs);
  }
  for (int i = 0; i < NUM_OF_TEMPERATURES; ++i) {
    for (int j = 0; j < NUM_OF_AVERAGES; ++j) {
      avgs[j] = array->data[j].temperature[i];
    }
    median->temperature[i] = GetIntMedian(avgs);
  }
}

void AddReading(LTMultipleData * mult, const LTData * reading) {
  mult->data[mult->ptr] = *reading;
  mult->ptr = (mult->ptr + 1) % NUM_OF_AVERAGES;
}

void GetLtBoardData(LTData *new_data, byte board_num, LTMultipleData * avg_data) {
  byte config;

  SpiStart();
  SendToSpi(kBoardAddress + board_num, 1);
  config = WRCFG;
  SendToSpi(&config, 1);
  SendToSpi(lt_config[board_num], 6);
  SpiEnd();

  SpiStart();
  SendToSpi(kBoardAddress + board_num, 1);
  config = RDCFG;
  SendToSpi(&config, 1);
  byte rconfig[6];
  GetFromSpi(rconfig, RDCFG, 6);
  SpiEnd();
#ifdef VERBOSE
  Serial.print("Sending config: ");
  for(int i = 0 ;i< 6;++i){
    Serial.print(lt_config[board_num][i], HEX);
  }
  Serial.println();
  Serial.print("Got config: ");
  for(int i = 0 ;i< 6;++i){
    Serial.print(rconfig[i], HEX);
  }
  Serial.println();
#endif

  if (rconfig[0] == 0xFF || rconfig[0] == 0x00 || rconfig[0] == 0x02) {
    //  If we get one of these responses, it means the LT board is not communicating.
    new_data->is_valid = false;
    #ifdef CRITICAL_MESSAGES
      Serial.print("LT Board ");
      Serial.print(board_num, DEC);
      Serial.println(" not responding");
    #endif
    return;
  }

  #ifdef VERBOSE
    Serial.print("Board ");
    Serial.print(board_num, DEC);
    Serial.print(" has config ");
    Serial.print(rconfig[1], HEX);
    Serial.println(rconfig[2], HEX);
  #endif

  SpiStart();
  SendToSpi(kBoardAddress + board_num, 1);
  config = RDCV;
  SendToSpi(&config, 1);
  byte voltages[18];
  GetFromSpi(voltages, RDCV, 18);
  SpiEnd();

  SpiStart();
  SendToSpi(kBoardAddress + board_num, 1);
  config = RDTMP;
  SendToSpi(&config, 1);
  byte temperatures[5];
  GetFromSpi(temperatures, RDTMP, 5);
  SpiEnd();

  ParseSpiData(new_data, voltages, temperatures);
  AddReading(avg_data, new_data);
  GetLtDataMedian(new_data, avg_data);
  new_data->is_valid = true;

  #ifdef VERBOSE
    Serial.print("LT Board ");
    Serial.print(board_num, DEC);
    Serial.print(" voltages: ");
    for (int i = 0; i < kLTNumOfCells[board_num]; ++i) {
      Serial.print(new_data->voltage[i]);
      Serial.print("(");
      Serial.print(LT_VOLT_TO_FLOAT * new_data->voltage[i]);
      Serial.print("), ");
    }
    Serial.print("temperatures: ");
    Serial.print(new_data->temperature[0]);
    Serial.print("(");
    Serial.print(ToTemperature(new_data->temperature[0]));
    Serial.print("), ");
    Serial.print(new_data->temperature[1]);
    Serial.print("(");
    Serial.print(ToTemperature(new_data->temperature[1]));
    Serial.print("), ");
    Serial.print(new_data->temperature[2]);
    Serial.print("(");
    Serial.print(CONVERT_THIRD_TO_CELCIUS(new_data->temperature[0]));
    Serial.println(")");
  #endif
}

void SendLtBoardCanMessage(const LTData * data, byte board_num) {
  int board_address = CAN_BPS_BASE + board_num * CAN_BPS_MODULE_OFFSET;
  TwoFloats msg;
  #ifdef CAN_DEBUG
    Serial.print("CAN: sent LT Board ");
    Serial.print(board_num);
    Serial.print(" msg, v: ");
  #endif
  for (int i = 0; i < kLTNumOfCells[board_num]; ++i) {
    msg.f[0] = data->voltage[i] * LT_VOLT_TO_FLOAT;
    #ifdef CAN_DEBUG
      Serial.print(msg.f[0]);
      Serial.print(", ");
    #endif
    Can.send(CanMessage(board_address + i, msg.c, 4));
  }
  #ifdef CAN_DEBUG
    Serial.println();
  #endif
  board_address += CAN_BPS_TEMP_OFFSET;
  msg.f[0] = ToTemperature(data->temperature[0]);
  Can.send(CanMessage(board_address, msg.c, 4));
  msg.f[0] = ToTemperature(data->temperature[1]);
  Can.send(CanMessage(board_address + 1, msg.c, 4));
  msg.f[0] = CONVERT_THIRD_TO_CELCIUS(data->temperature[2]);
  Can.send(CanMessage(board_address + 2, msg.c, 4));
}

void SendGeneralDataCanMessage(const CarDataFloat * data) {
  TwoFloats msg;
  #ifdef CAN_DEBUG
    Serial.print("CAN: sent Data Msg, ");
    Serial.print("battery V: ");
    Serial.print(data->battery_voltage);
    Serial.print(", motor V: ");
    Serial.print(data->motor_voltage);
    Serial.print(", battery C: ");
    Serial.print(data->battery_current);
    Serial.print(", solar C: ");
    Serial.println(data->solar_current);
  #endif
  msg.f[0] = data->battery_voltage;
  msg.f[1] = data->motor_voltage;
  Can.send(CanMessage(CAN_CUTOFF_VOLT, msg.c, 8));
  msg.f[0] = data->battery_current;
  msg.f[1] = data->solar_current;
  Can.send(CanMessage(CAN_CUTOFF_CURR, msg.c,  8));
}  

void SendErrorCanMessage(const CarState car_state, const Flags *flags) {
  char flags_can_data[4] = {0, 0, 0, 0};
  flags_can_data[0] |= flags->battery_overvoltage              << 0;
  flags_can_data[0] |= flags->battery_overvoltage_warning      << 1;
  flags_can_data[0] |= flags->battery_undervoltage             << 2;
  flags_can_data[0] |= flags->battery_undervoltage_warning     << 3;
  flags_can_data[0] |= flags->module_overvoltage               << 4;
  flags_can_data[0] |= flags->module_overvoltage_warning       << 5;
  flags_can_data[0] |= flags->module_undervoltage              << 6;
  flags_can_data[0] |= flags->module_undervoltage_warning      << 7;
  flags_can_data[1] |= flags->battery_overtemperature          << 0;
  flags_can_data[1] |= flags->battery_overtemperature_warning  << 1;
  flags_can_data[1] |= flags->charging_overtemperature         << 2;
  flags_can_data[1] |= flags->charging_temperature_warning     << 3;
  flags_can_data[1] |= flags->discharging_overcurrent          << 4;
  flags_can_data[1] |= flags->charging_overcurrent             << 5;
  flags_can_data[1] |= flags->too_hot_to_charge                << 6;
  flags_can_data[1] |= flags->too_full_to_charge               << 7;
  flags_can_data[2] |= flags->keyswitch_on                     << 0;
  flags_can_data[2] |= flags->batteries_charging               << 1;
  flags_can_data[2] |= flags->missing_lt_communication         << 2;
  flags_can_data[2] |= flags->motor_precharged                 << 3;
  flags_can_data[2] |= flags->charging_disabled_too_hot        << 4;
  flags_can_data[2] |= flags->charging_disabled_too_full       << 5;
  flags_can_data[3] = car_state;
  Can.send(CanMessage(CAN_CUTOFF_NON_CRITICAL_ERROR, flags_can_data, 4));
}

void ShutdownCar(const Flags *flags) {
  digitalWrite(BATTERY_RELAY, LOW);
  digitalWrite(SOLAR_RELAY, LOW);
  digitalWrite(LV_RELAY, LOW);
  digitalWrite(FAN1, LOW);
  digitalWrite(FAN2, LOW);
  digitalWrite(POWER_U3, LOW);
  digitalWrite(POWER_U8, LOW);
  digitalWrite(POWER_U9, LOW);
  digitalWrite(POWER_U10, LOW);
  buzzer.PlaySong(kShutdownBeep);
}

void TurnOnCar(void) {
  digitalWrite(BATTERY_RELAY, HIGH);
  delay(25);  // The delay prevents the current spike from being too large
  digitalWrite(SOLAR_RELAY, HIGH);
  delay(25);
  digitalWrite(LV_RELAY, HIGH);
  digitalWrite(FAN1, HIGH);
  digitalWrite(FAN2, HIGH);
  digitalWrite(POWER_U3, HIGH);
  digitalWrite(POWER_U8, HIGH);
  digitalWrite(POWER_U9, HIGH);
  digitalWrite(POWER_U10, HIGH);
  buzzer.PlaySong(kStartupBeep);
}

void SendToSpi(const byte * data, int length) {
  for (int i = 0; i < length; ++i) {
    SPI.transfer(data[i]);
  }
  #ifdef SPI_DEBUG
    Serial.print("SPI: Sent ");
    for (int i = 0; i < length; ++i) {
      Serial.print(data[i], HEX);
      Serial.print(", ");
    }
    Serial.println("end of message");
  #endif
}

void GetFromSpi(byte * data, byte info, int length) {
  for (int i = 0; i < length; ++i) {
    data[i] = SPI.transfer(info);
  }
  #ifdef SPI_DEBUG
    Serial.print("SPI: With ");
    Serial.print(info, HEX);
    Serial.print(", Got ");
    for (int i = 0; i < length; ++i) {
      Serial.print(data[i], HEX);
      Serial.print(", ");
    }
    Serial.println("end of message");
  #endif
}

void ParseSpiData(LTData *data, const byte voltages[], 
const byte temperatures[]) {
  data->voltage[0] = (voltages[0] & 0xFF) | (voltages[1] & 0x0F) << 8;
  data->voltage[1] = (voltages[1] & 0xF0) >> 4 | (voltages[2] & 0xFF) << 4;
  data->voltage[2] = (voltages[3] & 0xFF) | (voltages[4] & 0x0F) << 8;
  data->voltage[3] = (voltages[4] & 0xF0) >> 4 | (voltages[5] & 0xFF) << 4;
  data->voltage[4] = (voltages[6] & 0xFF) | (voltages[7] & 0x0F) << 8;
  data->voltage[5] = (voltages[7] & 0xF0) >> 4 | (voltages[8] & 0xFF) << 4;
  data->voltage[6] = (voltages[9] & 0xFF) | (voltages[10] & 0x0F) << 8;
  data->voltage[7] = (voltages[10] & 0xF0) >> 4 | (voltages[11] & 0xFF) << 4;
  data->voltage[8] = (voltages[12] & 0xFF) | (voltages[13] & 0x0F) << 8;
  data->voltage[9] = (voltages[13] & 0xF0) >> 4 | (voltages[14] & 0xFF) << 4;
  data->voltage[10] = (voltages[15] & 0xFF) | (voltages[16] & 0x0F) << 8;
  data->voltage[11] = (voltages[16] & 0xF0) >> 4 | (voltages[17] & 0xFF) << 4;
  data->temperature[0] = (temperatures[0] & 0xFF) | (temperatures[1] & 0x0F) << 8;
  data->temperature[1] = (temperatures[1] & 0xF0) >> 4 | (temperatures[2] & 0xFF) << 4;
  data->temperature[2] = (temperatures[3] & 0xFF) | (temperatures[4] & 0x0F) << 8;
}

void PrintErrorMessage(enum error_codes code) {
  char buffer[MAX_STR_SIZE];
  if (code > UNKNOWN_SHUTDOWN) {
    code = UNKNOWN_SHUTDOWN;
  }
  strcpy_P(buffer, (char *)pgm_read_word(&(error_code_lookup[code])));
  Serial.println(buffer);
}

int HighestVoltage(const LTData *board) {
  int max = 0x00;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    for (int j = 0; j < kLTNumOfCells[i]; ++j) {
      if (max < board[i].voltage[j]) {
        max = board[i].voltage[j];
      }
    }
  }
  return max;
}

int LowestVoltage(const LTData *board) {
  int min = 0x7fff;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    for (int j = 0; j < kLTNumOfCells[i]; ++j) {
      if (min > board[i].voltage[j]) {
        min = board[i].voltage[j];
      }
    }
  }
  return min;
}

float HighestTemperature(const LTData *board) {
  float max = 0x00;
  float current;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    current = ToTemperature(board[i].temperature[0]);
    if (max < current) {
      max = current;
    }
    current = ToTemperature(board[i].temperature[1]);
    if (max < current) {
      max = current;
    }
    // The internal die temperature seems unreliable for now, so let's 
    // ignore it when determining if we have too high a temperature.
    //    current = CONVERT_THIRD_TO_CELCIUS(board[i].temperature[2]);
    //    if (max < current) {
    //      max = current;
    //    }
  }
  return max;
}

float ToTemperature(int temp) {
  float voltage = temp * LT_VOLT_TO_FLOAT;
  float resist = 1 / (V_INF / voltage - 1);
  return THERM_B / log(resist / R_INF) - CELCIUS_KELVIN_BIAS;
}


void BalanceBatteries(const LTData *lt_board) {
  int lowest_voltage = LowestVoltage(lt_board);
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    for (int j = 0; j < kLTNumOfCells[i]; ++j) {
      byte discharge = lt_board[i].voltage[j] > lowest_voltage + DISCHARGE_GAP;
      #ifdef VERBOSE
        if (discharge) {
          Serial.print("Discharging board ");
          Serial.print(i);
          Serial.print(", cell ");
          Serial.println(j);
        }
      #endif
      if (j < 8 & discharge) {
        lt_config[i][1] = (lt_config[i][1] & !(1 << j)) | (discharge << j);
      } 
      else {
        lt_config[i][2] = (lt_config[i][2] & !(1 << (j - 8))) | (discharge << (j - 8));
      }
    }
    #ifdef VERBOSE
      Serial.print("Setting board ");
      Serial.print(i);
      Serial.print(" to config ");
      Serial.print(lt_config[i][1], HEX);
      Serial.println(lt_config[i][2], HEX);
    #endif
  }
}


