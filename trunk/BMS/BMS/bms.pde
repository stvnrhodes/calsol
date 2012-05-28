/* CalSol - UC Berkeley Solar Vehicle Team
 * bms.pde - BMS Module
 * Purpose: Main code for the BMS module
 * Author(s): Steven Rhodes.
 * Date: May 1 2012
 */

// #define CAN_DEBUG
// #define VERBOSE
// #define SPI_DEBUG

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

// Hackish, but I need to maintain a counter for this somewhere
byte lt_counter = 0;

// Only used to average out readings from LT boards
LTMultipleData averaging_data[NUM_OF_LT_BOARDS];

void setup() {
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
    #ifdef VERBOSE
      print("Car State: ");
      println(car_state);
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
        flags.charging_disabled = true;
        DisableCharging();
        break;
      case ENABLE_CHARGING:
        flags.charging_disabled = false;
        EnableCharging();
        break;
      case IN_PRECHARGE:
      case CAR_ON:
      case CAR_OFF:
      default:
        break;
    }
    if (!buzzer.IsPlaying()) {
      if (flags.battery_overvoltage_warning || flags.module_undervoltage_warning) {
        buzzer.PlaySong(kHighVoltageBeep);
      } else if (flags.battery_undervoltage_warning || flags.module_undervoltage_warning) {
        buzzer.PlaySong(kLowVoltageBeep);
      } else if (flags.battery_overtemperature_warning) {
        buzzer.PlaySong(kHighTemperatureBeep);
      }
    }
    CarDataFloat float_data;
    ConvertCarData(&float_data, &car_data);
    SendGeneralDataCanMessage(&float_data);
    data_time = time;
  }

  if (time - battery_time > BATTERY_TIME_LENGTH) {
    // We send out LT CAN messages one at a time to avoid overloading CAN
    if (car_data.lt_board[lt_counter].is_valid) {
      SendLtBoardCanMessage(car_data.lt_board + lt_counter, lt_counter);
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
    Serial.println("Hello World"); // Delete this soon
    #ifdef CAN_DEBUG
      Serial.print("Can RX: ");
      Serial.print(Can.rxError());
      Serial.print(" TX: ");
      Serial.print(Can.txError());
      Serial.print(" Error Flags: ");
      Serial.println(Can._mcp2515.read(EFLG), HEX);
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
  Serial.print("Last shutdown: ");
  PrintErrorMessage((enum error_codes) EEPROM.read(memory_index));
  #ifdef VERBOSE
    Serial.println("Earlier shutdown messages:");
    for (i = memory_index - 1; i != memory_index; --i) {
      if (i == 0) {
        i = NUM_OF_ERROR_MESSAGES);
      }
      Serial.print(i);
      Serial.print(": ");
      PrintErrorMessage((enum error_codes) EEPROM.read(memory_index))
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
  pinMode(RELAY3, OUTPUT); // DELETE?
  pinMode(LVRELAY, OUTPUT);
  pinMode(LEDFAIL, OUTPUT); // DELETE?
  pinMode(IO_T4, OUTPUT); // DELETE?
  pinMode(LT_CS, OUTPUT);
  pinMode(FAN1, OUTPUT);
  pinMode(FAN2, OUTPUT);
  pinMode(KEY_SWITCH, INPUT);
  pinMode(IN_SONG1, INPUT); // DELETE?
  pinMode(IN_SONG2, INPUT); // DELETE?
  pinMode(V_BATTERY, INPUT);
  pinMode(V_MOTOR, INPUT);
  pinMode(C_BATTERY, INPUT);

  digitalWrite(OUT_BUZZER, LOW);
  digitalWrite(BATTERY_RELAY, LOW);
  digitalWrite(SOLAR_RELAY, LOW);
  digitalWrite(RELAY3, LOW); // DELETE?
  digitalWrite(LVRELAY, LOW);
  digitalWrite(LEDFAIL, LOW); // DELETE?
  digitalWrite(LT_CS, HIGH);
  digitalWrite(FAN1, LOW);
  digitalWrite(FAN2, LOW);
  digitalWrite(KEY_SWITCH, HIGH);
  digitalWrite(IN_SONG1, HIGH); // DELETE?
  digitalWrite(IN_SONG2, HIGH); // DELETE?
  digitalWrite(V_BATTERY, HIGH); //DELETE?
  digitalWrite(V_MOTOR, HIGH); //DELETE?
}

void InitSpi(void) {
  #ifdef SPI_DEBUG
    println("Initializing SPI");
  #endif
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
}

void InitLtBoardData(void) {
  for (byte i = 0; i < NUM_OF_LT_BOARDS; ++i) {
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
          lt_board->voltage[k] = 0x8fff;
        }
        for (byte k = 0; k < NUM_OF_TEMPERATURES; ++k) {
          lt_board->temperature[k] = 0x8fff;
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
    println("Sending Cutoff and BPS Hearbeats");
  #endif
  Can.send(CanMessage(CAN_HEART_CUTOFF));
  Can.send(CanMessage(CAN_HEART_BPS));
}

void GetCarData(CarDataInt *car_data) {

  SpiStart();
  byte config = WRCFG;
  SendToSpi(&config, 1);
  SendToSpi(kConfig, 6);
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

  // TODO(stvn): Check assumptions about negative == charging
  flags->discharging_overcurrent = car_data->battery_current > OVERCURRENT_CUTOFF;
  flags->charging_overcurrent = car_data->battery_current < CHARGING_OVERCURRENT_CUTOFF;
  flags->batteries_charging = car_data->battery_current < 0;
  
  flags.motor_precharged = (car_data->motor_voltage > MOTOR_MINUMUM_VOLTAGE) &&
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
    flags->charging_overtemperature_warning = high_temperature > CHARGING_OVERTEMP_WARNING;

    if (flags->charging_disabled && high_temperature < OK_TO_CHARGE) {
      flags->charging_disabled = false;
    } else if (!flags->charging_disabled && high_temperature > OVERTEMPERATURE_NO_CHARGE) {
      flags->charging_disabled = true;
    }

    flags->too_full_to_charge = flags->module_overvoltage_warning || flags->battery_overvoltage_warning;
  }
  
  flags->keyswitch_on = IsKeyswitchOn();
}

// There may be too many strings here if we define VERBOSE
CarState GetCarState(const CarState old_state, const Flags *flags) {
  if (old_state == TURN_OFF) {
    #ifdef VERBOSE
      Serial.println("Car is shut down");
    #endif
    return CAR_OFF;
  }
  if (car_state == CAR_ON && !(flags->charging_disabled)) {
    if (flags->too_full_to_charge || flags->too_hot_to_charge){
      #ifdef VERBOSE
        Serial.println("Unsafe to charge, disabling charging");
      #endif
      return DISABLE_CHARGING;
    }
  }
  if (old_state != CAR_OFF) {
    if (flags->missing_lt_communication) {
      #ifdef VERBOSE
        Serial.println("Shutting down, can't talk to an LT Board");
      #endif
      EEPROM.write(EEPROM.read(0), BPS_DISCONNECTED);
      return TURN_OFF;
    }
    if (flags->battery_overvoltage) {
      #ifdef VERBOSE
        Serial.println("Shutting down, the battery pack is too high voltage");
      #endif
      EEPROM.write(EEPROM.read(0), S_OVERVOLT);
      return TURN_OFF;
    }
    if (flags->battery_undervoltage) {
      #ifdef VERBOSE
        Serial.println("Shutting down, the battery pack is too low voltage");
      #endif
      EEPROM.write(EEPROM.read(0), S_UNDERVOLT);
      return TURN_OFF;
    }
    if (flags->module_overvoltage) {
      #ifdef VERBOSE
        Serial.println("Shutting down, a battery module is too high voltage");
      #endif
      EEPROM.write(EEPROM.read(0), BPS_OVERVOLT);
      return TURN_OFF;
    }
    if (flags->module_undervoltage) {
      #ifdef VERBOSE
        Serial.println("Shutting down, a battery module is too low voltage");
      #endif
      EEPROM.write(EEPROM.read(0), BPS_UNDERVOLT);
      return TURN_OFF;
    }
    if (flags->battery_overcurrent) {
      #ifdef VERBOSE
        Serial.println("Shutting down, batteries are putting out too much current");
      #endif
      EEPROM.write(EEPROM.read(0), S_OVERCURRENT);
      return TURN_OFF;
    }
    if (flags->battery_charging_overcurrent) {
      #ifdef VERBOSE
        Serial.println("Shutting down, batteries are taking in too much current");
      #endif
      EEPROM.write(EEPROM.read(0), S_OVERCURRENT);
      return TURN_OFF;
    }
    if (flags->batteries_charging && flags->charging_overtemperature) {
      #ifdef VERBOSE
        Serial.println("Shutting down, batteries charging but too hot to charge");
      #endif
      EEPROM.write(EEPROM.read(0), BPS_OVERTEMP);
      return TURN_OFF;
    }
    if (!(flags->keyswitch_on)) {
      #ifdef VERBOSE
        Serial.println("Shutting down, keyswitch in off position");
      #endif
      EEPROM.write(EEPROM.read(0), KEY_OFF);
      return TURN_OFF;
    }
  }
  if (old_state == TURN_ON) {
    #ifdef VERBOSE
      Serial.println("Car is on");
    #endif
    return CAR_ON;
  }
  if (old_state == CAR_OFF && flags->keyswitch_on) {
      #ifdef VERBOSE
        Serial.println("Beginning Precharge");
      #endif
    return IN_PRECHARGE;
  }
  if (old_state == IN_PRECHARGE && flags->motor_precharged) {
    #ifdef VERBOSE
      Serial.println("Precharge completed, turning on");
    #endif
    return TURN_ON;
  }
  if (car_state == CAR_ON && flags->charging_disabled) {
    if (!(flags->too_full_to_charge) && !(flags->too_hot_to_charge)){
      #ifdef VERBOSE
        Serial.println("Safe to charge, reenabling charging");
      #endif
      return ENABLE_CHARGING;
    }
  }
  if (old_state == DISABLE_CHARGING || old_state == ENABLE_CHARGING) {
    return CAR_ON;
  }
  return old_state;
}

void ConvertCarData(CarDataFloat *out, const CarDataInt *in) {
  out->battery_voltage = in->battery_voltage / 1000.0;
  out->motor_voltage = in->motor_voltage / 1000.0;
  out->battery_current = in->battery_current / 1000.0;
  out->solar_current = in->solar_current / 1000.0

void DisableCharging (void) {
  #ifdef CAN_DEBUG
    Serial.print("Not yet implemented");
    Serial.println("Sending message to prevent regen");
  #endif
  // TODO(stvn): Send CAN message to prevent regen
  digitalWrite(C_SOLAR, LOW);
}

void EnableCharging (void) {
  #ifdef CAN_DEBUG
    Serial.print("Not yet implemented");
    Serial.println("Sending message to allow regen");
  #endif
  // TODO(stvn): Send CAN message to allow regen
  digitalWrite(C_SOLAR, HIGH);
}

signed int GetBatteryVoltage(void) {
  static signed int[] old_readings = {0xffff, 0x8fff, 0xffff};
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
  static signed int[] old_readings = {0xffff, 0x8fff, 0xffff};
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
  static signed int[] old_readings = {0xffff, 0x8fff, 0xffff};
  static byte ptr = 0;
  
  old_readings[ptr] = analogRead(C_BATTERY) - analogRead(C_GND);
  ptr = (ptr + 1) % 3;
  #ifdef VERBOSE
    Serial.print("Battery Current: ");
    Serial.println(GetIntMedian(old_readings));
  #endif
  return GetIntMedian(old_readings);
}

signed int GetSolarCellCurrent(void) {
  static signed int[] old_readings = {0xffff, 0x8fff, 0xffff};
  static byte ptr = 0;
  
  old_readings[ptr] = analogRead(C_SOLAR) - analogRead(C_GND);
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

void GetLtDataMedian(LTData median, const LTMultipleData * array) {
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

void GetLtBoardData(LTData *new_data, byte board_num,
    const LTMultipleData * avg_data) {
  byte config;

  SpiStart();
  SendToSpi(kBoardAddress + board_num, 1);
  config = RDCFG
  SendToSpi(&config);
  byte rconfig[6]
  GetFromSpi(rconfig, RDCFG, 6);
  SpiEnd();
  
  if (rconfig[0] == 0xFF || rconfig[0] == 0x00 || rconfig[0] == 0x02) {
    //  If we get one of these responses, it means the LT board is not communicating.
    new_data->is_valid = false;
    return;
  }
  
  SpiStart();
  config = STCVAD;
  SendToSpi(&config, 1);
  delay(15); //Time for conversions, approx 12ms
  SpiEnd();
  
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
}

void SendLtBoardCanMessage(const LTData * data, byte board_num) {
  int board_address = CAN_BPS_BASE + board_num * CAN_BPS_MODULE_OFFSET;
  TwoFloats msg;
  #ifdef CAN_DEBUG
    Serial.print("Sending LtBoardCanMessage ");
    Serial.print(board_num)
    Serial.print(", voltages: ");
  #endif
  for (int i = 0; i < kNumOfCells[board_num]; ++i) {
    msg.f[0] = data->voltage[i] * LT_VOLT_TO_FLOAT;
    #ifdef CAN_DEBUG
      Serial.print(msg.f[0]);
      Serial.print(", ");
    #endif
    Can.send(CanMessage(board_address + i, msg.c, 4);
  }
  #ifdef CAN_DEBUG
    Serial.println(" and some temperatures, too.");
  #endif
  board_address += CAN_BPS_TEMP_OFFSET;
  msg.f[0] = ToTemperature(data->temperature[0]);
  Can.send(CanMessage(board_address, msg.c, 4);
  msg.f[0] = ToTemperature(data->temperature[1]);
  Can.send(CanMessage(board_address + 1, msg.c, 4);
  msg.f[0] = data->temperature[2] * LT_THIRD_TEMP_TO_FLOAT - KELVIN_CELCIUS_BIAS;
  Can.send(CanMessage(board_address + 2, msg.c, 4);
}
  
void SendGeneralDataCanMessage(const CarDataFloat * data) {
  TwoFloats msg;
  #ifdef CAN_DEBUG
    Serial.print("Sending General CAN Message, ");
    Serial.print("battery voltage: ");
    Serial.print(data->battery_voltage);
    Serial.print(", motor voltage: ");
    Serial.print(data->motor_voltage);
    Serial.print(", battery current: ");
    Serial.print(data->battery_current);
    Serial.print(", solar current: ");
    Serial.println(data->solar_current);
  #endif
  msg.f[0] = data->battery_voltage;
  msg.f[1] = data->motor_voltage;
  Can.send(CanMessage(CAN_CUTOFF_VOLT, msg.c, 8));
  msg.f[0] = data->battery_current;
  msg.f[1] = data->solar_current;
  Can.send(CanMessage(CAN_CUTOFF_CURR, 8));
}  

void SendErrorCanMessage(Flags *flags) {
  // TODO(stvn): Figure out what to send here
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
}

void SendToSpi(const byte * data, int length) {
  for (int i = 0; i < length; ++i) {
    SPI.transfer(data[i]);
  }
}
    
void GetFromSpi(byte * data, byte info, int length) {
  for (int i = 0; i < length; ++i) {
    data[i] = SPI.transfer(info);
  }
}

void ParseSPIData(LTData *data, const byte voltages[], 
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
    code = UNKNOWN_SHUTDOWN);
  }
  strcpy_P(buffer, (char *)pgm_read_word(&(error_code_lookup[code])));
  Serial.println(buffer);
}

int HighestVoltage(LTData *board) {
  int max = 0x00;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    for (int j = 0; j < kNumOfCells[i]; ++j) {
      if (max < board[i].voltage[j]) {
        max = board[i].voltage[j];
      }
    }
  }
  return max;
}

int LowestVoltage(LTData *board) {
  int min = 0x00;
  for (int i = 0; i < NUM_OF_LT_BOARDS; ++i) {
    for (int j = 0; j < kNumOfCells[i]; ++j) {
      if (min > board[i].voltage[j]) {
        min = board[i].voltage[j];
      }
    }
  }
  return min;
}

float HighestTemperature(LTData *board) {
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
    current = board[i].temperature[2] * LT_THIRD_TEMP_TO_FLOAT;
    if (max < current) {
      max = current;
    }
  }
  return max;
}

float ToTemperature(int temp) {
  float voltage = temp * LT_VOLT_TO_FLOAT;
  float resist = 1 / (V_INF / voltage - 1);
  return THERM_B / log(resist / R_INF) - CELCIUS_KELVIN_BIAS;
}

byte GetPEC(const byte * din, int n) {
  // TODO(stvn): Implement this, not necessary unless we switch chips
}

