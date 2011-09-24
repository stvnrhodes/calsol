/*
 * File:   main.c
 * Author: Ducky
 *
 * Created on January 15, 2011, 5:22 PM
 *
 * Revision History
 * Date			Author	Change
 * 21 Jul 2011	Ducky	Added this change history box.
 *
 * @file
 * Application level code.
 */

//	#include <p24Hxxxx.h>	// included in hardware.h
#include <string.h>

#include "hardware.h"
#include "version.h"

#include "timing.h"
#include "ecan.h"
#include "uart-dma.h"
#include "Datalogger/datalogger.h"

#include "UserInterface/datalogger-ui-hardware.h"
#include "UserInterface/datalogger-ui-leds.h"

#define DEBUG_UART
#define DEBUG_UART_DATA
#include "debug-common.h"

/*
 * Function Prototypes
 */
void OscInit(void);
void AnalogInit(void);
void OutputInit(void);
void PPSInit(void);

/*
 * Configuration Bits
 */
#if defined(__PIC24HJ128GP502__)
	_FBS(RBS_NO_RAM & BSS_NO_FLASH & BWRP_WRPROTECT_OFF);
	_FSS(RSS_NO_RAM & SSS_NO_FLASH & SWRP_WRPROTECT_OFF);
	_FGS(GSS_OFF & GCP_OFF & GWRP_OFF);

	_FOSCSEL(FNOSC_PRI & IESO_OFF);
	_FOSC(FCKSM_CSDCMD & IOL1WAY_OFF & OSCIOFNC_OFF & POSCMD_HS);
	_FWDT(FWDTEN_OFF & WINDIS_OFF & WDTPRE_PR128 & WDTPOST_PS512);
	_FPOR(ALTI2C_OFF & FPWRT_PWR1);
	_FICD(JTAGEN_OFF & ICS_PGD2);
#elif defined (__dsPIC33FJ128MC802__)
	_FBS(RBS_NO_RAM & BSS_NO_FLASH & BWRP_WRPROTECT_OFF);
	_FSS(RSS_NO_RAM & SSS_NO_FLASH & SWRP_WRPROTECT_OFF);
	_FGS(GSS_OFF & GCP_OFF & GWRP_OFF);

	_FOSCSEL(FNOSC_PRIPLL & IESO_OFF);
	_FOSC(FCKSM_CSDCMD & IOL1WAY_OFF & OSCIOFNC_OFF & POSCMD_HS);
	//_FOSCSEL(FNOSC_FRC & IESO_OFF);	//		Configuration for internal 7.37 MHz FRC
	//_FOSC(FCKSM_CSDCMD & IOL1WAY_OFF & OSCIOFNC_ON & POSCMD_NONE);
	_FWDT(FWDTEN_OFF & WINDIS_OFF & WDTPRE_PR128 & WDTPOST_PS512);
#ifdef HARDWARE_RUN_2
	_FPOR(PWMPIN_ON & HPOL_OFF & ALTI2C_ON & FPWRT_PWR1);
	_FICD(JTAGEN_OFF & ICS_PGD2);
#elif defined HARDWARE_RUN_3
	_FPOR(PWMPIN_ON & HPOL_OFF & ALTI2C_OFF & FPWRT_PWR1);
	_FICD(JTAGEN_OFF & ICS_PGD1);
#elif defined HARDWARE_CANBRIDGE
	_FPOR(PWMPIN_ON & HPOL_OFF & ALTI2C_OFF & FPWRT_PWR1);
	_FICD(JTAGEN_OFF & ICS_PGD3);
#endif
#endif

