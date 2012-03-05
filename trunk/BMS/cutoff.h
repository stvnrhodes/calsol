/* CalSol - UC Berkeley Solar Vehicle Team 
 * cutoffHelper.h - BMS Module
 * Purpose: Helper functions for the BMS module
 * Author(s): Jimmy Hack, Ryan Tseng, Brian Duffy
 * Date: Aug 3rd 2011
 */

#define DEBUG
#define DEBUG_STATES

#ifndef _CUTOFF_HELPER_H_
#define _CUTOFF_HELPER_H_ 

#include <WProgram.h>
#include "bps.h"
#include "pitches.h"
#include "can_id.h"
#include "pindef.h"
#include <avr/pgmspace.h>

/* Constants */
// The max acceptable difference between the battery voltage and the motor
// voltage for turning on the contactors at the end of precharge.
#define VOLT_DELTA 10 

/* State variables */
unsigned long startTime = 0;
unsigned long prechargeTime = 0;
int warning = 0;
int error = 0;
int numHeartbeats = 0;  // Number of bps heartbeats recieved
char precharge_voltage_reached = 0;

// Millisecond counter for sending heartbeats and data
unsigned long last_send_cutoff_can = 0;
unsigned long last_send_bps_can = 0;

enum STATES {
  PRECHARGE,
  NORMAL,
  TURNOFF,
  ERROR
} state;

// Keep track of last state.  This simplifies our state machine a little, 
// but is really adding more states.
enum STATES lastState; 

enum SHUTDOWNREASONS {
  POWER_LOSS,
  KEY_OFF,
  BPS_HEARTBEAT,
  S_UNDERVOLT,
  S_OVERVOLT,
  S_OVERCURRENT,
  BPS_UNDERVOLT,
  BPS_OVERVOLT,
  BPS_OVERTEMP,  
  BPS_EMERGENCY_OTHER, //redundant?
  IO_EMERGENCY,
  CONTROLS_EMERGENCY,
  TELEMETRY_EMERGENCY,
  OTHER1_EMERGENCY,
  OTHER2_EMERGENCY,
  OTHER3_EMERGENCY,
  BPS_ERROR,
  BAD_CAN,
  CODING_ERROR
} shutdownReason;


/* Shutdown Reason Error Codes */  
/* these are stored in program memory to decrease SRAM usage */
/* otherwise we run out of RAM, the board acts strangely and resets */
/* lesson learned, don't do too many serial prints */
prog_char reason_POWER_LOSS[]          PROGMEM = "Loss of power to cutoff.  Possibly due to bomb switch.";
prog_char reason_KEY_OFF[]             PROGMEM = "Normal Shutdown.  Key in off position."; 
prog_char reason_S_UNDERVOLT[]         PROGMEM = "High voltage line undervoltage."; 
prog_char reason_S_OVERVOLT[]          PROGMEM = "High voltage line overvoltage."; 
prog_char reason_S_OVERCURRENT[]       PROGMEM = "High voltage line overcurrent."; 
prog_char reason_BPS_DISCONNECTED[]    PROGMEM = "Battery Protection System Emergency: Other"; 
prog_char reason_BPS_UNDERVOLT[]       PROGMEM = "BPS Emergency: Battery Undervoltage"; 
prog_char reason_BPS_OVERVOLT[]        PROGMEM = "BPS Emergency: Battery Overvoltage"; 
prog_char reason_BPS_OVERTEMP[]        PROGMEM = "BPS Emergency: Batteries Overtemperature"; 
prog_char reason_IO_EMERGENCY[]        PROGMEM = "Input Output Boards Emergency."; 
prog_char reason_CONTROLS_EMERGENCY[]  PROGMEM = "Controls Boards Emergency."; 
prog_char reason_TELEMETRY_EMERGENCY[] PROGMEM = "Telemetry Board Emergency."; 
prog_char reason_OTHER1_EMERGENCY[]    PROGMEM = "Other Board 1 Emergency."; 
prog_char reason_OTHER2_EMERGENCY[]    PROGMEM = "Other Board 2 Emergency."; 
prog_char reason_OTHER3_EMERGENCY[]    PROGMEM = "Other Board 3 Emergency."; 
prog_char reason_BAD_CAN[]             PROGMEM = "Lost CAN Communication.";
prog_char reason_CODING_ERROR[]        PROGMEM = "Unspecified Shutdown Reason: Coding Error.";
prog_char reason_UNKNOWN[]             PROGMEM = "Unknown Shutdown Code";

