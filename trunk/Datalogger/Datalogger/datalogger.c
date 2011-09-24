/*
 * File:   datalogger.c
 * Author: Ducky
 *
 * Created on August 12, 2011, 12:44 AM
 *
 * Revision History
 * Date			Author	Change
 * 12 Aug 2011	Ducky	Initial implementation.
 *
 * @file
 * Datalogger application.
 */

#include <string.h>

#include "../hardware.h"
#include "../version.h"

#include "../ecan.h"
#include "../timing.h"

#include "../SD-SPI-DMA/sd-spi-dma.h"
#include "../FAT32/fat32.h"
#include "../FAT32/fat32-file.h"

#include "../UserInterface/datalogger-ui-hardware.h"
#include "../UserInterface/datalogger-ui-leds.h"

#include "datalogger.h"
#include "datalogger-stringutil.h"
#include "datalogger-file.h"
#include "datalogger-applications.h"

#define DEBUG_UART
#define DEBUG_UART_DATA
//#define DEBUG_UART_SPAM
#define DBG_MODULE "DLG/Main"
#include "../debug-common.h"

char *MonthLookup[] = {
	"Unk",
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

#define DLG_BUFFER_SIZE		8192
uint8_t dlgBuffer[DLG_BUFFER_SIZE] __attribute__((far));

SD_Card card;
FS_FAT32 fs;
FS_File file;
DataloggerFile dlgFile;

#define DLG_MAX_CARD_INIT_TRIES 16

uint8_t cardInfoWritten;
uint8_t cardInitTries = 0;

void Datalogger_TryFileInit() {
	if (!UI_Switch_GetCardDetect()) {
		UI_LED_SetState(&UI_LED_SD_Write, LED_On);
		UI_LED_SetState(&UI_LED_SD_Error, LED_Off);
		card.State = SD_UNINITIALIZED;
		fs.State = FS_UNINITIALIZED;
		file.state = FILE_Uninitialized;
		cardInitTries = 0;
	}

	// Continue with the initializtaion process
	if (card.State == SD_UNINITIALIZED) {
		if (UI_Switch_GetCardDetect() && cardInitTries < DLG_MAX_CARD_INIT_TRIES) {
			UI_LED_SetState(&UI_LED_SD_Write, LED_Off);
			UI_LED_SetState(&UI_LED_SD_Error, LED_Off);
			SD_Initialize(&card);
		}
	}
	if (card.State == SD_INITIALIZING) {
		sd_result_t result = SD_GetInitializeResult(&card);
		DBG_SPAM_printf("SD_GetInitializeResult -> 0x%02x, substate %u", result, card.SubState);
		if (result == SD_BUSY) {
		} else if (result == SD_SUCCESS) {
			DBG_DATA_printf("SD Card initialized");
			FAT32_Initialize(&fs, &card);
		} else if (result == SD_INITIALIZE_NOSUPPORT) {
			DBG_DATA_printf("SD Card initialialization failed: unsupported card, got 0x%02x", result);
			cardInitTries = DLG_MAX_CARD_INIT_TRIES;
			UI_LED_SetState(&UI_LED_SD_Error, LED_Blink);
		}
		else {
			DBG_DATA_printf("SD Card initialialization failed, got 0x%02x", result);
			cardInitTries++;
			UI_LED_SetState(&UI_LED_SD_Error, LED_Blink);
		}
	}
	if (fs.State == FS_INITIALIZING) {
		fs_result_t result = FAT32_GetInitializeResult(&fs);
		DBG_SPAM_printf("FAT32_GetInitializeResult -> 0x%02x, substate %u", result, fs.SubState);
		if (result == FS_BUSY) {
		} else if (result == FS_SUCCESS) {
			DBG_DATA_printf("FS initialized");
			FS_CreateFileSeqName(&fs, &fs.rootDirectory, &file, "DLG0000", "DLA", 3, 4);
		} else {
			DBG_DATA_printf("FS initialialization failed, got 0x%02x", result);
			cardInitTries++;
			UI_LED_SetState(&UI_LED_SD_Error, LED_Blink);
		}
	}
	if (file.state == FILE_Creating) {
		fs_result_t result = FS_GetCreateFileResult(&file);
		DBG_SPAM_printf("FS_GetCreateFileResult -> 0x%02x, substate %u", result, file.subState);
		if (result == FS_BUSY) {
		} else if (result == FS_SUCCESS) {
			DBG_DATA_printf("File created");
		} else {
			DBG_DATA_printf("File creation failed, got 0x%02x", result);
			cardInitTries++;
			UI_LED_SetState(&UI_LED_SD_Error, LED_Blink);
		}
	}
}

/**
 * Writes the header at the beginning of the file specifying file format, etc.
 * @param file Datalogger file to write to.
 * @return Result.
 * @retval 0 Failure - nothing was written, try again later.
 * @retval 1 Success.
 */
uint8_t Datalogger_WriteCardInfo(DataloggerFile *dlgFile) {
	char bufCard[40] = "CRD xx xx xxxxx x.x xxxxxxxx xxxxx\n";
	char bufMount[40] = "MNT xxxxxxxx\n";
	SD_Card *card = dlgFile->file->fs->card;
	
	Int8ToString(card->MID, bufCard + 4);
	strncpy((char*)bufCard+7, card->OID, 2);
	strncpy((char*)bufCard+10, card->PNM, 5);
	Int4ToString(card->PRV >> 4, bufCard + 16);
	Int4ToString(card->PRV, bufCard+ 18);
	Int32ToString(card->PSN, bufCard+20);
	strncpy(bufCard+29, MonthLookup[(card->MDT >> 00) & 0b1111], 3);
	Int8ToString((uint16_t)((card->MDT >> 4) & 0xff), bufCard+32);

	Int32ToString(Get32bitTime(), bufMount+4);

	return DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)bufCard, 35)
			&& DataloggerFile_WriteAtomic(dlgFile, (uint8_t*)bufMount, 13);
}

