/* CalSol - UC Berkeley Solar Vehicle Team 
 * bps.h - BMS Module
 * Purpose: Helper methods for BPS functions of the BMS
 * Author(s): Ryan Tseng, Stephen Chen
 * Date: Sept 25th 2011
 */
 
#ifndef _BPS_H_
#define _BPS_H_

#include "SPI.h"  
#include "pindef.h"
#include "can_id.h"

//#define LTC6803

// Macros for beginning and ending LT SPI transmission
#define LT_SPI_START digitalWrite(LT_CS,LOW)
#define LT_SPI_END digitalWrite(LT_CS,HIGH)

// Utility macros
#define MAX(A, B) (A > B) ? A : B
#define MIN(A, B) (A < B) ? A : B

// Control Bytes 
#define WRCFG 0x01   // Write config 
#define RDCFG 0x02   // Read config 
#define RDCV  0x04   // Read cell voltages 
#ifdef LTC6803
  #define RDFLG 0x0c
  #define RDTMP 0x0e
#else
  #define RDFLG 0x06   // Read voltage flags 
  #define RDTMP 0x08   // Read temperatures  
#endif
#define STCVAD 0x10  // Start cell voltage A/D conversion 
#define STTMPAD 0x30 // Start temperature A/D conversion  
#define DSCHG 0x60   // Start cell voltage A/D allowing discharge

// Configuration constants
#define OVERVOLTAGE_WARNING_LEVEL  4.1
#define OVERVOLTAGE_ERROR_LEVEL    4.2
#define UNDERVOLTAGE_WARNING_LEVEL 3.0
#define UNDERVOLTAGE_ERROR_LEVEL   2.7
#define OVERTEMP_WARNING_LEVEL 55
#define OVERTEMP_ERROR_LEVEL 60

// LT board constants
const byte board_address[3] = { 0x80 , 0x81 , 0x82 };
const byte OV = 0xAB;  // 4.1V
const byte UV = 0x71;  // 2.712V
const float over = 4.1;
const float under = 2.7;
const int B = 3988;

byte config[6] = { 0xE5,0x00,0x00,0x00,UV,OV };  //Default config

// Global flags
int overtemp_error = 0;
int overtemp_warning = 0;
int overvolt_error = 0;
int overvolt_warning = 0;
int undervolt_error = 0;
int undervolt_warning = 0;
int system_error = 0;

// Global variables
float battery_voltages[3][12];
float temperatures[3][3];
int can_current_module = 0;

// Function
void printBpsData();

// Calculate PEC, n is  size of DIN
byte getPEC(byte * din, int n) {
  byte pec, in0, in1, in2;
  pec = 0x41;
  for(int j=0; j<n; j++) {
    for(int i=0; i<8; i++) {
      in0 = ((din[j] >> (7 - i)) & 0x01) ^ ((pec >> 7) & 0x01);
      in1 = in0 ^ ((pec >> 0) & 0x01);
      in2 = in0 ^ ((pec >> 1) & 0x01);
      pec = in0 | (in1 << 1) | (in2 << 2) | ((pec << 1) & ~0x07);
    }
  }
  return pec;
}

// Send to SPI, with PEC for LTC6803
void sendMultipleToSPI(byte * data, int n) {
  for(int i=0; i<n; i++) {
    SPI.transfer(data[i]);
  }
  #ifdef LTC6803
    SPI.transfer(getPEC(data, n));
    #ifdef BPS_DEBUG
      Serial.print("Sending: ");
      for(int i=0; i<n; i++) {
        Serial.print(data[i], HEX);
        Serial.print(", ");
      }
      Serial.println(getPEC(data, n), HEX);
    #endif
  #endif
}

// Get data from SPI, with PEC for LTC6803
byte * getMultipleFromSPI(byte * data, byte info, int n) {
  for(int i=0; i<n; i++) {
    data[i] = SPI.transfer(info);
  }
  #ifdef LTC6803
    byte pec = SPI.transfer(info);
    #ifdef BPS_DEBUG
      Serial.print("Receiving: ");
      for(int i=0; i<n; i++) {
        Serial.print(data[i], HEX);
        Serial.print(", ");
      }
      Serial.print(pec, HEX);
      Serial.print(" vs ");
      Serial.println(getPEC(data, n), HEX);
      if(pec != getPEC(data, n)) {
        Serial.println("BPS: Problem with PEC");
      }
    #endif
  #endif
}

void sendToSPI(byte data) {
  sendMultipleToSPI(&data, 1);
}

// Init SPI
void initBps() {
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
}

// Write config to the LT board's configuration
void writeConfig(byte* config) {
  LT_SPI_START;
  sendToSPI(WRCFG);  // Configuration is written to all chips
  sendMultipleToSPI(config, 6);
  LT_SPI_END; 
}

// Write config to a single board
void writeConfig(byte* config, byte board) {
  LT_SPI_START;
  sendToSPI(board);  // Non-broadcast command
  sendToSPI(WRCFG);  // Configuration is written to all chips
  sendMultipleToSPI(config, 6);
  LT_SPI_END; 
}

