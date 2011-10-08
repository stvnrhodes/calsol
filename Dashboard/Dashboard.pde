/* CalSol - UC Berkeley Solar Vehicle Team 
 * Dashboard.pde - Dashboard Module
 * Purpose: Main code for the Dashboard IO module
 * Author(s): Ryan Tseng.
 * Date: Oct 3rd 2011
 */
 
#include "pindef.h"
#include "can_id.h"
#include "dashboard.h"

void setup() {
  Serial.begin(115200);
  Can.begin(1000);
  Can.attach(&processCan);
  CanBufferInit();
  initPins();
  updateDrivingState();  // Set car state (for/rev/neu) based on inputs.
  status = OKAY_STATUS;
}

void loop() {
  // Lights and horn control. Called every 10 ms.
  if (millis() - last_auxiliary_cycle > 10) {
    last_auxiliary_cycle = millis();
    updateAuxiliaryStates();
    auxiliaryControl();
  }
  
  // Health indicator.  If OK blink at 1Hz, otherwise blink at 5Hz.
  if (millis() - last_updated_speed > 1200) {
    status = ERROR_STATUS;
  }
  if (status == OKAY_STATUS) {
    if (millis() - last_status_blink > 1000) {
      last_status_blink = millis();
      digitalWrite(OUT_BRAIN_LED, !digitalRead(OUT_BRAIN_LED));
    }
  } else {
    if (millis() - last_status_blink > 200) {
      last_status_blink = millis();
      digitalWrite(OUT_BRAIN_LED, !digitalRead(OUT_BRAIN_LED));
    }
  }
  
  // Driver control.  Call state handler every 100 ms.
  if (millis() - last_sent_tritium > 100) {
    last_sent_tritium = millis();
    updateDrivingState();
    driverControl();
    testPins();
  }
}
