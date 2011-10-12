/* CalSol - UC Berkeley Solar Vehicle Team 
 * bms.pde - BMS Module
 * Purpose: Main code for the BMS module
 * Author(s): Ryan Tseng. Jimmy Hack.  Brian Duffy.
 * Date: Sept 25th 2011
 */

#define DEBUG_TEMP
#define CAN_DEBUG
//#define BPS_DEBUG

#include <EEPROM.h>
#include "pindef.h"
#include "bps.h"
#include "pitches.h"
#include "can_id.h"
#include "cutoff.h"
  
/***
 * This is the startup function that is called whenever the car resets
 * It handles reinitializing all variables and pins
 */
void initialize() {
  Serial.println("Initializing");
  initPins();
  initVariables();
  lastShutdownReason();  // Print out reason for last shutdown
  prepShutdownReason();  // Increment the position in shutdown log.
  /* Precharge */
  lastState = TURNOFF;  // Initialize in off state if key is off.
  if (checkOffSwitch()) {
    state = TURNOFF;
  } else {
    state = PRECHARGE; //boot up the car
  }
}

/***
 * The default setup function called by Arduino at startup 
 * This performs one-time setup of our communication protocols
 * and calls another function initialize()
 * to setup pins and variables
 */
void setup() {
  /* General init */
  Serial.begin(115200);
  Serial.println("Powering Up");  
  initBps();
  initialize(); //initialize pins and variables to begin precharge state.  
  //initCAN();
  Can.begin(1000);
}

/***
 * The default looping function called by Arduino after setup()
 * This program will loop continuously while the code is in execution.
 * We use a state machine to handle all of the different modes of operation
 */
void loop() {
  // Update internal battery voltage values and flags
  bpsUpdate();
  
  // Update internal 
  
  // Set errors and warnings
  warning = undervolt_warning;
  warning |= overvolt_warning << 1;
  warning |= overtemp_warning << 2;
  error = undervolt_error || overvolt_error || overtemp_error || system_error;
  
  // Reset warnings
  undervolt_warning = 0;
  overvolt_warning = 0;
  overtemp_warning = 0;

  // Perform state fuctions and update state */
  switch (state) {
    case PRECHARGE:
      do_precharge();
      break;
    case NORMAL:
      do_normal();        
      break;
    case TURNOFF:
      do_turnoff();
      break;
    case ERROR:
      do_error();
      break;
    default:
      #ifdef DEBUG
        Serial.println("Defaulted to error state.  "
                       "There must be a coding issue.");
      #endif
      do_error();
      break;
  }
  
  /* The code below cannot change the state */
  // Send CAN data once per second.  These should be off by 500 ms.
  if (millis() - last_send_cutoff_can > 1000) {
    last_send_cutoff_can = millis();
    sendCutoffCAN();
    #ifdef DEBUG_TEMP
      for (int i = 0; i < 3; i++) {
        Serial.print("Temp Board ");
        Serial.print(i);
        for (int j = 0; j < 3; j++) {
          Serial.print(" ");
          Serial.print(temperatures[i][j]);
        }
        Serial.println();
      }
    #endif
  }
  
  // Send BPS CAN data.  Only sends 1 module per second, switches module every second.
  if (millis() - last_send_bps_can > 1000) {
    last_send_bps_can = millis();
    sendBpsData();
    // Debug
    #ifdef CAN_DEBUG
      Serial.print("Can RX: ");
      Serial.print(Can.rxError());
      Serial.print(" TX: ");
      Serial.print(Can.txError());
      Serial.print(" Error Flags: ");
      Serial.println(Can._mcp2515.read(EFLG), HEX);
    #endif
  }
  
  // Accept serial inputs
  processSerial();
}
