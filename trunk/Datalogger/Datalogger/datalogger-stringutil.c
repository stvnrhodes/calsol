/*
 * File:   datalogger-stringutil.c
 * Author: Ducky
 *
 * Created on August 13, 2011, 4:53 PM
 */

#include "../types.h"

#include "datalogger-stringutil.h"

char* Int32ToString(uint32_t input, char *buffer) {
	uint8_t chr;
	chr = input & 0x0f;
	if (chr < 0x0a) {	buffer[7] = chr + '0';
	} else {			buffer[7] = chr + '7';
	}					chr = (input >> 4) & 0x0f;
	if (chr < 0x0a) {	buffer[6] = chr + '0';
	} else {			buffer[6] = chr + '7';
	}					chr = (input >> 8) & 0x0f;
	if (chr < 0x0a) {	buffer[5] = chr + '0';
	} else {			buffer[5] = chr + '7';
	}					chr = (input >> 12) & 0x0f;
	if (chr < 0x0a) {	buffer[4] = chr + '0';
	} else {			buffer[4] = chr + '7';
	}					chr = (input >> 16) & 0x0f;
	if (chr < 0x0a) {	buffer[3] = chr + '0';
	} else {			buffer[3] = chr + '7';
	}					chr = (input >> 20) & 0x0f;
	if (chr < 0x0a) {	buffer[2] = chr + '0';
	} else {			buffer[2] = chr + '7';
	}					chr = (input >> 24) & 0x0f;
	if (chr < 0x0a) {	buffer[1] = chr + '0';
	} else {			buffer[1] = chr + '7';
	}					chr = (input >> 28) & 0x0f;
	if (chr < 0x0a) {	buffer[0] = chr + '0';
	} else {			buffer[0] = chr + '7';
	}
	return buffer;
}

char* Int16ToString(uint16_t input, char *buffer) {
	uint8_t chr;
	chr = input & 0x0f;
	if (chr < 0x0a) {	buffer[3] = chr + '0';
	} else {			buffer[3] = chr + '7';
	}					chr = (input >> 4) & 0x0f;
	if (chr < 0x0a) {	buffer[2] = chr + '0';
	} else {			buffer[2] = chr + '7';
	}					chr = (input >> 8) & 0x0f;
	if (chr < 0x0a) {	buffer[1] = chr + '0';
	} else {			buffer[1] = chr + '7';
	}					chr = (input >> 12) & 0x0f;
	if (chr < 0x0a) {	buffer[0] = chr + '0';
	} else {			buffer[0] = chr + '7';
	}
	return buffer;
}

char* Int12ToString(uint16_t input, char *buffer) {
	uint8_t chr;
	chr = input & 0x0f;
	if (chr < 0x0a) {	buffer[2] = chr + '0';
	} else {			buffer[2] = chr + '7';
	}					chr = (input >> 4) & 0x0f;
	if (chr < 0x0a) {	buffer[1] = chr + '0';
	} else {			buffer[1] = chr + '7';
	}					chr = (input >> 8) & 0x0f;
	if (chr < 0x0a) {	buffer[0] = chr + '0';
	} else {			buffer[0] = chr + '7';
	}
	return buffer;
}

char* Int8ToString(uint8_t input, char *buffer) {
	uint8_t chr;
	chr = input & 0x0f;
	if (chr < 0x0a) {	buffer[1] = chr + '0';
	} else {			buffer[1] = chr + '7';
	}					chr = (input >> 4) & 0x0f;
	if (chr < 0x0a) {	buffer[0] = chr + '0';
	} else {			buffer[0] = chr + '7';
	}
	return buffer;
}

char* Int4ToString(uint8_t input, char *buffer) {
	uint8_t chr;
	chr = input & 0x0f;
	if (chr < 0x0a) {	buffer[0] = chr + '0';
	} else {			buffer[0] = chr + '7';
	}
	return buffer;
}
