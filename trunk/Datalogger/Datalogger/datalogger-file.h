/*
 * File:   datalogger-file.h
 * Author: Ducky
 *
 * Created on August 12, 2011, 1:16 PM
 *
 * Revision History
 * Date			Author	Change
 * 12 Aug 2011	Ducky	Initial implementation.
 *
 * @file
 * Datalogger file operations, including a large circular buffer in RAM.
 */

#ifndef DATALOGGER_FILE_H
#define DATALOGGER_FILE_H

#include "../FAT32/fat32-file.h"

typedef struct {
	FS_File *file;			/// Pointer to the open file.

	// Buffering variables
	uint8_t *buffer;		/// Pointer to the beginning of the data block.
	uint16_t bufferSize;	/// Size, in bytes, of the circular buffer.
	uint16_t bufferFree;	/// Number of free bytes left in the buffer;
	uint16_t readPos;		/// Position to read from, the beginning of the buffer.
	uint16_t writePos;		/// Position to write to, the end of the buffer.

	// Random
	uint8_t requestClose;	/// If the file is requested to be closed.
} DataloggerFile;

/**
 * Initializes the Datalogger file.
 *
 * @param dlgFile Datalogger file to initialize.
 * @param file Filesystem file to write to. Can be uninitialized, and disk
 * operations will not start until the file has been successfully initialized.
 * @param buffer RAM buffer
 * @param bufferSize Size of the RAM buffer, in bytes, or 0 if a RAM buffer
 * is not being used.
 */
void DataloggerFile_Init(DataloggerFile *dlgFile, FS_File *file,
		uint8_t *buffer, uint16_t bufferSize);

/**
 * Writes data to a Datalogger file.
 * This either writes the entire string or nothing at all. Partial writes
 * will NOT happen.
 *
 * @param dlgFile Datalogger file to be written to,
 * @param data Data to write.
 * @param dataLen Data length, in bytes.
 * @return Result of the write operation.
 * @retval 0 Write failed - not enough space in the buffer.
 * @retval 1 Write successful, entire data written.
 */
uint16_t DataloggerFile_WriteAtomic(DataloggerFile *dlgFile, uint8_t *data,
		uint16_t dataLen);

/**
 * Called periodically to perform tasks for the Datalogger file, such as
 * writing the RAM buffer to the file and performing filesystem file tasks.
 *
 * @param dlgFile Datalogger file to process.
 * @return Result of the call to FS_FileTasks.
 */
fs_result_t DataloggerFile_Tasks(DataloggerFile *dlgFile);

/**
 * Requests that the file be closed.
 *
 * @param dlgFile Datalogger file to process.
 */
void DataloggerFile_RequestClose(DataloggerFile *dlgFile);

#endif
