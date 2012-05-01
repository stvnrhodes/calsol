/* CalSol - UC Berkeley Solar Vehicle Team 
 * bms.h - BMS Module
 * Purpose: Define constants and functions for the BMS
 * Author(s): Steven Rhodes
 * Date: May 1, 2012
 */
 
#ifndef _BMS_H_
#define _BMS_H_
#include "bms.c"

/* Magic Numbers */
#define OVERTEMP_CUTOFF 60.0
#define TEMP_OK_TO_CHARGE 44.5
#define OVERTEMP_NO_CHARGE 45.0
#define OVERTEMP_WARNING 55.0
#define OVERVOLTAGE_CUTOFF 4.2
#define OVERVOLTAGE_WARNING 4.1
#define UNDERVOLTAGE_CUTOFF 2.5
#define UNDERVOLTAGE_WARNING 2.6

/* Pinouts */
// Relays
#define BATTERY_RELAY   18 // Check this!
#define CUTOFF_RELAY   19 // Check this
#define RELAY3   20  // No led
#define LVRELAY  22
// LEDs
#define LEDFAIL  23
#define CANINT    3
// Voltage/Current readings
#define C_GND        5 
#define C_BATTERIES  3
#define C_MOTOR      4
#define V_BATTERIES  1
#define V_MOTOR      2 
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
#define IO_T4         31   // Analog 0
// BPS
#define LT_CS         11





void initPins(void)

#endif