int main(void) {
	uint16_t i = 0;

	OscInit();
	AnalogInit();
	OutputInit();
	PPSInit();

	Timing_Init();
	UART_DMA_Init();

	UI_LED_Initialize();
	for (i=0;i<UI_LED_Count;i++) {
		UI_LED_SetState(UI_LED_List[i], LED_On);
	}
	UI_LED_Update();

	DBG_printf("\r\n");
	DBG_printf("\2330;36mCalsol Datalogger v%u.%u (alpha)\23337m", VERSION_MAJ, VERSION_MIN);
	DBG_printf("\23336m  Built %s %s with C30 ver %i\23337m", __DATE__, __TIME__, __C30_VERSION__);

	DBG_DATA_printf("\23336mDevice reset:%s%s%s%s%s%s%s%s\23337m",
			(RCONbits.TRAPR? " Trap" : ""),
			(RCONbits.IOPUWR? " IllegalOpcode/UninitializedW" : ""),
			(RCONbits.CM? " ConfigMismatch" : ""),
			(RCONbits.EXTR? " ExternalReset" : ""),
			(RCONbits.SWR? " SoftwareReset" : ""),
			(RCONbits.WDTO? " WatchdogTimeout" : ""),
			(RCONbits.BOR? " BrownOutReset" : ""),
			(RCONbits.POR? " PowerOnReset" : "")
			);

	ECAN_Init();
	ECAN_Config();
	C1FCTRLbits.FSA = 4;	// FIFO starts
	C1FEN1 = 0;
	ECAN_SetStandardFilter(0, 0x00, 0, 15);
	ECAN_SetStandardMask(0, 0x00);
	ECAN_SetMode(ECAN_MODE_OPERATE);
	ECAN_SetupDMA();

	UI_Switch_Update();
	if (UI_Switch_GetTest()) {
		DBG_printf("Entering test mode");
		UI_LED_SetState(&UI_LED_Fault, LED_Blink);
		while (UI_Switch_GetTest()) {
			UI_LED_Update();
			UI_Switch_Update();
		}
		UI_LED_SetState(&UI_LED_Fault, LED_Off);
		UI_LED_Update();
	}

	for (i=0;i<UI_LED_Count;i++) {
		UI_LED_SetState(UI_LED_List[i], LED_Off);
	}

	DBG_printf("Initialization complete");


	Datalogger_Init();

	while(1) {
		Datalogger_Loop();
	}
}

/**
 * Initializes oscillator configuration, including setting up PLL
 */
void OscInit(void) {
	_PLLPRE = 14;	// N1=16
	_PLLPOST = 1;	// N2=4
	_PLLDIV = 126;	// M=128
	__builtin_write_OSCCONL( 0x02 );    // enable SOSC
}

/**
 * Initializes analog input configuration, including whether pins are digital or analog.
 */
void AnalogInit(void) {
	AD1PCFGL = 0b0001111111111100;	// Set unused analog pins for digital IO

	AD1CON2bits.VCFG = 0b000;		// VR+ = AVdd, VR- = AVss
	AD1CON3bits.ADCS = 63;			// 64 Tcy conversion clock
	AD1CON1bits.SSRC = 0b111;		// Auto-convert
	AD1CON3bits.SAMC = 31;			// and auto-convert time bits
	AD1CON1bits.ASAM = 1;			// Auto-sample
	AD1CON1bits.FORM = 0b00;		// Integer representation
	AD1CON2bits.SMPI = 0b0000;		// Interrupt rate, 1 for each sample/convert

	AD1CHS0bits.CH0SA = ANA_CH_12VDIV;
	AD1CHS0bits.CH0SB = ANA_CH_12VDIV;

	AD1CON1bits.ADON = 1;			// Enable A/D module
}

/**
 * Initializes both direction and initial state of all GPIO pins.
 */
void OutputInit(void) {
	SD_SPI_CS_IO = 1;
	SD_SPI_CS_TRIS = 0;
}

/**
 * Initializes PPS selections for all peripheral modules using PPS: UART, ECAN, SD SPI.
 */
void PPSInit(void) {
	// Unlock Registers
	__asm__ volatile (	"MOV #OSCCON, w1 \n"
						"MOV #0x46, w2 \n"
						"MOV #0x57, w3 \n"
						"MOV.b w2, [w1] \n"
						"MOV.b w3, [w1] \n"
						"BCLR OSCCON,#6");
	// UART Pins
	UART_TX_RPR = U2TX_IO;
	_U2RXR = UART_RX_RPN;

	// ECAN Pins
	ECAN_TXD_RPR = C1TX_IO;
	_C1RXR = ECAN_RXD_RPN;

	// SD SPI Pins
	SD_SPI_SCK_RPR = SCK2OUT_IO;
	SD_SPI_MOSI_RPR = SDO2_IO;
	_SDI2R = SD_SPI_MISO_RPN;

#ifdef HARDWARE_RUN_3
	_T3CKR = SOSCI_RPN;
#endif

	// Lock Registers
	__asm__ volatile (	"MOV #OSCCON, w1 \n"
						"MOV #0x46, w2 \n"
						"MOV #0x57, w3 \n"
						"MOV.b w2, [w1] \n"
						"MOV.b w3, [w1] \n"
						"BSET OSCCON, #6" );
}
