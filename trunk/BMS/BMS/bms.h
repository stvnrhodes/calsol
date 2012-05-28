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
#define OVERTEMP_CUTOFF 60.0
#define OVERTEMP_WARNING 55.0
#define TEMPERATURE_OK_TO_CHARGE 43.5
#define OVERTEMPERATURE_NO_CHARGE 44.0
#define CHARGING_OVERTEMP_CUTOFF 45.0
#define CHARGING_OVERTEMP_WARNING 44.5
#define MODULE_OVERVOLTAGE_CUTOFF 4.2
#define MODULE_OVERVOLTAGE_WARNING 4.1
#define MODULE_UNDERVOLTAGE_CUTOFF 2.5
#define MODULE_UNDERVOLTAGE_WARNING 2.6
#define OVERVOLTAGE_CUTOFF 4.2
#define OVERVOLTAGE_WARNING 4.1
#define UNDERVOLTAGE_CUTOFF 2.5
#define UNDERVOLTAGE_WARNING 2.6
#define OVERCURRENT_CUTOFF 100
#define CHARGING_OVERCURRENT_CUTOFF -45000
#define MOTOR_MINIMUM_VOLTAGE 90
#define MOTOR_MAXIMUM_DELTA 10

/* Magic Conversion Numbers */
#define LT_VOLT_TO_FLOAT 0.0015
#define LT_THIRD_TEMP_TO_FLOAT 0.1875// Converts to Kelvin
#define THERM_B 3988
#define CELCIUS_KELVIN_BIAS 273.15
#define ROOM_TEMPERATURE 298.15
#define V_INF 3.11  // Voltage at 0 Kelvin
#define ONE_MEG_RESISTOR 1002958
#define THREE_K_RESISTOR 2958
#define CONVERT_TO_MILLIAMPS(x) 
#define CONVERT_TO_MILLIVOLTS(x) ((x * ONE_MEG_RESISTOR) / THREE_K_RESISTOR) 

/* Times (in ms) for how often to do actions */
#define DATA_TIME_LENGTH 200
#define BATTERY_TIME_LENGTH 100
#define HEARTBEAT_TIME_LENGTH 250
#define SONG_TIME_LENGTH 25

/* LT Boards */
const char kLTNumOfCells[] = {10, 12, 11};  // Check this!
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
const byte kConfig[] = {0xE5,0x00,0x00,0x00,
                         LT_UNDER_VOLTAGE,LT_OVER_VOLTAGE};

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
  byte charging_disabled: 1;
} Flags;

typedef enum {
  TURN_OFF,
  TURN_ON,
  DISABLE_CHARGING,
  ENABLE_CHARGING,
  CAR_ON,
  CAR_OFF,
  IN_PRECHARGE,
} CarState;

/* Functions */

/***
 * Called at the beginning of the code and sets pins to the appropriate input
 * and output states.
 */
void InitPins(void);

/***
 * Called at the beginning of the code and sets SPI to the appropriate state
 */
void InitSpi(void);

/***
 * Initializes the values of LT_Multiple_Data.
 */
void InitLtBoardData(void);

/***
 * Send the BMS heartbeat (i.e. both BPS and Cutoff).
 */
void SendHeartbeat(void);

/***
 * Updates the car data with fresh values from the sensors and the LT boards
 * Call this function once every 200ms or so.
 *
 * @param CarDataInt *car_data The data to be updated.
 */
void GetCarData(CarDataInt *car_data);

/***
 * Checks to see if we've hit any limits that we should flag.
 *
 * @param Flags *flags The flags we care about
 * @param CarDataInt *car_data The data from the car.
 */
void GetFlags(Flags *flags, const CarDataInt *car_data);

/***
 * Checks what mode we're in based on the flags.
 *
 * @param Flags *car_flags The flags about the car
 * @return CarState An enum specifying what state we're in
 */
CarState GetCarState(const Flags *flags);

/***
 * Does the appropriate conversions to change CarDataInt into CarDataFloat
 *
 * @param CarDataFloat *out The output data
 * @param CarDataInt *in The input data
 */
void ConvertCarData(CarDataFloat *out, const CarDataInt *in);

/***
 * This function will both turn off the contactor for the solar cells and send
 * out a message that will hopefully be used to prevent regen braking.
 */
void DisableCharging (void);

/***
 * This function will turn on the solar cell contactor and broadcast a message
 * that it's okay to use regen braking.
 */
void EnableCharging (void);

/***
 * Get the battery voltage.
 *
 * @return int The battery voltage.
 */
signed int GetBatteryVoltage(void);

