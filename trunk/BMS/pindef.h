/* CalSol - UC Berkeley Solar Vehicle Team 
 * bmsPindef.h - BMS Module
 * Purpose: Pin Definitions for the BMS module
 * Author(s): Jimmy Hack, Ryan Tseng, Brian Duffy
 * Date: Sept 25th 2011
 */

#ifndef _BMS_PINDEF_H_
#define _BMS_PINDEF_H_
#include <WProgram.h>

/* PINOUTS */
// Relays
#define RELAY1   18
#define RELAY2   19
#define RELAY3   20  // No led
#define LVRELAY  22
// LEDs
#define LEDFAIL  23
#define CANINT    3
// Voltage/Current readings
#define CGND     5 
#define C1       3 // Batteries
#define C2       4 // Motor
#define V1       1 // Batteries
#define V2       2 // Motor
// Battery Box Fans
#define FAN1     24
#define FAN2     25
// Power Distribution
#define POWER_U10 12
#define POWER_U9  13
#define POWER_U8  14
#define POWER_U3  15
// I/O
#define OUT_BUZZER    21
#define IN_OFFSWITCH   2   // OFF SWITCH
#define IN_SONG1       1   // SONG 1 (Tetris)
#define IN_SONG2       0   // SONG 2 (Bad Romance)
#define IO_T4         31   // Analog 0
// BPS
#define LT_CS         11

// This is used to convert floats to char arrays.
typedef union {
  char c[8];
  float f[2];
} two_floats;

/*** 
 * Sets pin direction and initial state.
 ***/
void initPins() {
  // Buzzer
  pinMode(OUT_BUZZER, OUTPUT);
  digitalWrite(OUT_BUZZER, LOW);
  
  // Relays
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(LVRELAY, OUTPUT);
  
  // LEDs
  pinMode(LEDFAIL, OUTPUT);
  digitalWrite(LEDFAIL, LOW);
  
  // Off Switch
  pinMode(IN_OFFSWITCH, INPUT); // OFF SWITCH
  digitalWrite(IN_OFFSWITCH, HIGH);
  
  // Songs
  pinMode(IN_SONG1, INPUT); // Song 1
  digitalWrite(IN_SONG1, HIGH);
  pinMode(IN_SONG2, INPUT); // Song 
  digitalWrite(IN_SONG2, HIGH);
  
  // Spare IO pin
  pinMode(IO_T4, OUTPUT);
  
  // Voltage measurements
  pinMode(V1, INPUT);
  digitalWrite(V1, HIGH);
  pinMode(V2, INPUT);
  digitalWrite(V2, HIGH);
  
  // BPS SPI
  pinMode(LT_CS, OUTPUT);
  digitalWrite(LT_CS, HIGH);
  
  // Fans
  pinMode(FAN1, OUTPUT);
  digitalWrite(FAN1, HIGH);
  pinMode(FAN2, OUTPUT);
  digitalWrite(FAN2, HIGH);
}
#endif
