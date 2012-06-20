/* CalSol - UC Berkeley Solar Vehicle Team 
 * Dashboard.pde - Dashboard Module
 * Purpose: Main code for the Dashboard IO module
 * Author(s): Ryan Tseng, Steven Rhodes.
 * Date: March 30 2012
 */
 
#include "pindef.h"
#include "can_id.h"
#include "dashboard.h"

// Enable for more information over serial
 #define VERBOSE
 //#define CAN_DEBUG
#define ERRORS

void setup() {
  Serial.begin(115200);
  Can.attach(&processCan);
  Can.begin(1000);
  CanBufferInit();
  initPins();
  updateDrivingState();  // Set car state (for/rev/neu) based on inputs.
  status = OKAY_STATUS;
  tritium_reset = 1;
}

void loop() {
  // Lights and horn control. Called every 10 ms.
  if (millis() - last_auxiliary_cycle > 10) {
    last_auxiliary_cycle = millis();
    updateAuxiliaryStates();
    auxiliaryControl();
  }
  
  // The following code blinks the error LED.  Not available 
  // during cruise control.
  if(!cruise_on) {
    blinkStatusLED();
  }
  
  // Reset overcurrent scale after 10 seconds if its below 1.0
  if (overcurrent_scale != 1.0 && millis() - time_of_last_oc > 10000)
    overcurrent_scale = 1.0;
  
  // Send Tritium resets if we get an overcurrent error
  if (tritium_reset) {
    tritium_reset--;
    resetTritium();
  }
  
  // Driver control.  Call state handler every 100 ms.
  if (millis() - last_sent_tritium > 100) {
    last_sent_tritium = millis();
    updateDrivingState();
    updateCruiseState();
   // if (status != ERROR_STATUS) {
      driverControl();
  //  }
  }
  
  // Debug and HeartBeat
  if (millis() - last_debug_cycle > 1000) {
    last_debug_cycle = millis();
    Can.send(CanMessage(CAN_HEART_DRIVER_IO));
    testPins();
    #ifdef VERBOSE
      Serial.print("accel: ");
      Serial.print(accel);
      Serial.print(", brake:");
      Serial.print(brake);
      Serial.print(", cruise  speed:");
      Serial.print(set_speed);
      Serial.print(", current speed:");
      Serial.println(current_speed);
      Serial.print("Cruise is ");
      Serial.print(cruise_on);
      Serial.print(", Driving mode is ");
      Serial.println(state);
    #endif
    char flags = right_state << 4 |
                 left_state << 3 |
                 horn_state << 2 |
                 brake_state << 1 |
                 (state == REVERSE);
    Can.send(CanMessage(CAN_DASHBOARD_INPUTS, &flags, 1));
  }
    
  #ifdef CAN_DEBUG
    Serial.print("Can RX: ");
    Serial.print(Can.rxError());
    Serial.print(" TX: ");
    Serial.println(Can.txError());
  #endif
  
}
