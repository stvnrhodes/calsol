/* CalSol - UC Berkeley Solar Vehicle Team
 * pindef.h - BMS Module
 * Purpose: Define constants and functions for the BMS
 * Author(s): Steven Rhodes
 * Date: May 1 2012
 */

#ifndef BMS_PINDEF_H
#define BMS_PINDEF_H

/* Pinouts */
// Relays
#define BATTERY_RELAY  18 // Check this!
#define SOLAR_RELAY   19 // Check this!
#define LV_RELAY        22
// LEDs
#define LEDFAIL  23
#define CANINT    3
// Voltage/Current readings
#define C_GND        5
#define C_BATTERY    3
#define C_SOLAR      4
#define V_BATTERY    1
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
#define KEY_SWITCH   2
// BPS
#define LT_CS         11

/* Strings */

/* Shutdown Reason Error Codes */
/* these are stored in program memory to decrease SRAM usage */
/* otherwise we run out of RAM, the board acts strangely and resets */
/* lesson learned, don't do too many serial prints */
const prog_char reason_POWER_LOSS[]
    PROGMEM = "Sudden loss of power.";
const prog_char reason_KEY_OFF[]
    PROGMEM = "Normal Shutdown. Key in off position.";
const prog_char reason_S_UNDERVOLT[]
    PROGMEM = "High voltage line undervoltage.";
const prog_char reason_S_OVERVOLT[]
    PROGMEM = "High voltage line overvoltage.";
const prog_char reason_S_OVERCURRENT[]
    PROGMEM = "High voltage line overcurrent.";
const prog_char reason_BPS_DISCONNECTED[]
    PROGMEM = "BPS, LT Board Unresponsive";
const prog_char reason_BPS_UNDERVOLT[]
    PROGMEM = "BPS, Battery Module Undervoltage";
const prog_char reason_BPS_OVERVOLT[]
    PROGMEM = "BPS, Battery Module Overvoltage";
const prog_char reason_BPS_OVERTEMP[]
    PROGMEM = "BPS, Battery Module Overtemperature";
const prog_char reason_S_ENABLE_CHARGING[]
    PROGMEM = "Safe to charge, enabling charging";
const prog_char reason_S_DISABLE_CHARGING[]
    PROGMEM = "Unsafe to charge, disabling charging";
const prog_char reason_S_COMPLETED_PRECHARGE[]
    PROGMEM = "Unsafe to charge, disabling charging";
const prog_char reason_UNKNOWN[]
    PROGMEM = "Unknown Shutdown Code";

// Make sure that this number is larger than the size of the largest string.
#define MAX_STR_SIZE 40

// This number determines how many messages we're willing to log in the system.
#define NUM_OF_ERROR_MESSAGES 50

// an array containing all of our error codes
PROGMEM const char *error_code_lookup[] = {
  reason_POWER_LOSS,
  reason_KEY_OFF,
  reason_S_UNDERVOLT,
  reason_S_OVERVOLT,
  reason_S_OVERCURRENT,
  reason_BPS_DISCONNECTED,
  reason_BPS_UNDERVOLT,
  reason_BPS_OVERVOLT,
  reason_BPS_OVERTEMP,
  reason_S_ENABLE_CHARGING,
  reason_S_DISABLE_CHARGING,
  reason_S_COMPLETED_PRECHARGE,
  reason_UNKNOWN
};


// This enum matches with the array above
enum error_codes {
  POWER_LOSS,
  KEY_OFF,
  S_UNDERVOLT,
  S_OVERVOLT,
  S_OVERCURRENT,
  BPS_DISCONNECTED,
  BPS_UNDERVOLT,
  BPS_OVERVOLT,
  BPS_OVERTEMP,
  S_ENABLE_CHARGING,
  S_DISABLE_CHARGING,
  S_COMPLETED_PRECHARGE,
  UNKNOWN_SHUTDOWN // Must always be last one
};

#endif BMS_PINDEF_H