// an array containing all of our error codes
PROGMEM const char *errorCode_lookup[] = {   
  reason_POWER_LOSS,
  reason_KEY_OFF,
  reason_S_UNDERVOLT,
  reason_S_OVERVOLT,
  reason_S_OVERCURRENT,
  reason_BPS_DISCONNECTED,
  reason_BPS_UNDERVOLT, 
  reason_BPS_OVERVOLT,
  reason_BPS_OVERTEMP,
  reason_IO_EMERGENCY,
  reason_CONTROLS_EMERGENCY,
  reason_TELEMETRY_EMERGENCY,
  reason_OTHER1_EMERGENCY,
  reason_OTHER2_EMERGENCY,
  reason_OTHER3_EMERGENCY,
  reason_BAD_CAN,
  reason_CODING_ERROR,
  reason_UNKNOWN
};

//number of valid shutdown types
unsigned int numErrorCodes = 18; 

/* Prints out the shutdown Reason */
/* strings containing error messages are contained in program memory to save SRAM space */
void printShutdownReason(unsigned int shutdownReason) {
  char buffer[80]; //create a buffer to hold our error string  
  if (shutdownReason >= numErrorCodes) {
    // If we have an invalid shutdownReason, set the reason to unknown
    shutdownReason = numErrorCodes - 1; 
  }
  // Copy the error code from program memory into our buffer c string.
  strcpy_P(buffer, (char*)pgm_read_word(&(errorCode_lookup[shutdownReason])));  
  Serial.print("Shutdown Reason: ");  
  Serial.println( buffer ); //print out the error message
}

/* Buzzer and Music */
// Play buzzer/keep LED on until this time is reached
unsigned long warningTime = 0; 
// Play buzzer for a short duration
unsigned long shortWarning = 100;
// Play buzzer for slightly longer duration
unsigned long longWarning = 500;  

boolean playingSong = false;
boolean playingError = false;
int* duration;
int* notes;
int songSize;
int currentNote = 0;
long endOfNote = 0;
float songTempo = 1;

void initialize();

void initCAN(){
   /* Can Initialization w/ filters */
  Can.reset();
  Can.filterOn();
  Can.setFilter(1, 0x020, 1);
  Can.setFilter(1, 0x040, 2);
  Can.setMask(1, 0x7F0);
  Can.setMask(2, 0x000);
  Can.begin(1000, false);
  CanBufferInit(); 
}

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration);

inline void raiseWarning(int warningID){
  if (!playingError) {  
    playingError = true;
    digitalWrite(OUT_BUZZER, HIGH);
    digitalWrite(LEDFAIL, HIGH);
    warningTime = millis() + shortWarning;
    Serial.println("BPS Warning: Level 1");
    warning = 0; // Reset warning
    EEPROM.write(51, warningID); // Record the reason for the warning
  }
}

inline void raiseError(int warningID){
  // Level 2 Warning: not currently used by BPS
  if (!playingError){
    playingError =true;  
    digitalWrite(OUT_BUZZER, HIGH);
    digitalWrite(LEDFAIL, HIGH);
    warningTime = millis() + longWarning;
    Serial.println("BPS Warning: Level 2");
    warning = 0;
    EEPROM.write(51, warningID); //record the reason for the warning
  }
}

/*Music Functions */

// Play tetris
void loadTetris(){     
  if (!playingSong) {
    playingSong = true;
    duration = tetrisDuration;
    notes = tetrisNotes;
    songSize = tetrisSize;
    currentNote = 0;
  } 
}

void loadBadCan() {
  if (!playingSong) {
    playingSong = true;
    duration = beepDuration;
    notes = beepNotes;
    songSize = beepSize;
    songTempo = 0.5;
    currentNote = 0;
  }
}

// Play BadRomance
void loadBadRomance() {
      //play bad romance
      if (!playingSong) {
        playingSong = true;
        duration = badRomanceDuration;
        notes = badRomanceNotes;
        songSize = badRomanceSize;
        songTempo= .5; //play at half speed
        currentNote = 0;        
      }
}

void playSongs() {
  // Shut off buzzer/LED if no longer sending warning
  if (playingError &&(millis() > warningTime)){
    digitalWrite(LEDFAIL, LOW);
    digitalWrite(OUT_BUZZER, LOW);
  }
  if (playingError &&(millis() > (warningTime+1000))) { 
    //turn off the warning.  Allowing another warning to be raised.
    playingError=false; 
    digitalWrite(LEDFAIL, LOW);
    digitalWrite(OUT_BUZZER, LOW);
  } else if (playingSong && endOfNote < millis()) {
  //play some tunes
    int noteDuration = 1000/(duration[currentNote] *songTempo); 
    tone(OUT_BUZZER, notes[currentNote], noteDuration);
    int pause = noteDuration * 1.30 ;
    endOfNote = millis() + pause;
    currentNote++;
    if (currentNote >= songSize) {
      playingSong = false;
      digitalWrite(OUT_BUZZER, LOW);            
      songTempo = 1;
    }
  }
}

