/*
 * File:   hardware.h
 * Author: Ducky
 *
 * Created on July 21, 2011, 9:48 PM
 *
 * Revision History
 * Date			Author	Change
 * 21 Jul 2011	Ducky	File creation.
 *
 * @file
 * Defines the hardware platform upon which the code runs.
 * As a general note, only the latest hardware platform will be fully supported
 * although backwards compatibility will remain a goal.
 *
 * The define macro to select the hardware should be set in the project options.
 */

#ifdef HARDWARE_RUN_2
	#include "hardware-run2.h"
#elif defined HARDWARE_RUN_3
	#include "hardware-run3.h"
#elif defined HARDWARE_CANBRIDGE
	#include "hardware-canbridge.h"
#endif
