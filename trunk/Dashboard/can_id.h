/* CalSol - UC Berkeley Solar Vehicle Team 
 * can_id.h - All Modules
 * Purpose: Can ID Definitions
 * Author(s): Ryan Tseng. Brian Duffy.
 * Date: Jun 18th 2011
 */

#ifndef _CAN_ID_H_
#define _CAN_ID_H_

// Emergency signals
#define CAN_EMER_BPS            0x021
#define CAN_EMER_CUTOFF         0x022
#define CAN_EMER_DRIVER_IO      0x023 
#define CAN_EMER_DRIVER_CTL     0x024 
#define CAN_EMER_TELEMETRY      0x025
#define CAN_EMER_OTHER1         0x026 
#define CAN_EMER_OTHER2         0x027 
#define CAN_EMER_OTHER3         0x028 

// Heartbeats
#define CAN_HEART_BPS           0x041 
#define CAN_HEART_CUTOFF        0x042 
#define CAN_HEART_DRIVER_IO     0x043 
#define CAN_HEART_DRIVER_CTL    0x044 
#define CAN_HEART_TELEMETRY     0x045 
#define CAN_HEART_DATALOGGER    0x046 
#define CAN_HEART_OTHER2        0x047 
#define CAN_HEART_OTHER3        0x048 

// Cutoff signals
#define CAN_CUTOFF_VOLT    0x523 
#define CAN_CUTOFF_CURR    0x524 
#define CAN_CUTOFF_NORMAL_SHUTDOWN 0x521

// BPS signals
#define CAN_BPS_BASE            0x100  // BPS signal base
#define CAN_BPS_MODULE_OFFSET   0x010  // Difference between modules
#define CAN_BPS_TEMP_OFFSET     0x00C  // Offset in addition to module offset
#define CAN_BPS_DIE_TEMP_OFFSET 0x00C  // Offset for LT die temperature

// To Tritium signals
#define CAN_TRITIUM_DRIVE       0x501

// From Tritium signals
#define CAN_TRITIUM_ID          0x400
#define CAN_TRITIUM_STATUS      0x401
#define CAN_TRITIUM_BUS         0x402
#define CAN_TRITIUM_VELOCITY    0x403
#define CAN_TRITIUM_PHASE_CURR  0x404
#define CAN_TRITIUM_MOTOR_VOLT  0x405
#define CAN_TRITIUM_MOTOR_CURR  0x406
#define CAN_TRITIUM_MOTOR_BEMF  0x407
#define CAN_TRITIUM_15V_RAIL    0x408
#define CAN_TRITIUM_LV_RAIL     0x409
#define CAN_TRITIUM_FAN_SPEED   0x40A
#define CAN_TRITIUM_MOTOR_TEMP  0x40B
#define CAN_TRITIUM_AIR_TEMP    0x40C
#define CAN_TRITIUM_CAP_TEMP    0x40D
#define CAN_TRITIUM_ODOMETER    0x40E

#endif