// Reads voltage from first voltage source (millivolts)
long readV1() {
  long reading = analogRead(V1);
  long voltage = reading * 1000000 / 2953;
  return voltage ;
}

// Reads voltage from second voltage source (milliVolts)
long readV2() {
  long reading = analogRead(V2);
  long voltage = reading * 1000000 / 2953;
  return voltage; 
}

// Reads current from first hall effect sensor (milliAmps)
long readC1() {
  long cRead = analogRead(C1);
  //Serial.println(cRead);
  long gndRead = analogRead(CGND);
  //Serial.println(gndRead);
  long c1 = cRead * 5 * 1000 / 1023;
  long cGND = gndRead * 5 * 1000 / 1023;
  long current = 40 * (c1 - cGND); //Scaled over 25 ohm resistor. multiplied by 1000 V -> mV conversion
  return current;
}

// Reads current from second hall effect sensor (milliAmps)
long readC2() {
  long cRead = analogRead(C2);
  long gndRead = analogRead(CGND);
  long c1 = cRead * 5 * 1000 / 1023;
  long cGND = gndRead * 5 * 1000 / 1023;
  long current = 40 * (c1 - cGND); //Scaled over 25 ohm resistor. multiplied by 1000 V -> mV conversion
  return current;
}


// Turn off car if Off Switch is triggered
char checkOffSwitch(){
      //normal shutdown initiated
      if (digitalRead(IN_OFFSWITCH) == HIGH) {
        if (state!=TURNOFF){ 
          Serial.print("key detected in off position\n");
          delay(50); //
        }
        if (digitalRead(IN_OFFSWITCH) == HIGH) { //double check.  
            //Had issues before when releasing key from precharge
            state = TURNOFF;            
            shutdownReason=KEY_OFF;
            //Serial.print("Normal Shutdown\n");
            return 1;
        }
      }
      return 0;
}

// Check bus voltage and bus current
void checkReadings() {
  CanMessage msg = CanMessage();
  long batteryV = readV1();
  long motorV = readV2();
  long batteryC = readC1();
  long otherC = readC2();
  long undervoltage = 94500;  // 90,000 mV
  long overvoltage = 143500;  // 140,000 mV
  long overcurrent1 = 60000;  // 60,000 mA
  long overcurrent2 = 15000;  // 15,000 mA
  if (batteryV <= undervoltage) {
    shutdownReason=S_UNDERVOLT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x02;
    Can.send(msg);
  } else if (batteryV >= overvoltage || motorV >= overvoltage) {
    shutdownReason=S_OVERVOLT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x01;
    Can.send(msg);
  } /* else if (batteryC >= overcurrent1 || otherC >= overcurrent2) {
    shutdownReason=S_OVERCURRENT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x04;
    Can.send(msg);
  } */  //disabled until current sensors reliable
}

void floatEncoder(CanMessage &msg, float f1, float f2) {
  msg.data[0] = *((char *)&f1);
  msg.data[1] = *(((char *)&f1)+1);
  msg.data[2] = *(((char *)&f1)+2);
  msg.data[3] = *(((char *)&f1)+3);
  msg.data[4] = *((char *)&f2);
  msg.data[5] = *(((char *)&f2)+1);
  msg.data[6] = *(((char *)&f2)+2);
  msg.data[7] = *(((char *)&f2)+3);
}

void sendVoltages(){
  CanMessage msg = CanMessage();
  long v1 = readV1(); //get Voltage 1 (in mV)
  long v2 = readV2(); //get Voltage 2 (in mV)
  float mvolt1 = v1; //convert to float (still in mV)
  float mvolt2 = v2; //convert to float (still in mV)
  float volt1 = mvolt1/1000; //convert to Volts from mV
  float volt2 = mvolt2/1000; //convert to Volts from mV
  
  #ifdef DEBUG_MEASUREMENTS
      Serial.print(" V1: ");
      Serial.print(volt1);
      Serial.print("V V2: ");
      Serial.print(volt2);   
      Serial.print("V ");  
  #endif
  
  msg.id = CAN_CUTOFF_VOLT;
  msg.len = 8;
  floatEncoder(msg, volt1, volt2);
  Can.send(msg);
}