void Datalogger_Init() {
	uint8_t i=0;
	uint8_t canDat1[] = {0xca, 0xfe, 0x0d, 0x06, 0xf0, 0x0d};
	uint8_t canDat2[] = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xf0, 0x0d};
	//uint8_t canDat3[] = {0x1b, 0xad, 0xb0, 0x07};
	uint8_t canDat4[] = {0x13, 0x37};
	char buffer[] = "PRM INIT xxxxxxxx xxxx\n";
	
	DBG_printf("Datalogger Initialize")

	Datalogger_InitVoltageRecorder();

	card = SD_CreateCard();
	fs.State = FS_UNINITIALIZED;
	file.state = FILE_Uninitialized;
	DataloggerFile_Init(&dlgFile, &file, dlgBuffer, DLG_BUFFER_SIZE);

	cardInfoWritten = 0;
	cardInitTries = 0;

	DataloggerFile_WriteAtomic(&dlgFile,
(uint8_t*)"PRM FMT 1\n\
PRM SW 0.2\n\
PRM TIMEBASE 1/1024s\n\
PRM VOLTBASE 1/1024Vdd\n\
PRM VOLTMEAS 0 12vPwr 68 18\n\
PRM VOLTMEAS 1 ExtAnalog0\n\
PRM CANCHA 0 Vehicle\n", 140);

#ifdef HARDWARE_RUN_2
	DataloggerFile_WriteAtomic(&dlgFile, (uint8_t*)"PRM HW RUN2\n", 12);
#elif defined HARDWARE_RUN_3
	DataloggerFile_WriteAtomic(&dlgFile, (uint8_t*)"PRM HW RUN3\n", 12);
