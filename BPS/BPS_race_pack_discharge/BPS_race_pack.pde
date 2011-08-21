#include "SPI.h"  
// LTC6802-2 BMS with isolator installed 
// pin 39 - SPI Clock 
// pin 38 - MISO 
// pin 37 - MOSI 
// pin B0 - Slave Select  
#define START_BPS_SPI digitalWrite(B0,LOW) //Begins chip communication 
#define END_BPS_SPI digitalWrite(B0,HIGH)  //Ends chip communication  

//Control Bytes 
#define WRCFG 0x01 //write config 
#define RDCFG 0x02 //read config 
#define RDCV  0x04 //read cell voltages 
#define RDFLG 0x06 //read voltage flags 
#define RDTMP 0x08 //read temperatures  
#define STCVAD 0x10 //start cell voltage A/D conversion 
#define STTMPAD 0x30 //start temperature A/D conversion  
#define DSCHG 0x60 //start cell voltage A/D allowing discharge
#define fan1 16
#define fan2 17

const byte board_address[3] = {0x80, 0x81, 0x82};
const byte OV = 0xAB; //4.1V
const byte UV = 0x71; //2.712V
const float over = 4.1;
const float under = 2.7;
const int B = 3988;

byte config[6] = {0xE5, 0x00, 0x00, 0x00, UV, OV}; //Default config
int heartrate= 200; //send heartbeat every 200ms;
unsigned long lastHeartbeat =0;

//Write the configuration 
void writeConfig(byte* config) {
  START_BPS_SPI;
  SPI.transfer(WRCFG); //configuration is written to all chips
  for(int i=0;i<6;i++) {
    SPI.transfer(config[i]);
  }
  END_BPS_SPI; 
}

//Write single configuration to board
void writeConfig(byte* config, byte board) {
  START_BPS_SPI;
  SPI.transfer(board); //non-broadcast command
  SPI.transfer(WRCFG); //configuration is written to all chips
  for(int i=0;i<6;i++) {
    SPI.transfer(config[i]);
    
  }
  END_BPS_SPI; 
}

//Read the configuration for a board
void readConfig(byte* config, byte board) {
  START_BPS_SPI;   
  SPI.transfer(board); //board address is selected
  SPI.transfer(RDCFG); //configuration is read
  for(int i=0;i<6;i++) {
    config[i] = SPI.transfer(RDCFG);
  }
  END_BPS_SPI;
}

// Begins CV A/D conversion
void beginCellVolt() {
  START_BPS_SPI;
  SPI.transfer(STCVAD);
  delay(15); //Time for conversions, approx 12ms
  END_BPS_SPI;
}

// Reads cell voltage registers  
void readCellVolt(float* cell_voltage, byte board) {
  START_BPS_SPI;
  SPI.transfer(board); // board address is selected
  SPI.transfer(RDCV); // cell voltages to be read
  byte cvr[18]; // buffer to store unconverted values
  for(int i=0;i<18;i++) {
    cvr[i] = SPI.transfer(RDCV);
  }
  END_BPS_SPI; 

  //converting cell voltage registers to cell voltages
  cell_voltage[0] = (cvr[0] & 0xFF) | (cvr[1] & 0x0F) << 8;
  cell_voltage[1] = (cvr[1] & 0xF0) >> 4 | (cvr[2] & 0xFF) << 4;
  cell_voltage[2] = (cvr[3] & 0xFF) | (cvr[4] & 0x0F) << 8;
  cell_voltage[3] = (cvr[4] & 0xF0) >> 4 | (cvr[5] & 0xFF) << 4;
  cell_voltage[4] = (cvr[6] & 0xFF) | (cvr[7] & 0x0F) << 8;
  cell_voltage[5] = (cvr[7] & 0xF0) >> 4 | (cvr[8] & 0xFF) << 4;
  cell_voltage[6] = (cvr[9] & 0xFF) | (cvr[10] & 0x0F) << 8;
  cell_voltage[7] = (cvr[10] & 0xF0) >> 4 | (cvr[11] & 0xFF) << 4;
  cell_voltage[8] = (cvr[12] & 0xFF) | (cvr[13] & 0x0F) << 8;
  cell_voltage[9] = (cvr[13] & 0xF0) >> 4 | (cvr[14] & 0xFF) << 4;
  cell_voltage[10] = (cvr[15] & 0xFF) | (cvr[16] & 0x0F) << 8;
  cell_voltage[11] = (cvr[16] & 0xF0) >> 4 | (cvr[17] & 0xFF) << 4;
  
  for(int i=0;i<12;i++) {
    cell_voltage[i] = cell_voltage[i] * 1.5 * 0.001;
  }
}  

