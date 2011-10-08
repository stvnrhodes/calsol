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
  CanBufferInit();
  initPins();
  updateDrivingState();  // Set car state (for/rev/neu) based on inputs.
}

void loop() {
  // Lights and horn control
  updateAuxiliaryStates();
  auxiliaryControl();
  
  // Update speed values from Tritium
  // TODO: Write this code
  
  // Driver control.  Call state handler every 100 ms.
  if (millis() - last_sent_tritium > 100) {
    last_sent_tritium = millis();
    updateDrivingState();
    driverControl();
  }
}
