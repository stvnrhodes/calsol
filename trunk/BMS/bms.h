/* CalSol - UC Berkeley Solar Vehicle Team
 * BatteryManagement.h - BMS Module
 * Purpose: Define constants and functions for the BMS
 * Author(s): Steven Rhodes
 * Date: May 1 2012
 * Overview of System: This code was rewritten for better readability and to
 * allow averaging of system data.  All functions are defined in bms.h with
 * substantial comments to reveal their purpose, then implemented in bms.pde.
 * The exception to this is buzzer songs, which are a C++ class defined in
 * pitches.h and implemented in pitches.c.
 */

#ifndef BMS_H
#define BMS_H
#include "pindef.h"
#include "can_id.h"
#include "pitches.h"

/* Constants */
#define NUM_OF_LT_BOARDS 3
#define NUM_OF_VOLTAGES 12
#define NUM_OF_TEMPERATURES 3
#define NUM_OF_AVERAGES 3

/* Magic Threshold Numbers */
#define OVERTEMP_CUTOFF 60.0  // in Celcius
#define OVERTEMP_WARNING 55.0  // in Celcius
#define CHARGING_OVERTEMP_CUTOFF 45.0  // in Celcius
#define CHARGING_OVERTEMP_WARNING 44.5  // in Celcius
#define OVERTEMPERATURE_NO_CHARGE 44.0  // in Celcius
#define TEMPERATURE_OK_TO_CHARGE 43.5  // in Celcius
#define MODULE_OVERVOLTAGE_CUTOFF 2800  // 4.2V, 1.5mV increments
#define MODULE_OVERVOLTAGE_WARNING 2760  // 4.15V, 1.5mV increments
#define MODULE_UNDERVOLTAGE_WARNING 1870  // 2.8V, 1.5mV increments
#define MODULE_UNDERVOLTAGE_CUTOFF 1800  // 2.7V, 1.5mV increments
#define VOLTAGE_OK_TO_CHARGE 2730  // 4.1V, 1.5mV increments
#define OVERVOLTAGE_NO_CHARGE 2760  // 4.15V, 1.5mV increments
#define OVERVOLTAGE_CUTOFF 13860  // 138.6V, in 10mV increments
#define OVERVOLTAGE_WARNING 13695  // 136.95V, in 10mV increments
#define UNDERVOLTAGE_WARNING 9240  // 92.4V, in 10mV increments
#define UNDERVOLTAGE_CUTOFF 8910  // 89.1V, in 10mV increments
#define DISCHARGING_OVERCURRENT_CUTOFF 4500  // 45A, in 10mA increments
#define CHARGING_OVERCURRENT_CUTOFF -4500  // -45A, in 10mA increments
#define CHARGING_THRESHOLD 100  // -1A, in 10mA increments
#define MOTOR_MINIMUM_VOLTAGE 9000 // 90V, in 10mV increments
#define MOTOR_MAXIMUM_DELTA 2000 // 20V, in 10mV increments
#define DISCHARGE_GAP 10  // 15mV, in 10mV increments

/* Magic Conversion Numbers */
#define LT_VOLT_TO_FLOAT 0.0015
#define LT_THIRD_TEMP_TO_FLOAT 0.1875  // Converts to Kelvin
#define THERM_B 3988
#define CELCIUS_KELVIN_BIAS 273.15
#define ROOM_TEMPERATURE 298.15
#define V_INF 3.075  // Voltage at 0 Kelvin
#define R_INF 0.00000155921// exp(-THERM_B / ROOM_TEMPERATURE), precalculated for speed
#define VOLTAGE_NUMERATOR 100000
#define VOLTAGE_DENOMINATOR 2958
#define CONVERT_THIRD_TO_CELCIUS(x) (x * LT_THIRD_TEMP_TO_FLOAT - CELCIUS_KELVIN_BIAS)
#define CONVERT_TO_MILLIAMPS(x) (x * 40)
#define CONVERT_TO_MILLIVOLTS(x) ((x * VOLTAGE_NUMERATOR) / (VOLTAGE_DENOMINATOR)) // 10mV/unit

/* Times (in ms) for how often to do actions */
#define DATA_TIME_LENGTH 200
#define BATTERY_TIME_LENGTH 100
#define HEARTBEAT_TIME_LENGTH 250
#define SONG_TIME_LENGTH 25

