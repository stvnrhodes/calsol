/*
 * File:   datalogger-perflogger.c
 * Author: Ducky
 *
 * Created on August 14, 2011, 11:18 AM
 */

#include <stdlib.h>
#include <string.h>

#include "../types.h"

#include "../ecan.h"
#include "../timing.h"

#include "../UserInterface/datalogger-ui-hardware.h"
#include "../UserInterface/datalogger-ui-leds.h"

#include "datalogger-stringutil.h"
#include "datalogger-file.h"
#include "datalogger-applications.h"

#define DEBUG_UART
#define DEBUG_UART_DATA
//#define DEBUG_UART_SPAM
#define DBG_MODULE "DLG/PerfLog"
#include "../debug-common.h"

StatisticalMeasurement Performance = {0,0,0,0};

void Datalogger_ProcessPerfLogger(DataloggerFile *dlgFile) {
	static uint16_t lastTime = 0;
	uint16_t currTime = GetbmsecOffset();
	uint16_t diffTime = 0;
	
	if (lastTime > currTime ) {
		static char buffer[64] = "PS xxxxxxxx LPTM ";
		uint8_t bufferPos = 17;

		uint16_t average = ((Performance.runningAverage/Performance.sampleCount)
				* 1015762) >> 18;
		uint16_t high = (((uint32_t)Performance.high) * 1015762) >> 18;
		uint16_t low = (((uint32_t)Performance.low) * 1015762) >> 18;
		DBG_SPAM_printf("Loop time: Low %u, Avg %u, High %u, Samp %u", low, average, high, Performance.sampleCount);

		Int32ToString(Get32bitTime(), buffer+3);

		itoa(buffer+bufferPos, Performance.sampleCount, 10);
		bufferPos += strlen(buffer+bufferPos);
		buffer[bufferPos] = ' ';	bufferPos++;
		itoa(buffer+bufferPos, low, 10);
		bufferPos += strlen(buffer+bufferPos);
		buffer[bufferPos] = ' ';	bufferPos++;
		itoa(buffer+bufferPos, average, 10);
		bufferPos += strlen(buffer+bufferPos);
		buffer[bufferPos] = ' ';	bufferPos++;
		itoa(buffer+bufferPos, high, 10);
		bufferPos += strlen(buffer+bufferPos);
		buffer[bufferPos] = '\n';	bufferPos++;
		buffer[bufferPos] = 0;

		DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)buffer, bufferPos);

		// Reset statistical counters
		Performance.sampleCount = 0;
		Performance.low = 65535;
		Performance.high = 0;
		Performance.runningAverage = 0;
	}

	if (currTime >= lastTime) {
		diffTime = currTime - lastTime;
	} else {
		diffTime = currTime + 1024 - lastTime;
	}
	Performance.sampleCount ++;
	Performance.runningAverage += diffTime;
	if (diffTime > Performance.high) {
		Performance.high = diffTime;
	}
	if (diffTime < Performance.low) {
		Performance.low = diffTime;
	}

	lastTime = currTime;
}
