/* CalSol - UC Berkeley Solar Vehicle Team 
 * pindef.h - Dashboard Module
 * Purpose: Pin definitions for the Dashboard IO module
 * Author(s): Michael Chang.  Ryan Tseng.
 * Date: Oct 03rd 2011
 */
 
// TODO: Add in indicator LEDs

#ifndef _DASHBOARD_PINDEF_H_
#define _DASHBOARD_PINDEF_H_
#include <WProgram.h>

/* PINOUTS */
// Light and Horn Switches
#define IN_HAZ_SWITCH 10
#define IN_HORN_BUTTON 11
#define IN_LTURN_SWITCH 18
#define IN_RTURN_SWITCH 17

// Light Indicators
#define OUT_LTURN_INDICATOR 24
#define OUT_RTURN_INDICATOR 23

// Cruise control inputs and outputs
#define IN_CRUISE_ON 16
#define IN_CRUISE_DEC 25
#define IN_CRUISE_ACC 26
#define OUT_CRUISE_INDICATOR 15

// Analog pedal in
#define ANALOG_ACCEL_PEDAL 4
#define ANALOG_BRAKE_PEDAL 3

// Drive Switches
#define IN_VEHICLE_FWD 19 
#define IN_VEHICLE_REV 21
#define IN_REGEN_SWITCH 20

// 12V Switched Outputs
#define OUT_LTURN 31
#define OUT_RTURN 0
#define OUT_BRAKELIGHT 1
#define OUT_CAMERA 2
#define OUT_HORN 30

/*** 
 * Sets pin direction and initial state.
 ***/
void initPins() {
  // Light and horn switches
  pinMode(IN_LTURN_SWITCH, INPUT);
  pinMode(IN_RTURN_SWITCH, INPUT);
  pinMode(IN_HAZ_SWITCH, INPUT);
  pinMode(IN_HORN_BUTTON, INPUT);
  digitalWrite(IN_LTURN_SWITCH, HIGH);
  digitalWrite(IN_RTURN_SWITCH, HIGH);
  digitalWrite(IN_HAZ_SWITCH, HIGH);
  digitalWrite(IN_HORN_BUTTON, HIGH);
  
  // Indicators
  pinMode(OUT_LTURN_INDICATOR, OUTPUT);
  pinMode(OUT_RTURN_INDICATOR, OUTPUT);
  pinMode(OUT_CRUISE_INDICATOR, OUTPUT);
  
  // Cruise controls
  pinMode(IN_CRUISE_ON, INPUT);
  pinMode(IN_CRUISE_DEC, INPUT);
  pinMode(IN_CRUISE_ACC, INPUT);
  digitalWrite(IN_CRUISE_ON, HIGH);
  digitalWrite(IN_CRUISE_DEC, HIGH);
  digitalWrite(IN_CRUISE_ACC, HIGH);
  
  // Driver controls
  pinMode(IN_VEHICLE_FWD, INPUT);
  pinMode(IN_VEHICLE_REV, INPUT);
  digitalWrite(IN_VEHICLE_REV, HIGH);
  digitalWrite(IN_VEHICLE_FWD, HIGH);
  
  // 12V output
  pinMode(OUT_LTURN, OUTPUT);
  pinMode(OUT_RTURN, OUTPUT);
  pinMode(OUT_HORN, OUTPUT);
  pinMode(OUT_BRAKELIGHT, OUTPUT);
  pinMode(OUT_CAMERA, OUTPUT);
  digitalWrite(OUT_LTURN, LOW);
  digitalWrite(OUT_RTURN, LOW);
  digitalWrite(OUT_HORN, LOW);
  digitalWrite(OUT_BRAKELIGHT, LOW);
  digitalWrite(OUT_CAMERA, HIGH);  // Forever on
}
#endif