void sendCurrents() {
  CanMessage msg = CanMessage();
  long c1 = readC1();
  long c2 = readC2();
  
  float mcurr1 = c1; //convert to float (mA)
  float mcurr2 = c2; //convert to float (mA)
  float curr1 = mcurr1/1000; //convert to A from mA
  float curr2 = mcurr2/1000; //convert to A from mA
  
  #ifdef DEBUG_MEASUREMENTS
    Serial.print("C1: ");
    Serial.print(curr1);
    Serial.print(" C2: ");
    Serial.print(curr2); 
    Serial.print(" RxE: ");
    Serial.println(Can.rxError());
  #endif
  
  msg.id = CAN_CUTOFF_CURR;
  msg.len = 8;
  floatEncoder(msg, curr1, curr2);
  Can.send(msg);
}

/* Send system voltage/current over CAN */
void sendReadings() {
  sendVoltages();
  sendCurrents();  
}

void sendHeartbeat() {
  CanMessage msg;
  msg.id = CAN_HEART_CUTOFF;
  msg.len = 1;
  msg.data[0] = 0x00;
  Can.send(msg);
}

void initVariables(){
  warning = 0;
  // If no other reason is specified, the code on cutoff has an error
  shutdownReason = CODING_ERROR; 
  playingError = false;
  startTime = millis();
  last_send_cutoff_can = millis();
  last_send_bps_can = millis() + 500;  // Stagger by 500 ms.
}

/* Does the precharge routine: Wait for motor voltage to reach a threshold,
 * then turns the car to the on state by switching on both relays */
void do_precharge() {
  #ifdef DEBUG_STATES
    Serial.println("in precharge");
  #endif
  lastState=PRECHARGE;
  long prechargeV = (readV1() / 1000.0); //milliVolts -> Volts
  long batteryV = (readV2() / 1000.0); //milliVolts -> Volts
  int prechargeTarget = 90; // ~100V ?
  int voltageDiff= abs(prechargeV-batteryV);
  
  lastState= PRECHARGE;
  if ( checkOffSwitch() ) {
    /* Off switch engaged, Transition to off */
    state = TURNOFF; //actually redundant
    return;
  } else if ((prechargeV < prechargeTarget)  || (voltageDiff > VOLT_DELTA) || 
             (millis()-startTime < 1000)) {
    //wait for precharge to bring motor voltage up to battery voltage  
    /* Precharge incomplete */
    #ifdef DEBUG
      Serial.print("Precharge State -- Motor Voltage: ");
      Serial.print(prechargeV, DEC);
      Serial.print("V  Battery Voltage: ");
      Serial.print(batteryV, DEC);
      Serial.print("V\n");
      delay(100);
    #endif
    state = PRECHARGE;
  } else {
    /* Precharge complete */
    #ifdef DEBUG
      Serial.print("Precharge Voltage Reached: ");
      Serial.print(prechargeV);
      Serial.println("V");
    #endif
    /* Turn on relays, delay put in place to avoid relay current spike */
    digitalWrite(RELAY1, HIGH);
    delay(100);
    digitalWrite(RELAY2, HIGH);
    delay(100);
    digitalWrite(LVRELAY, HIGH);
    digitalWrite(FAN1, HIGH);
    digitalWrite(FAN2, HIGH);
    
    /* Sound buzzer */
    digitalWrite(OUT_BUZZER, HIGH);
    delay(100);
    digitalWrite(OUT_BUZZER, LOW);
    delay(50);
    digitalWrite(OUT_BUZZER, HIGH);
    delay(100);
    digitalWrite(OUT_BUZZER, LOW);
    delay(50);
    digitalWrite(OUT_BUZZER, HIGH);
    delay(100);
    digitalWrite(OUT_BUZZER, LOW);
    
    /* State Transition */
    state = NORMAL;
  }
}

// Car running state, check for errors, otherwise continue operation
void do_normal() {
  #ifdef DEBUG_STATES
    Serial.println("Car on");
  #endif
  CanMessage msg = CanMessage();
  lastState = NORMAL;
  
  if ( undervolt_error || overvolt_error || overtemp_error || system_error) {
    // Check if there are any errors
    state = ERROR;
    return;
  } else if ( checkOffSwitch() ) {
    //Check if switch is on "On" position
    #ifdef DEBUG
      Serial.println("Switch off. Normal -> Turnoff");
    #endif
    state = TURNOFF;
    return;
  } else if (warning) {
    // Check for warning
    raiseWarning(warning);
  }
  playSongs();
}