#endif


	Int32ToString(Get32bitTime(), buffer+9);
	Int16ToString(T1CON, buffer+18);
	DataloggerFile_WriteAtomic(&dlgFile, (uint8_t*)buffer, 23);

	for (i=0;i<UI_LED_Count;i++) {
		UI_LED_SetState(UI_LED_List[i], LED_Off);
	}

	UI_LED_SetBlinkPeriod(&UI_LED_Fault, 512);
	UI_LED_SetState(&UI_LED_Fault, LED_Blink);

	UI_LED_SetBlinkPeriod(&UI_LED_Status_Waiting, 512);
	UI_LED_SetOnTime(&UI_LED_Status_Waiting, 64);
	UI_LED_SetBlinkPeriod(&UI_LED_Status_Operate, 512);
	UI_LED_SetOnTime(&UI_LED_Status_Operate, 64);
	UI_LED_SetState(&UI_LED_Status_Operate, LED_Blink);

	UI_LED_SetState(&UI_LED_SD_Write, LED_On);

	ECAN_WriteStandardBuffer(0, 0x47, 0, NULL);

	ECAN_WriteStandardBuffer(1, 0x137, 6, canDat1);
	ECAN_WriteStandardBuffer(2, 0x138, 8, canDat2);
	ECAN_WriteStandardBuffer(3, 0x139, 2, canDat4);

	UI_LED_Update();
}

void Datalogger_Loop() {
	static uint16_t lastTxTimer = 0;
	uint8_t autoTerminate;

	UI_Switch_Update();

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	if (UI_Switch_GetTest()) {
		if (lastTxTimer > GetbmsecOffset()) {
			lastTxTimer = 0;
		}
		if (GetbmsecOffset() - lastTxTimer > 2) {
			DBG_SPAM_printf("CAN Transmit");
			ECAN_TransmitBuffer(1);
			ECAN_TransmitBuffer(2);
			ECAN_TransmitBuffer(3);
			UI_LED_Pulse(&UI_LED_CAN_TX);
			lastTxTimer = GetbmsecOffset();
		}
	}

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	Datalogger_ProcessCANCommunications(&dlgFile);
	Datalogger_ProcessVoltageRecorder(&dlgFile);
	Datalogger_ProcessPerfLogger(&dlgFile);
	autoTerminate = Datalogger_ProcessAutoTerminate();
	
	unsigned char get = (U2STAbits.URXDA) ? U2RXREG : 0;
	if (get == 'r') {
		exit(0);
	}

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	// If file is ready to go
	fs_result_t result = DataloggerFile_Tasks(&dlgFile);

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	if (file.state == FILE_Uninitialized && file.state == FILE_Creating 
			&& file.state != FILE_Closed
			&& result != FS_BUSY && result != FS_IDLE) {
		DBG_DATA_printf("Error performing file tasks, got 0x%02x", result);
		UI_LED_Pulse(&UI_LED_Status_Error);
	}

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	if (!dlgFile.requestClose) {
		if (!cardInfoWritten
				&& (file.state != FILE_Uninitialized && file.state != FILE_Creating)) {
			DBG_DATA_printf("Attempting to write card information");
			if (Datalogger_WriteCardInfo(&dlgFile)) {
				cardInfoWritten = 1;
			}
		}

		Datalogger_ProcessCANMessages(&dlgFile);
	}

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}

	if (file.state == FILE_Uninitialized || file.state == FILE_Creating) {
		Datalogger_TryFileInit();
	} else if (file.state == FILE_Closed) {
		UI_LED_SetState(&UI_LED_SD_Read, LED_On);
		UI_LED_SetState(&UI_LED_Status_Operate, LED_Off);
		UI_LED_SetState(&UI_LED_Status_Waiting, LED_Blink);

		if (!UI_Switch_GetCardDetect()) {
			Datalogger_Init();
		}
	} else {
		// Process file-based user inputs
		if (UI_Switch_GetCardDismount() || get == 't' || autoTerminate) {
			DataloggerFile_RequestClose(&dlgFile);
		}
	}

	UI_LED_Update();

	if (T1CON == 0x00) {
		DBG_ERR_printf("T1CON = 0");
		T1CON = 0x8002;
	}
}
