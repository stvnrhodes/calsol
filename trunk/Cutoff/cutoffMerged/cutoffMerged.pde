/* CalSol - UC Berkeley Solar Vehicle Team 
 * CutoffBasic.pde - Cutoff Module
 * Author(s): Jimmy Hack, Ryan Tseng, Brian Duffy
 * Date: Aug 3rd 2011
 */

#define DEBUG_CAN
#define DEBUG_MEASUREMENTS
#define DEBUG
//#define DEBUG_STATES

#include <EEPROM.h>
#include "cutoffHelper.h"
#include "cutoffCanID.h"
#include "cutoffPindef.h"
  
/*  This function is called via interrupt whenever a CAN message is received. */
/*  It identifies the CAN message by ID and processes it to take the appropriate action */
void process_packet(CanMessage &msg) {
  last_can = millis(); //calls to timers while in an interrupt function are not recommended, and can give some funny results.
  switch(msg.id) {     
    /* Add cases for each CAN message ID to be received*/
    
    /* Heartbeats */
    /* to optimize execution time, ordered in frequency they are likely to occur */
    case CAN_HEART_BPS:
      last_heart_bps = last_can;
      numHeartbeats++;
      warning = msg.data[0];  
      //if warning = 0 normal heartbeat   
      if (warning== 0x01){                 
          /* BPS Undervolt Warning flag */
          warning = 1; 
      }  
      else if(warning== 0x02){
          /* BPS Overvoltage Error */
          warning = 2;
      }
      else if (warning == 0x03){
          /* BPS Temperature Error */
          warning = 3;
      }
      else if (warning == 0x04){
          /* BPS Critical error flag */
          warning =0;
          shutdownReason=BPS_ERROR;
          emergency = 1;
      }
      break;
    case CAN_HEART_DRIVER_IO:
      last_heart_driver_io = last_can;
      break;
    case CAN_HEART_DRIVER_CTL:
      last_heart_driver_ctl = last_can;
      break;
    case CAN_HEART_TELEMETRY:
      last_heart_telemetry = last_can;
      break;
    case CAN_HEART_DATALOGGER:
      last_heart_datalogger = last_can;
      break;    
    
    /* Emergencies */
    // Emergency messages recieved from any board will cause the car to shut down.
    case CAN_EMER_BPS:
      bps_code = msg.data[0];
      emergency = 1;      
      shutdownReason=BPS_EMERGENCY_OTHER;  //will shut down, then specify type of BPS ERRO     
      break; 
    case CAN_EMER_DRIVER_IO:
      emergency = 1;
      shutdownReason=IO_EMERGENCY;
      break;
    case CAN_EMER_DRIVER_CTL:
      emergency = 1;
      shutdownReason=CONTROLS_EMERGENCY;
      break;  
    case CAN_EMER_TELEMETRY:
      emergency = 1;
      shutdownReason=TELEMETRY_EMERGENCY;
      break;      
    case CAN_EMER_OTHER1:
      emergency = 1;
      shutdownReason=OTHER1_EMERGENCY;
      break;
    case CAN_EMER_OTHER2:
      emergency = 1;
      shutdownReason=OTHER2_EMERGENCY;
      break;
    case CAN_EMER_OTHER3:
      emergency = 1;
      shutdownReason=OTHER3_EMERGENCY;
      break; 
    default:
      break;
  }
}

/* This is the startup function that is called whenever the car resets */
/* It handles reinitializing all variables and pins*/
void initialize(){
  Serial.println("Initializing");
  initPins();           //set all pins to their default configurations and values
  initVariables();      //set all variables to their default values
  lastShutdownReason(); //print out reason for last shutdown
  prepShutdownReason(); //increment the position in shutdown log.
  /* Precharge */
  lastState=TURNOFF;  //last state was off
  if (checkOffSwitch()){ //initialize in off state if key is off.
    state=TURNOFF;
  }
  else{
    state = PRECHARGE; //boot up the car
  }
}

/* The default setup function called by Arduino at startup 
/* This performs one-time setup of our communication protocols
/* and calls another function initialize()
/* to setup pins and variables */
void setup() {
  /* General init */
  Serial.begin(115200);
  Serial.println("Powering Up");  
  initialize(); //initialize pins and variables to begin precharge state.  
  initCAN(); //initialize CAN configurations
}


/* The default looping function called by Arduino after setup()
/* This program will loop continuously while the code is in execution.
/* We use a state machine to handle all of the different modes of operation */
void loop() {
  /* Perform state fuctions and update state */
    switch (state) {
      case PRECHARGE: //this is the startup state of the car
        do_precharge();
        break;
      case NORMAL: //this is the normal operation state of the car
        do_normal();        
        break;
      case TURNOFF: //this is the standard shutdown state which will allow the car to turn back on
        do_turnoff();
        break;
      case ERROR: //this is the error shutdown state which will not permit the car to turn back on
        do_error();
        break;
      default:  //the default state should not be used unless there is a coding error
        #ifdef DEBUG
          Serial.println("Defaulted to error state.   There must be a coding issue.");
        #endif
        do_error();
        break;
    }
  
  /*This code cannot change the state */
  sendCutoffCAN();  //send out heartbeats and measurements at the appropriate intervals 
  processSerial();  //accept serial inputs

}