/***
 * Get the motor voltage.
 *
 * @return int The motor voltage.
 */
signed int GetMotorVoltage(void);

/***
 * Get the battery current.
 *
 * @return int The battery current.
 */
signed int GetBatteryCurrent(void);

/***
 * Get the current from the solar cells.
 *
 * @return int The solar cell current.
 */
signed int GetSolarCellCurrent(void);

/***
 * Get the middle value from an array of ints
 *
 * This only works for arrays of size 3 for now.  A future change would hopefully make it work for
 * arrays of size NUM_OF_AVERAGES
 *
 * @param int array[] Array to parse
 * @return Middle value from array
 */
signed int GetIntMedian(const signed int * array);

/***
 * Get the middle values from a struct of LT data
 *
 * This function relies on GetMedian, so it only works for 3 averages until the
 * other function is updated.
 *
 * @param LTData with all median values
 * @param LTMultipleData * array Struct to parse
 */
void GetLtDataMedian(LTData median, const LTMultipleData * array);

/***
 * Add a LTData to a LTMultipleData.
 *
 * @param LTMultipleData mult The struct we add the data to.
 * @param LTData reading The reading we want to add.
 */
void AddReading(LTMultipleData * mult, const LTData * reading);

/***
 * Fetches the data from a LT Module
 *
 * @param LT_Data *new_data The data fetched from the LT Board (grabbing the median)
 * @param char board_num Which board to read from
 * @param LT_Multiple_Data * averaging_data The data used to figure out the
 * rolling median
 */
void GetLtBoardData(LTData *new_data, byte board_num,
    const LTMultipleData * avg_data);

/***
 * Sends a CAN Message with LT data.
 *
 * @param LT_Data * data The data to use for the CAN Message
 */
void SendLtBoardCanMessage(const LTData * data);

/***
 * Sends a CAN Message with general car data.
 *
 * @param CarDataFloat * data The data to use for the CAN Message
 * @return CanMessage The message to send to CAN
 */
void SendGeneralDataCanMessage(const CarDataFloat * data);

/***
 * Sends a CAN Message to send out any errors.
 *
 * @param Flags *flags The flags to use for the CAN Message
 */
void SendErrorCanMessage(Flags *flags);

/***
 * Shuts down car.
 */
void ShutdownCar(void);

/***
 * Turns on car so that we can drive around
 */
void TurnOnCar(void);

/***
 * Sends a heartbeat CAN Message
 */
void SendHeartbeat(void);

/** Used to start talking over SPI. **/
inline void SpiStart(void) {
  digitalWrite(LT_CS, LOW);
}

/** Used to end talking over SPI. **/
inline void SpiEnd(void) {
  digitalWrite(LT_CS, HIGH);
}

/***
 * Send data over SPI
 *
 * @param byte * data Data to send over SPI
 * @param int length The length of the message we want to send
 */
void SendToSpi(const byte * data, int length);

/***
 * Get data over SPI
 *
 * @param byte * data Area to store data received
 * @param byte info Byte to send when requesting data
 * The length of the message we want to recieve
 */
void GetFromSpi(byte * data, byte info, int length);

/***
 * Turn the data from the LT Boards into a readable form
 *
 * @param LTData *data The place to store the data
 * @param byte voltages[] The voltages fetched from SPI
 * @param byte temperatures[] The temperatures fetchd from SPI
 */
void ParseSPIData(LTData *data, const byte voltages[], 
    const byte temperatures[]);

/***
 * Read off the PROGMEM to determine what string to print.
 *
 * @param enum error_codes code The code to interpret as a message.
 */
void PrintErrorMessage(enum error_codes code);

/**
 * Picks out the highest voltage from the data by iterating through all of it.
 *
 * @param LTData *board The boards we're inspecting
 * @return int The highest voltage that we find.
 */
int HighestVoltage(const LTData *board);

/**
 * Picks out the lowest voltage from the data by iterating through all of it.
 *
 * @param LTData *board The boards we're inspecting
 * @return int The lowest voltage that we find.
 */
int LowestVoltage(const LTData *board);

/**
 * Picks out the highest temperature from the data by iterating through all of it.
 *
 * @param LTData *board The boards we're inspecting
 * @return int The highest temperature that we find.
 */
float HighestTemperature(const LTData *board);

/**
 * Uses the B equation for thermistors to be able to get the temperature data
 * from the first two readings of the LT Boards.
 *
 * @param int temp The temperature as an integer
 * @return float The temperature as a float
 */
float ToTemperature(int temp);

/** Used for retrieving error code on LTC603 chips **/
byte GetPEC(const byte *din, int n);


#endif  // BMS_H
