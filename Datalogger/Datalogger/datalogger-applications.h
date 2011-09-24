/*
 * File:   datalogger-applications.c
 * Author: Ducky
 *
 * Created on August 13, 2011, 4:45 PM
 *
 * Revision History
 * Date			Author	Change
 * 13 Aug 2011	Ducky	Initial implementation.
 *
 * @file
 * Datalogger applications, like recording CAN and the like.
 */

#ifndef DATALOGGER_APPLICATION_H
#define DATALOGGER_APPLICATION_H

#include "../types.h"

#include "datalogger-file.h"

typedef struct {
	uint16_t sampleCount;
	uint16_t low;
	uint16_t high;
	uint32_t runningAverage;
} StatisticalMeasurement;

/**
 * Processes the auto-termination feature.
 *
 * @return Whether to terminate or not. This will usually return 0, but on
 * a termination request, will return 1 ONCE.
 * @retval 0 Do not terminate.
 * @retval 1 Terminate.
 */
uint8_t Datalogger_ProcessAutoTerminate();

/**
 * Processes CAN messages, writing the received messages to the file.
 * @param file Datalogger file to write to.
 */
void Datalogger_ProcessCANMessages(DataloggerFile *dlgFile);

/**
 * Processes CAN communications, such as heartbeat transmission.
 */
void Datalogger_ProcessCANCommunications(DataloggerFile *dlgFile);

/**
 * Initializes the voltage recorder stuff.
 */
void Datalogger_InitVoltageRecorder();

/**
 * Does voltage recording functions, such as calculating statistical values
 * or recording data to disk.
 * @param dlgFile Datalogger file to write to.
 */
void Datalogger_ProcessVoltageRecorder(DataloggerFile *dlgFile);

/**
 * Does performance recording functions, such as logging statistical value
 * of cycle time to disk.
 * @param dlgFile Datalogger file to write to.
 */
void Datalogger_ProcessPerfLogger(DataloggerFile *dlgFile);

#endif
