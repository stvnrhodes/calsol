/*
 * File:   datalogger-autoterminate.c
 * Author: Ducky
 *
 * Created on August 13, 2011, 4:47 PM
 *
 * Revision History
 * Date			Author	Change
 * 13 Aug 2011	Ducky	Initial implementation.
 *
 * @file
 * Datalogger automatic file closing routine.
 */

#define TERMINATION_THRES_HIGH		2350		// ~9v
#define TERMINATION_THRES_LOW		1800		// ~7v

#define TERMINATION_TIME_ARM		2048
#define TERMINATION_TIME_FIRE		2048

#include "../UserInterface/datalogger-ui-leds.h"
#include "../timing.h"

#include "datalogger-applications.h"

#define DEBUG_UART
#define DEBUG_UART_DATA
//#define DEBUG_UART_SPAM
#define DBG_MODULE "DLG/AutoTerm"
#include "../debug-common.h"

extern uint16_t currVoltage;

uint8_t Datalogger_ProcessAutoTerminate() {
	static uint8_t termState = 0;
	static uint32_t termTime = 0;
	uint8_t retval = 0;

	if (termState == 0) {			// Termination not armed
		if (currVoltage > TERMINATION_THRES_HIGH) {
			DBG_DATA_printf("Arming termination");
			termTime = Get32bitTime();
			termState = 1;
			UI_LED_SetBlinkPeriod(&UI_LED_Fault, 256);
			UI_LED_SetState(&UI_LED_Fault, LED_Blink);
		}
	} else if (termState == 1) {	// Termination arming
		if (currVoltage > TERMINATION_THRES_HIGH) {
			if (Get32bitTime() - termTime >= TERMINATION_TIME_ARM) {
				DBG_DATA_printf("Armed termination");
				termState = 2;
				UI_LED_SetState(&UI_LED_Fault, LED_Off);
			}
		} else {
			DBG_DATA_printf("Disarmed termination");
			termState = 0;
			UI_LED_SetBlinkPeriod(&UI_LED_Fault, 512);
			UI_LED_SetState(&UI_LED_Fault, LED_Blink);
		}
	} else if (termState == 2) {	// Termination armed
		if (currVoltage < TERMINATION_THRES_LOW) {
			DBG_DATA_printf("Firing termination");
			termTime = Get32bitTime();
			termState = 3;
			UI_LED_SetBlinkPeriod(&UI_LED_Fault, 256);
			UI_LED_SetState(&UI_LED_Fault, LED_Blink);
		}
	} else if (termState == 3) {	// Termination firing
		if (currVoltage < TERMINATION_THRES_HIGH) {
			if (Get32bitTime() - termTime >= TERMINATION_TIME_FIRE) {
				DBG_DATA_printf("Fired termination");
				termState = 0;
				retval = 1;
				UI_LED_SetBlinkPeriod(&UI_LED_Fault, 512);
				UI_LED_SetState(&UI_LED_Fault, LED_Blink);
			}
		} else {
			DBG_DATA_printf("Termination aborted");
			termState = 2;
			UI_LED_SetState(&UI_LED_Status_Operate, LED_Off);
		}
	}

	return retval;
}
