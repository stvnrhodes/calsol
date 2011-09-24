/*
 * File:   datalogger-stringutil.h
 * Author: Ducky
 *
 * Created on August 13, 2011, 4:55 PM
 */

#ifndef DATALOGGER_STRINGUTIL_H
#define DATALOGGER_STRINGUTIL_H

#include "../types.h"

/**
 * Converts a 32-bit integer to a hexadecimal string stored in /a buffer.
 * Output is not null-terminated.
 * @param input Input integer to convert.
 * @param buffer Location to store the output string.
 * @return Pointer to the output string, equal to /a buffer.
 */
char* Int32ToString(uint32_t input, char *buffer);

/**
 * Converts a 16-bit integer to a hexadecimal string stored in /a buffer.
 * Output is not null-terminated.
 * @param input Input integer to convert.
 * @param buffer Location to store the output string.
 * @return Pointer to the output string, equal to /a buffer.
 */
char* Int16ToString(uint16_t input, char *buffer);

/**
 * Converts a 12-bit integer to a hexadecimal string stored in /a buffer.
 * Output is not null-terminated.
 * @param input Input integer to convert.
 * @param buffer Location to store the output string.
 * @return Pointer to the output string, equal to /a buffer.
 */
char* Int12ToString(uint16_t input, char *buffer);

/**
 * Converts a 8-bit integer to a hexadecimal string stored in /a buffer.
 * Output is not null-terminated.
 * @param input Input integer to convert.
 * @param buffer Location to store the output string.
 * @return Pointer to the output string, equal to /a buffer.
 */
char* Int8ToString(uint8_t input, char *buffer);

/**
 * Converts a 4-bit integer to a hexadecimal string stored in /a buffer.
 * Output is not null-terminated.
 * @param input Input integer to convert.
 * @param buffer Location to store the output string.
 * @return Pointer to the output string, equal to /a buffer.
 */
char* Int4ToString(uint8_t input, char *buffer);

#endif