// Read the configuration for a board
void readConfig(byte*config,byte board) {
  LT_SPI_START;   
  sendToSPI(board);  // Board address is selected
  sendToSPI(RDCFG);  // Configuration is read
  getMultipleFromSPI(config, RDCFG, 6);
  LT_SPI_END;
}

// Begins CV A/D conversion
void beginCellVolt() {
  LT_SPI_START;
  sendToSPI(STCVAD);
  delay(15); //Time for conversions, approx 12ms
  LT_SPI_END;
}

// Reads cell voltage registers  
void readCellVolt(float* cell_voltages, byte board) {
  LT_SPI_START;
  sendToSPI(board);  // Board address is selected
  sendToSPI(RDCV);  // Cell voltages to be read
  byte cvr[18];  // Buffer to store unconverted values
  getMultipleFromSPI(cvr, RDCV, 18);
  LT_SPI_END; 

  // Converting cell voltage registers to cell voltages
  cell_voltages[0] = (cvr[0] & 0xFF) | (cvr[1] & 0x0F) << 8;
  cell_voltages[1] = (cvr[1] & 0xF0) >> 4 | (cvr[2] & 0xFF) << 4;
  cell_voltages[2] = (cvr[3] & 0xFF) | (cvr[4] & 0x0F) << 8;
  cell_voltages[3] = (cvr[4] & 0xF0) >> 4 | (cvr[5] & 0xFF) << 4;
  cell_voltages[4] = (cvr[6] & 0xFF) | (cvr[7] & 0x0F) << 8;
  cell_voltages[5] = (cvr[7] & 0xF0) >> 4 | (cvr[8] & 0xFF) << 4;
  cell_voltages[6] = (cvr[9] & 0xFF) | (cvr[10] & 0x0F) << 8;
  cell_voltages[7] = (cvr[10] & 0xF0) >> 4 | (cvr[11] & 0xFF) << 4;
  cell_voltages[8] = (cvr[12] & 0xFF) | (cvr[13] & 0x0F) << 8;
  cell_voltages[9] = (cvr[13] & 0xF0) >> 4 | (cvr[14] & 0xFF) << 4;
  cell_voltages[10] = (cvr[15] & 0xFF) | (cvr[16] & 0x0F) << 8;
  cell_voltages[11] = (cvr[16] & 0xF0) >> 4 | (cvr[17] & 0xFF) << 4;
  
  for(int i=0;i<12;i++) {
    cell_voltages[i] = cell_voltages[i]*1.5*0.001;
  }
  printBpsData();
}  

void beginTemp() {
  LT_SPI_START;
  sendToSPI(STTMPAD);
  delay(15); //Time for conversions
  LT_SPI_END;
}

// Converts external thermistor voltage readings 
// into temperature (K) using B-value equation
void convertVoltTemp(short* volt, float* temp) {
  float resist[2];
  resist[0] = (10000 / ((3.11/(volt[0]*1.5*0.001))-1));
  resist[1] = (10000 / ((3.11/(volt[1]*1.5*0.001))-1));
  float rinf = 10000 * exp(-1*(B/298.15));
  temp[0] = (B/log(resist[0]/rinf)) - 273.15;
  temp[1] = (B/log(resist[1]/rinf)) - 273.15;
  temp[2] = (volt[2]*1.5*0.125) - 273;
}

//Reads temperatures
void readTemp(float* temperatures, byte board) {
  short raw_temperatures[3];
  // Start Temperature ADC conversions
  beginTemp();
  LT_SPI_START;
  sendToSPI(board); //board address is selected
  sendToSPI(RDTMP); //temperatures to be read
  byte tempr[5];
  getMultipleFromSPI(tempr, RDTMP, 5);
  LT_SPI_END;
  //convert temperature registers to temperatures
  raw_temperatures[0] = (tempr[0] & 0xFF) | (tempr[1] & 0x0F) << 8;
  raw_temperatures[1] = (tempr[1] & 0xF0) >> 4 | (tempr[2] & 0xFF) << 4;
  raw_temperatures[2] = (tempr[3] & 0xFF) | (tempr[4] & 0x0F) << 8;
  
  // Convert raw_temperatures into floats
  convertVoltTemp(raw_temperatures, temperatures);
}

// Sends out CAN packet
void sendCAN(int id, char*data, int size) {
  CanMessage msg = CanMessage();
  msg.id = id;
  msg.len = size;
  for(int i=0;i<size;i++) {
    msg.data[i] = data[i];
  }
  Can.send(msg);
}

