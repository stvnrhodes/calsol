;**********************************************************************
;   P R O G R A M M	MPPTnG                                            *
;                                                                     *
;   Traking program for MPPT new generation                           *
;**********************************************************************
;                                                                     *
;    Filename:	    user.inc                                          *
;    Date:          20.04.01                                          *
;    Last Update:   14.03.05                                          *
;    File Version:  V3.0                                              *
;                                                                     *
;    Author:        Michael Zürcher, Christoph Raible                 *
;                                                                     *
;    Company:       HTA-Biel/Bienne Indulab, drivetek ag              *
;                                                                     *
;    Changes:       							                      *
;                                                                     *
;**********************************************************************
;                                                                     *
;    Notes:                                                           *
;    - Main program : mpptng.asm                                      *
;    - Client : 23			                                          *
;								                                      *
;   Order Number: 23				                                  *
;                       					                          *
;**********************************************************************

;**********************************************************************
;   maximum battery voltage
;**********************************************************************
#define INITDUTY	0x89		; Duty resulting of Ubat and Uoc.
								;I think it does 0x25 for UI>70V
								;0x5b for UI>50V
								;0x89 for UI<50V
#define MAXUOH		0x02		; 145.17 V: Max Batt. volt HByte 
					; in 196.97 mV Units
#define MAXUOL		0xE1		; 145.17 V: Max Batt. volt LByte in
					; 196.97 mV Unit

;**********************************************************************
;   P&O Constants
;**********************************************************************
#define MINUINH	0x00	; Min  input voltage HByte 26V in 150.49 mV Units
#define MINUINL	0xAD	; Min  input voltage LByte 26V in 150.49 mV Units

;**********************************************************************
;Min Output Voltage, used for Precharge
;**********************************************************************
#define MINUBATH	0x01	; Min output voltage HByte 94 V in 196.97 mV  Units
#define MINUBATL	0xDD	; Min output voltage LByte 94 V in 196.97 mV  Units

;**********************************************************************
;   border ot discontinuous mode 
;   ON-Values have to be greater than OFF-Values
;   notice the hysteresis NOT USED IN HARDWARE V3.0!!!
;**********************************************************************
;#define IHFETONH	0x01	; H-FET Turn on HByte 32% of max current
;#define IHFETONL	0x47	; H-FET Turn on LByte 32% of max current
;#define IHFETOFFH	0x01	; H-FET Turn on HByte 25% of max current
;#define IHFETOFFL	0x00	; H-FET Turn on LByte 25% of max current


;**********************************************************************
;   Minimum Input Power of 
;**********************************************************************

#define MINPINM 0x16    ; M-Byte Min input power 5W (5.86 mA/Bit x 150.49 mV/Bit)
#define MINPINL 0x26	; L-Byte Min input power

;**********************************************************************
;   Maximum Power Point Input Voltage (influences tracking step width)
;**********************************************************************                                                                                                                                                           
#define UmppH 0x01      ; Mpp input voltage HByte 46 V in 150.49 mV Units
#define UmppL 0x32	    ; Mpp input voltage LByte 46 V in 150.49 mV Units
