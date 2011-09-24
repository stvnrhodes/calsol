/*
 * File:   timing-run3.c
 * Author: Ducky
 *
 * Created on July 9, 2011, 7:34 PM
 *
 * Revision History
 * Date			Author	Change
 * 27 Jul 2011	Ducky	Added this revision history box.
 *						Timing now uses TMR1 with the 32.768 kHz secondary
 *						oscillator.
 * 12 Aug 2011	Ducky	Separated Run 2 and Run 3 timing.
 *
 * @file
 * Hardware abstraction layer for the real-time clock and timer functions.
 */
#include "hardware.h"
#include "timing.h"

#if defined(HARDWARE_RUN_3) || defined(HARDWARE_CANBRIDGE)

seconds_t TimeSeconds;

/**
 * Initializes hardware needed for the timing functions.
 * This starts the seconds counter at the current time.
 */
void Timing_Init() {
	// Initialize variables
	TimeSeconds = 0;

	// Initialize Timer
	T1CONbits.TCS = 1;
	PR1 = 32768;
	TMR1 = 0;
	_T1IE = 1;
	T1CONbits.TON = 1;
}

/**
 * @return The number of seconds elapsed since power on, rounded down.
 */
inline seconds_t GetTimeSeconds() {
	return TimeSeconds;
}

/**
 * @return The binary-milliseconds (1/1024 second) offset from the current
 * second.
 */
inline uint16_t GetbmsecOffset() {
	return TMR1 >> 5;
}

/**
 * @return The current time, in 32-bit format. The low 10 bits represent
 * the fractions of a second in 1/1024 increments while the upper 22 bits
 * represent the time in seconds.
 */
inline uint32_t Get32bitTime() {
	uint32_t retVal = TMR1 >> 5;
	retVal = retVal | (TimeSeconds << 10);
	return retVal;
}

/**
 * Starts the countdown for the CountdownTimer object.
 *
 * @param timer Timer to start.
 * @param duration Duration of the timer in 1/1024 seconds.
 */
void Timer_StartCountdown(CountdownTimer *timer, uint16_t duration) {
	uint16_t secs = (duration - (duration % 1024)) / 1024;
	timer->ExpirationSeconds = TimeSeconds + secs;
	duration = duration % 1024;
	
	timer->ExpirationFracSecs = TMR1;
	timer->ExpirationFracSecs += duration * 32;

	if (timer->ExpirationFracSecs >= 32768) {
		timer->ExpirationSeconds++;
		timer->ExpirationFracSecs -= 32768;
	}
}

/**
 * Checks if the CountdownTimer object has expired (time has elapsed).
 *
 * @param timer Timer to check.
 * @return
 */
uint8_t Timer_CountdownExpired(CountdownTimer *timer) {
	if (timer->ExpirationSeconds > TimeSeconds) {
		return 1;
	} else if (timer->ExpirationSeconds == TimeSeconds) {
		if (timer->ExpirationFracSecs > TMR1) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

// Interrupt Handler
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
	_T1IF = 0;
	TimeSeconds++;
}

#endif