void bpsSetup() {
  char init[1] = { 0x00 };
  sendCAN(0x041, init, 1);
  pinMode(B0,OUTPUT);
  
  // Turn on fans
  pinMode(FAN1,OUTPUT);
  pinMode(FAN2,OUTPUT);
  
  // Turns on SPI
  LT_SPI_END; 
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);
}
void printBpsData() {
  // Set number of cells correctly.  Module 2 has 11 cells.
  // As of March 2012, Module 0 also has 11 cells.
  // As of April 2012, Module 0 has 10 cells.
  int length = 11;
  if (can_current_module == 1) {
    length = 12;
  } else if (can_current_module == 0) {
    length = 10;
  }
  float* cell_voltages = battery_voltages[can_current_module];
  float* module_temperatures = temperatures[can_current_module];
  #ifdef VERBOSE
    Serial.print("Module ");
    Serial.println(can_current_module);
    
    // Iterate through all cells
    for(int i=0; i<length; i++) {
      Serial.print("Cell index ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(cell_voltages[i]);
    }
  #endif
  
  // Increment current working module, mod 3
  can_current_module++;
  if (can_current_module >= 3) {
    can_current_module = 0;
  }
}

/*** Sends all cell voltages and temperatures.  Sends 1 module worth of
 * data per call, alternates module on each call.
 */
void sendBpsData() {
  // Set number of cells correctly.  Module 2 has 11 cells.
  // As of March 2012, Module 0 also has 11 cells.
  // As of April 2012, Module 0 has 10 cells.
  int length = 11;
  if (can_current_module == 1) {
    length = 12;
  }
  if (can_current_module == 0) {
    length = 10;
  }
  float* cell_voltages = battery_voltages[can_current_module];
  float* module_temperatures = temperatures[can_current_module];
  int can_module_offset = CAN_BPS_BASE;
  can_module_offset += can_current_module * CAN_BPS_MODULE_OFFSET;
  
  // Iterate through all cells, send voltage measurements.
  for(int cell_number = 0; cell_number < length; cell_number++) {
    // Set up the correct ID w/ module and cell offsets.
    int id = can_module_offset + cell_number;
    two_floats voltage_data;
    voltage_data.f[0] = cell_voltages[cell_number];
    sendCAN(id, voltage_data.c, 4);
  }
  
  // Send temperature data
  for(int temp_number; temp_number < 3; temp_number++) {
    // Set up temperature can ID w/ module and sensor offsets.
    int id = can_module_offset + CAN_BPS_TEMP_OFFSET + temp_number;
    two_floats temperature_data;
    temperature_data.f[0] = module_temperatures[temp_number];
    sendCAN(id, temperature_data.c, 4);
  }
  
  
  // Increment current working module, mod 3
  can_current_module++;
  if (can_current_module >= 3) {
    can_current_module = 0;
  }
}

/***se
 * Queries all LT boards and update internal state.  Raise flags if needed.
 */
void bpsUpdate() {
  config[0] = 0xE5;
  writeConfig(config);
  
  // Iterate through all 3 LT boards
  for(int current_board=0; current_board < 3; current_board++) {
    float* current_board_voltages = battery_voltages[current_board];
    // Set number of cells to 10 for board 0, 12 for board 1, 11 for board 2
    int total_cells = 11;
    if(current_board == 1) {
      total_cells = 12;
    } else if (current_board == 0) {
      total_cells = 10;
    }
    
    // Write configuration into the board
    byte rconfig[6];
    readConfig(rconfig, board_address[current_board]);
    if(rconfig[0]==0xFF || rconfig[0]==0x00 || rconfig[0]==0x02) {
      Serial.print("Board not communicating: ");
      Serial.println(current_board);
      system_error = 1;
      continue;
    }

    // Read cell voltages
    beginCellVolt();
    readCellVolt(current_board_voltages, board_address[current_board]);
    
    // Iterate through all cells and run checks. Toggle error flag if needed.
    for (int current_cell = 0; current_cell < total_cells; current_cell++) {
      const float cell_voltage = current_board_voltages[current_cell];
      
      // Over voltage checks
      if (cell_voltage > OVERVOLTAGE_ERROR_LEVEL) {
        overvolt_error = 1;
      } else if (cell_voltage > OVERVOLTAGE_WARNING_LEVEL) {
        overvolt_warning = 1;
      }
      
      // Under voltage checks
      if (cell_voltage < UNDERVOLTAGE_ERROR_LEVEL) {
        undervolt_error = 1;
      } else if (cell_voltage < UNDERVOLTAGE_WARNING_LEVEL) {
        undervolt_warning = 1;
      }
    }

    // Read temperatures
    float * current_temperatures = temperatures[current_board];
    readTemp(current_temperatures, board_address[current_board]);
    
    // Iterate through sensors and check for over temperature
    for (int current_sensor = 0; current_sensor < 3; current_sensor++) {
      const float temperature = current_temperatures[current_sensor];
      if (temperature > OVERTEMP_ERROR_LEVEL) {
        overtemp_error = 1;
      } else if (temperature > OVERTEMP_WARNING_LEVEL) {
        overtemp_warning = 1;
      }
    }
  }
  #ifdef BPS_DEBUG
    Serial.print("OV: ");
    Serial.print(overvolt_warning);
    Serial.print(" ");
    Serial.println(overvolt_error);
    Serial.print("UV: ");
    Serial.print(undervolt_warning);
    Serial.print(" ");
    Serial.println(overvolt_error);
    Serial.print("OT: ");
    Serial.print(overtemp_warning);
    Serial.print(" ");
    Serial.println(overtemp_error);
  #endif
}

#endif