void beginTemp() {
  START_BPS_SPI;
  SPI.transfer(STTMPAD);
  delay(15); //Time for conversions
  END_BPS_SPI;
}

//Reads temperatures
void readTemp(short*temp,byte board) {
  START_BPS_SPI;
  SPI.transfer(board); //board address is selected
  SPI.transfer(RDTMP); //temperatures to be read
  byte tempr[5];
  for(int i=0;i<5;i++) {
    tempr[i] = SPI.transfer(RDTMP);
  }
  END_BPS_SPI;
  //convert temperature registers to temperatures
  temp[0] = (tempr[0] & 0xFF) | (tempr[1] & 0x0F) << 8;
  temp[1] = (tempr[1] & 0xF0) >> 4 | (tempr[2] & 0xFF) << 4;
  temp[2] = (tempr[3] & 0xFF) | (tempr[4] & 0x0F) << 8;
}

// Converts external thermistor voltage readings 
// into temperature (K) using B-value equation
void convertVoltTemp(short*volt,float*temp) {
  float resist[2];
  resist[0] = (10000 / ((3.11/(volt[0]*1.5*0.001))-1));
  resist[1] = (10000 / ((3.11/(volt[1]*1.5*0.001))-1));
  float rinf = 10000 * exp(-1*(B/298.15));
  temp[0] = (B/log(resist[0]/rinf)) - 273.15;
  temp[1] = (B/log(resist[1]/rinf)) - 273.15;

  temp[2] = (volt[2]*1.5*0.125) - 273;
}

//Sends out CAN packet
void sendCAN(int ID,char*data,int size) {
  CanMessage msg = CanMessage();
  msg.id = ID; //msg id isn't decided yet
  msg.len = size;
  for(int i=0;i<size;i++) {
    msg.data[i] = data[i];
  }
  Can.send(msg);
}

//Checks voltages for undervoltage
boolean checkOverVoltage(float*voltages,float limit,int length) {
  for(int i=0;i<length;i++) {
    if(voltages[i] >= limit) {
      return true;
    }
  }
  return false;
}

//Checks voltages for undervoltage
boolean checkUnderVoltage(float*voltages,float limit,int length) {
  for(int i=0;i<length;i++) {
    if(voltages[i] <= limit) {
      Serial.print("Index: ");
      Serial.println(i);
      return true;
    }
  }
  return false;
}

//Checks temperatures within range
boolean checkTemperatures(float*temps,float limit) {
  for(int i=0;i<2;i++) {
    if(temps[i] >= limit) {
      return true;
    }
  }
  return false;
}

void setup() {
  Can.begin(1000);
  Serial.begin(115200);
  char init[1] = { 0x00 };
  sendCAN(0x041,init,1);
  pinMode(B0,OUTPUT);
  pinMode(fan1,OUTPUT);
  pinMode(fan2,OUTPUT);
  digitalWrite(fan1,HIGH);
  digitalWrite(fan2,HIGH);
  END_BPS_SPI; //Sets Slave Select to high (unselected)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
  delay(500);
}