/* LT Boards */
const char kLTNumOfCells[] = {10, 12, 11};
const byte kBoardAddress[] = {0x80, 0x81, 0x82};
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
#define LT_UNDER_VOLTAGE 0x71  // 2.712V
#define LT_OVER_VOLTAGE 0xAB  // 4.1V
#define CONFIG_ARG_NUM 6

// This holds the configuration values for the LT boards, and controls discharging.
byte lt_config[NUM_OF_LT_BOARDS][CONFIG_ARG_NUM];

/* Data structures */

// 8 chars and 2 floats sharing the same space. Used for converting two floats
// into a format that we can use to send CAN data.
typedef union {
  char  c[8];
  float f[2];
} TwoFloats;

// This struct is used to store the information from the LTC6802/LTC6803 chips
typedef struct {
  int voltage[NUM_OF_VOLTAGES];
  int temperature[NUM_OF_TEMPERATURES];
  int is_valid;
} LTData;

typedef struct {
  LTData data[NUM_OF_AVERAGES];
  char ptr;
} LTMultipleData;

typedef struct {
  LTData lt_board[3];
  signed int battery_voltage;
  signed int motor_voltage;
  signed int battery_current;
  signed int solar_current;
} CarDataInt;

typedef struct {
  float battery_voltage;
  float motor_voltage;
  float battery_current;
  float solar_current;
} CarDataFloat;

typedef struct {
  byte battery_overvoltage: 1;
  byte battery_overvoltage_warning: 1;
  byte battery_undervoltage: 1;
  byte battery_undervoltage_warning: 1;
  byte module_overvoltage: 1;
  byte module_overvoltage_warning: 1;
  byte module_undervoltage: 1;
  byte module_undervoltage_warning: 1;
  byte battery_overtemperature: 1;
  byte battery_overtemperature_warning: 1;
  byte charging_overtemperature: 1;
  byte charging_temperature_warning: 1;
  byte discharging_overcurrent: 1;
  byte charging_overcurrent: 1;
  byte too_hot_to_charge: 1;
  byte too_full_to_charge: 1;
  byte keyswitch_on: 1;
  byte batteries_charging: 1;
  byte missing_lt_communication: 1;
  byte motor_precharged: 1;
  // These two flags are controlled in loop()
  byte charging_disabled_too_hot: 1;
  byte charging_disabled_too_full: 1;
} Flags;

typedef enum {
  TURN_OFF,
  TURN_ON,
  DISABLE_CHARGING,
  ENABLE_CHARGING,
  CAR_ON,
  CAR_OFF,
  EMERGENCY_SHUTOFF,
  IN_PRECHARGE,
} CarState;

/* Functions */
void InitPins(void);
void InitSpi(void);
void InitLtBoardData(void);
void SendHeartbeat(void);
void GetCarData(CarDataInt *);
void GetFlags(Flags *, const CarDataInt *);
CarState GetCarState(const CarState, const Flags *);
int IsCriticalError(const Flags *flags);
void ConvertCarData(CarDataFloat *, const CarDataInt *);
void DisableCharging (void);
void EnableCharging (void);
signed int GetBatteryVoltage(void);
signed int GetMotorVoltage(void);
signed int GetBatteryCurrent(void);
signed int GetSolarCellCurrent(void);
signed int GetIntMedian(const signed int *);
void GetLtDataMedian(LTData *, const LTMultipleData *);
void AddReading(LTMultipleData *, const LTData *);
void GetLtBoardData(LTData *, byte, LTMultipleData *);
void SendLtBoardCanMessage(const LTData *);
void SendGeneralDataCanMessage(const CarDataFloat *);
void SendErrorCanMessage(const CarState, const Flags *);
void ShutdownCar(void);
void TurnOnCar(void);
void SendHeartbeat(void);
void SendToSpi(const byte *, int);
void GetFromSpi(byte *, byte, int);
void ParseSpiData(LTData *, const byte *, const byte *);
void PrintErrorMessage(enum error_codes);
int HighestVoltage(const LTData *);
int LowestVoltage(const LTData *);
float HighestTemperature(const LTData *);
float ToTemperature(int);
inline void SpiStart(void) {
  digitalWrite(LT_CS, LOW);
}
inline void SpiEnd(void) {
  digitalWrite(LT_CS, HIGH);
}

/** Set the config variable to discharge batteries **/
void BalanceBatteries(const LTData *);

#endif  // BMS_H