void lastShutdownReason() {
  unsigned int memoryIndex = EEPROM.read(0);
  if (memoryIndex > 50){
    // In case EEPROM data is bad
    memoryIndex=1;
  }
  Serial.print("Last ");
  unsigned int lastReason = EEPROM.read(memoryIndex);
  printShutdownReason(lastReason);
}

void shutdownLog() {
  unsigned int memoryIndex = EEPROM.read(0);
  if (memoryIndex > 50 || memoryIndex == 0){
    //invalid value, start at 1
    memoryIndex = 1;
  }
  // Start with the last shutdown reason, not the currently prepped one.  
  memoryIndex--; 
  for(int i=1;i<=49; i++) { 
    if (memoryIndex<1) {
      memoryIndex=50;
    }
    unsigned int lastReason = EEPROM.read(memoryIndex);
    printShutdownReason(lastReason);
    memoryIndex--;
  }
}

void prepShutdownReason(){ //move to next index for shutdown reason
  unsigned int memoryIndex = EEPROM.read(0); 
  memoryIndex++;  // Go to next unused memory position
  if (memoryIndex > 50){
    // Valid index positions 1 - 50
    memoryIndex =1;
  }

  EEPROM.write(0, memoryIndex);  // Record current position in error log
  // If no other shutdown reason specified, assume power loss.
  EEPROM.write(memoryIndex, POWER_LOSS); 
}

// Requires prepShutdownReason to increment memory Index.
void recordShutdownReason() { 
  unsigned int memoryIndex = EEPROM.read(0); 
  if (memoryIndex > 50){ memoryIndex =1;}  // Valid index positions 1 - 50
  EEPROM.write(memoryIndex, shutdownReason); // Record shutdown reason
}

/* Turn the car off normally */
void do_turnoff() {
  #ifdef DEBUG_STATES
    Serial.println("car off");
  #endif
  if (lastState != TURNOFF) {
    lastState=TURNOFF;
    Can.send(CanMessage(CAN_CUTOFF_NORMAL_SHUTDOWN));
    /* Will infinite loop in "off" state as long as key switch is off */
    recordShutdownReason();
    printShutdownReason(shutdownReason);
  }
  
  // Turn off all power by shutting down all relays
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(LVRELAY, LOW);
  
  //if key is no longer in the off position allow for car to restart
  if ( !checkOffSwitch() ) { 
    #ifdef DEBUG_STATES
      Serial.println("Key on, rebooting car");
    #endif
    //allow to restart car if powered by USB
    initialize();
  }    
}

/* Something bad has happened, we must beep loudly and turn the car off */
void do_error() {
  // Do these only when entering the error state
  if (lastState != ERROR) {  
    //turn off relays first
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    digitalWrite(RELAY3, LOW);
    digitalWrite(LVRELAY, LOW);
    digitalWrite(LEDFAIL, HIGH);
    
    // Store the shutdown reason into EEPROM
    #ifdef DEBUG
       Serial.print("  BPS Code: ");
       Serial.print("OV: ");
       Serial.print(overvolt_error);
       Serial.print("UV: ");
       Serial.print(undervolt_error);
       Serial.print("OT: ");
       Serial.print(overtemp_error);
    #endif
    if (overvolt_error)
      shutdownReason = BPS_OVERVOLT;
    else if (undervolt_error)
      shutdownReason = BPS_UNDERVOLT;
    else if (overtemp_error)
      shutdownReason = BPS_OVERTEMP;
    else
     shutdownReason = BPS_EMERGENCY_OTHER;      
    recordShutdownReason();
    printShutdownReason(shutdownReason);
  }
  #ifdef DEBUG_STATES
    Serial.println("Car off due to error");
  #endif
  /* Trap the code execution here on error */
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(LVRELAY, LOW);
  digitalWrite(LEDFAIL, HIGH); 
  playSongs();
  lastState = ERROR;
  state = ERROR;  // Ensure we will not leave the error state
}

void printLastWarning() {
  unsigned int warningID = EEPROM.read(51);
  switch (warningID){
    case 0x01:
      Serial.println("Undervolt Warning detected");
      break;  
    case 0x02:
      Serial.println("Overvolt Warning detected");
      break; 
    case 0x03:
      Serial.println("Temperature Warning detected");
      break; 
    default: 
      Serial.print("Last Warning Unknown ID:");
      Serial.println(warningID);
      break;
  }      
}

inline void processSerial(){
  if (Serial.available()) {
    char letter= Serial.read();
    if (letter=='l') {
      shutdownLog();
    }
    if (letter=='w') {
      printLastWarning();
    }
  }
}

inline void sendCutoffCAN() {
  sendHeartbeat();
  sendReadings();
}

#endif //_CUTOFF_HELPER_H_