void loop() {
  config[0] = 0xE5;
  writeConfig(config);
  boolean discharge = false;
  boolean warning_ov = false;
  boolean warning_uv = false;
  boolean warning_temp = false;
  boolean error = false;
  
  // Iterate through BPS module boards.
  for(int k=0; k<3; k++) {
    // If we're on board 0 or 1, then check 12 cells
    // If we're on board 2, then theres just 11 cells
    int length;
    if(k == 2) {
      length = 11;
    } else {
      length = 12
    }
    
    // Ping the boards to make sure the config match
    byte rconfig[6];
    readConfig(rconfig, board_address[k]);
    if(rconfig[0]==0xFF || rconfig[0]==0x00 || rconfig[0]==0x02) {
      Serial.print("Board not communicating: ");
      Serial.println(k);
      error = true;
      continue;
    }

    // Measures voltages
    float cell_voltages[12];
    // Send "Start ADC" command to BPS module.  Includes a 15 ms delay.
    beginCellVolt();
    // Stores ADC readings from board k into the cell_voltages array
    readCellVolt(cell_voltages, board_address[k]);
    
    // Read Temperatures
    short raw_temperature_readings[3];
    float converted_temperature_readings[3];
    // Send "Start Temperature" command to BPS module.  Includes a 15 ms delay.
    beginTemp();
    // Queries the BPS module for temperature readings.  Stores the readings
    // in raw_temperature_readings
    readTemp(raw_temperature_readings, board_address[k]);
    // Convert temperature readings into kelvin readings.
    convertVoltTemp(raw_temperature_readings, converted_temperature_readings);
    
    // Checks for overvoltage problems, raises flag if necessary.
    if(checkOverVoltage(cell_voltages, 4.0, length)) {
      discharge = true; //balance cells
      if(checkOverVoltage(cell_voltages, 4.05, length)) { 
        if(checkOverVoltage(cell_voltages, over, length)) {
          //if batteries over 4.1V shut down system
          char overchg[1] = { 0x01 };
          sendCAN(0x021, overchg, 1);
          error = true;
          delay(10);
          Serial.println("Overvoltage cells");
          delay(30000);
        } else {
          //if batteries over 4.05V create warning.
          warning_ov = true;          
        }
      }
    }
    
    // Checks for undervoltage problems, raises flag if necessary
    if(checkUnderVoltage(cell_voltages,2.8,length)) {
      if(checkUnderVoltage(cell_voltages,under,length)) {
        // if batteries under 2.7V raise warning
        char underchg[1] = { 0x02 };
        sendCAN(0x021,underchg,1);
        error = true;
        delay(10);
        Serial.println("Critical Undervoltage cells");
        Serial.print("Pack: ");
        Serial.println(k);
        delay(30000);
      } else {
        //if batteries under 2.8V raise warning
        warning_uv = true;
      }
    }
    
    // Check for absolute temperature problems
    if(checkTemperatures(converted_temperature_readings,50)) {
      if(checkTemperatures(converted_temperature_readings,55 )) {
        char exceedtmp[1] = { 0x04 };
        sendCAN(0x021,exceedtmp,1);
        error = true;
        delay(10);
        Serial.println("Exceeded Temperature Limit");
        delay(30000);
      } else {
        warning_temp = true;
      }
    }

    //Sending out CAN packets
    for(int i=0; i<(length+3); i++) {
      char data[4];
      int ID = (1 << 8) | (k << 4) | i;
      if(i < length) {
        if(k==2 && i > 8 && i < 12) {
          continue;
        } else {
          data[0] = *((char*)&cell_voltages[i]);
          data[1] = *((char*)&cell_voltages[i]+1);
          data[2] = *((char*)&cell_voltages[i]+2);
          data[3] = *((char*)&cell_voltages[i]+3);
        }
      } else {
          data[0] = *((char*)&converted_temperature_readings[i-length]);
          data[1] = *((char*)&converted_temperature_readings[i-length]+1);
          data[2] = *((char*)&converted_temperature_readings[i-length]+2);
          data[3] = *((char*)&converted_temperature_readings[i-length]+3);
      }
      sendCAN(ID,data,4);
    }
    
    // Basic discharge, will discharge cells if voltage > 4.0V
    if(discharge) {
      for(int i=0;i<12;i++) {
        if(cell_voltages[i] > 4.0) {
          if(i < 8) {
            config[1] = config[1] | (1 << i); //Sets DCC bits 0-7
          } else {
            config[2] = config[2] | (1 << (i-8)); //Sets DCC bits 8-11
          }
        }
      }
    }
  }
  
  if ((millis() - lastHeartbeat) > heartrate) {
    // Heartbeat, tells board in case of warning or error.
    if(error) {
      char err[1] = { 0x04 };
      sendCAN(0x041,err,1);
      delay(10);
      Serial.println("Critical Error detected");
    } else if(warning_uv) {
      char warn_uv[1] = { 0x01 };
      sendCAN(0x041,warn_uv,1);
      delay(10);
      Serial.println("Undervolt Warning detected");
      warning_uv=0; //reset warning
    } else if(warning_ov) {
      char warn_ov[1] = { 0x02 };
      sendCAN(0x041,warn_ov,1);
      delay(10);
      Serial.println("Overvolt Warning detected");
      warning_ov=0; //reset warning
    } else if(warning_temp) {
      char warn_temp[1] = { 0x03 };
      sendCAN(0x041,warn_temp,1);
      delay(10);
      Serial.println("Temperature Warning detected");
      warning_temp=0; //reset warning
    } else {
      char ok[1] = { 0x00 };
      sendCAN(0x041,ok,1);
      delay(10);
      Serial.print("BPS operations OK ");
      Serial.println(millis());
    }
    lastHeartbeat = millis();
  }
  
  // Standby Configuration
  if(!discharge) {
    config[0] = 0xE0;
    config[1] = 0x00;     // Reset discharge bits
    config[2] = 0x00;     // Reset discharge bits
    writeConfig(config);  // Writes the standby config (low current)
  }
}
