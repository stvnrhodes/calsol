/*
 * File:   datalogger-canrecorder.c
 * Author: Ducky
 *
 * Created on August 13, 2011, 4:47 PM
 *
 * Revision History
 * Date			Author	Change
 * 13 Aug 2011	Ducky	Initial implementation.
 *
 * @file
 * Datalogger CAN recorder and CAN communications routines.
 */

#include "../types.h"

#include "../ecan.h"
#include "../timing.h"

#include "../UserInterface/datalogger-ui-hardware.h"
#include "../UserInterface/datalogger-ui-leds.h"

#include "datalogger-stringutil.h"
#include "datalogger-file.h"

#define DEBUG_UART
#define DEBUG_UART_DATA
//#define DEBUG_UART_SPAM
#define DBG_MODULE "DLG/CAN"
#include "../debug-common.h"

//#define DATALOGGER_CAN_UART

void Datalogger_ProcessCANMessages(DataloggerFile *dlgFile) {
	static uint8_t canOverflow = 0;
	static uint8_t msgOverflow = 0;
	static uint32_t lastTime = 0;
	static char buffer[50] = "CM xxxxxxxx/xx 0 00 x xxx xx xx xx xx xx xx xx xx\n";
	static char bufferMOvf[20] = "CM xxxxxxxx/xx MOVF\n";
	static char bufferCOvf[20] = "CM xxxxxxxx/xx COVF\n";

	int8_t nextBuf;

	while ((nextBuf = ECAN_GetNextRXBuffer()) != -1) {
		uint16_t sid;
		uint32_t eid;
		uint8_t dlc;
		uint8_t data[8];
		uint8_t i;
		uint32_t currTime = Get32bitTime();
		uint32_t diffTime = currTime - lastTime;
		if (diffTime > 255) {
			diffTime = 255;
		}

		// TODO: FIX OVERFLOW CHECKS
		// Check overflow
		if ((C1RXOVF1 != 0) || (C1RXOVF2 != 0)) {
			C1RXOVF1 = 0;
			C1RXOVF2 = 0;
			UI_LED_Pulse(&UI_LED_CAN_Error);
			canOverflow = 1;
		}
		if (canOverflow) {
			Int32ToString(currTime, bufferCOvf+3);
			Int8ToString((uint8_t)diffTime, bufferCOvf+12);
			if (!DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)bufferCOvf, 20)) {
				msgOverflow = 1;
				UI_LED_Pulse(&UI_LED_SD_Error);
			}
			canOverflow = 0;
		}
		if (msgOverflow) {
			Int32ToString(currTime, bufferMOvf+3);
			Int8ToString((uint8_t)diffTime, bufferMOvf+12);
			if (!DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)bufferMOvf, 20)) {
				UI_LED_Pulse(&UI_LED_SD_Error);
			}
			msgOverflow = 0;
		}

		// Generate message timestamp
		Int32ToString(currTime, buffer+3);
		Int8ToString((uint8_t)diffTime, buffer+12);

		// Read message
		dlc = ECAN_ReadBuffer(nextBuf, &sid, &eid, 8, data);

		// Generate message contents
		Int4ToString(dlc, buffer+20);
		Int12ToString(sid, buffer+22);

		for (i=0;i<dlc;i++) {
			Int8ToString(data[i], buffer+26+i*3);
			buffer[28+i*3] = ',';
		}
		buffer[25+dlc*3] = '\n';

		if (!msgOverflow
				&& !DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)buffer, 26+dlc*3)) {
			msgOverflow = 1;
		}

#ifdef DATALOGGER_CAN_UART
		UART_DMA_WriteBlocking(buffer, 26+dlc*3);
#endif

		// User interface stuff
		UI_LED_Pulse(&UI_LED_CAN_RX);
	}
	lastTime = Get32bitTime();
}

void Datalogger_ProcessCANCommunications(DataloggerFile *dlgFile) {
	static uint32_t lastTime = 0;
	static uint16_t lastOffset = 0;
	uint16_t currOffset = GetbmsecOffset();;
	static uint8_t txFailed = 0;

	static char bufferTErr[20] = "CT xxxxxxxx/xx TERR\n";

	if (C1TR01CONbits.TXERR0 && txFailed == 0) {
		uint32_t currTime = Get32bitTime();
		uint32_t diffTime = currTime - lastTime;
		if (diffTime > 255) {
			diffTime = 255;
		}

		DBG_SPAM_printf("CAN TXERR0");
		txFailed = 1;
		UI_LED_Pulse(&UI_LED_CAN_Error);
		Int32ToString(currTime, bufferTErr+3);
		Int8ToString((uint8_t)diffTime, bufferTErr+12);
		DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)bufferTErr, 20);
	}

	if (lastOffset > currOffset) {
		DBG_SPAM_printf("CAN Heartbeat");
		ECAN_TransmitBuffer(0);
		txFailed = 0;
		UI_LED_Pulse(&UI_LED_CAN_TX);
	}
	lastOffset = GetbmsecOffset();
	lastTime = Get32bitTime();
}
