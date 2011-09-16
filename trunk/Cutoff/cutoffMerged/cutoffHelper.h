/* CalSol - UC Berkeley Solar Vehicle Team 
 * cutoffHelper.h - Cutoff Module
 * Purpose: Helper functions for the cutoff module
 * Author(s): Jimmy Hack, Ryan Tseng, Brian Duffy
 * Date: Aug 3rd 2011
 */

#ifndef _CUTOFF_HELPER_H_
#define _CUTOFF_HELPER_H_ 
#include <WProgram.h>
#include "pitches.h"
#include "cutoffCanID.h"
#include "cutoffPindef.h"
#include <avr/pgmspace.h>

/* State variables */

unsigned long startTime = 0; //the time at which the car started up
unsigned long prechargeTime =0; //the time at which the precharge voltage was reached

volatile unsigned long last_readings = 0;  //the last time at which the cutoff sent out current and voltage readings
volatile unsigned long last_heart_bps = 0;  //the last time at which a bps heartbeat was received
volatile unsigned long last_heart_driver_io = 0;  //the last time at which a Driver IO heartbeat was received
volatile unsigned long last_heart_driver_ctl = 0;  //the last time at which a Driver Controls heartbeat was received
volatile unsigned long last_heart_telemetry = 0;  //the last time at which a telemetry heartbeat was received
volatile unsigned long last_heart_datalogger = 0;  // the last time at which a datalogger heartbeat was received
volatile unsigned long last_heart_cutoff = 0;  //the last time at which the cutoff sent out a heartbeat
volatile unsigned long last_can = 0;        //time at which the last CAN message was received
volatile unsigned long last_printout = 0;   //time at which last printout of number of heartbeats was made
volatile int emergency = 0;                  //emergency flag
volatile int warning = 0;    //warning flag
volatile int bps_code = 0;    //stores the message data contained in any bps error messages
int numHeartbeats = 0; //number of bps heartbeats recieved
char precharge_voltage_reached = 0; //flag to indicate if we've already reached the precharge voltage

enum STATES { //a list of all of the states of the car
  PRECHARGE,
  NORMAL,
  TURNOFF,
  ERROR
} state;

enum STATES lastState; //keep track of last state.  This simplifies our state machine a little, but is really adding more states.

enum SHUTDOWNREASONS { //a list of all valid shutdown reasons
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
  BPS_ERROR, //critical heartbeat
  BAD_CAN,
  CODING_ERROR
} shutdownReason;


/* Shutdown Reason Error Codes */  
/* these are stored in program memory to decrease SRAM usage */
/* otherwise we run out of RAM, the board acts strangely and resets */
/* lesson learned, don't do too many serial prints */
prog_char reason_POWER_LOSS[]          PROGMEM = "Manual reset, power loss or runtime error. Check bomb switch.";
prog_char reason_KEY_OFF[]             PROGMEM = "Normal Shutdown.  Key in off position."; 
prog_char reason_BPS_HEARTBEAT[]       PROGMEM = "Missing BPS heartbeat."; 
prog_char reason_S_UNDERVOLT[]         PROGMEM = "High voltage line undervoltage."; 
prog_char reason_S_OVERVOLT[]          PROGMEM = "High voltage line overvoltage."; 
prog_char reason_S_OVERCURRENT[]       PROGMEM = "High voltage line overcurrent."; 
prog_char reason_BPS_EMERGENCY_OTHER[] PROGMEM = "Battery Protection System Emergency: Other"; 
prog_char reason_BPS_UNDERVOLT[]       PROGMEM = "BPS Emergency: Battery Undervoltage"; 
prog_char reason_BPS_OVERVOLT[]        PROGMEM = "BPS Emergency: Battery Overvoltage"; 
prog_char reason_BPS_OVERTEMP[]        PROGMEM = "BPS Emergency: Batteries Overtemperature"; 
prog_char reason_IO_EMERGENCY[]        PROGMEM = "Input Output Boards Emergency."; 
prog_char reason_CONTROLS_EMERGENCY[]  PROGMEM = "Controls Boards Emergency."; 
prog_char reason_TELEMETRY_EMERGENCY[] PROGMEM = "Telemetry Board Emergency."; 
prog_char reason_OTHER1_EMERGENCY[]    PROGMEM = "Other Board 1 Emergency."; 
prog_char reason_OTHER2_EMERGENCY[]    PROGMEM = "Other Board 2 Emergency."; 
prog_char reason_OTHER3_EMERGENCY[]    PROGMEM = "Other Board 3 Emergency."; 
prog_char reason_BPS_ERROR[]           PROGMEM = "Critical Battery Protection System Heartbeat.";
prog_char reason_BAD_CAN[]             PROGMEM = "Lost CAN Communication.";
prog_char reason_CODING_ERROR[]        PROGMEM = "Unspecified Shutdown Reason: Coding Error.";
prog_char reason_UNKNOWN[]             PROGMEM = "Unknown Shutdown Code";

PROGMEM const char *errorCode_lookup[] = 	   // an array containing all of our error codes
{   
  reason_POWER_LOSS,
  reason_KEY_OFF,
  reason_BPS_HEARTBEAT,
  reason_S_UNDERVOLT,
  reason_S_OVERVOLT,
  reason_S_OVERCURRENT,
  reason_BPS_EMERGENCY_OTHER,
  reason_BPS_UNDERVOLT, 
  reason_BPS_OVERVOLT,
  reason_BPS_OVERTEMP,
  reason_IO_EMERGENCY,
  reason_CONTROLS_EMERGENCY,
  reason_TELEMETRY_EMERGENCY,
  reason_OTHER1_EMERGENCY,
  reason_OTHER2_EMERGENCY,
  reason_OTHER3_EMERGENCY,
  reason_BPS_ERROR,
  reason_BAD_CAN,
  reason_CODING_ERROR,
  reason_UNKNOWN
};

unsigned int numErrorCodes=20; //number of valid shutdown types


/* Prints out the shutdown Reason */
/* strings containing error messages are contained in program memory to save SRAM space */
void printShutdownReason(unsigned int shutdownReason){
  char buffer[80]; //create a buffer to hold our error string  
  Serial.print("Shutdown Reason ");
  Serial.print(shutdownReason, DEC); //print ID of shutdown error
  Serial.print(": "); 
  if (shutdownReason>=numErrorCodes){
    shutdownReason =numErrorCodes-1; //if we have an invalid shutdownReason, set the reason to unknown
  }
  strcpy_P(buffer, (char*)pgm_read_word(&(errorCode_lookup[shutdownReason])));  //copy the error code from program memory into our buffer c string.    
  Serial.println( buffer ); //print out the error message
}



/*Buzzer and Music */
unsigned long warningTime = 0; //play buzzer/keep LED on until this time is reached
unsigned long shortWarning = 100; //play buzzer for a short duration
unsigned long longWarning = 500; //play buzzer for slightly longer duration

boolean playingSong = false; //keep track of whether a song is currently being played
boolean playingError = false; //keep track of whether an error noise is currently being played
int* duration; //an array of the lengths of notes in a song
int* notes; //an array of the notes in a song
int songSize; //length of a song in number of notes
int currentNote=0; //current note loaded in a song
long endOfNote=0; //time at which to end the current note
float songTempo =1; //the relative speed to play a song at

void process_packet(CanMessage &msg);  //prototype for CAN interpretation function


void initialize(); //prototype for initialization function to be called on car reset

//configure CAN connection
void initCAN(){
   /* Can Initialization w/ filters */
  Can.reset();
  Can.filterOn();
  Can.setFilter(1, 0x020, 1);
  Can.setFilter(1, 0x040, 2);
  Can.setMask(1, 0x7F0);
  Can.setMask(2, 0x000);
  Can.attach(&process_packet);
  Can.begin(1000, false);
  CanBufferInit(); 
}

void tone(uint8_t _pin, unsigned int frequency, unsigned long duration); //prototype for tone function

//raise a level 1 warning
inline void raiseWarning(int warningID){
  if (!playingError){  
    playingError =true;
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LEDFAIL, HIGH);
    warningTime = millis() + shortWarning;
    Serial.println("BPS Warning: Level 1");
    warning=0; //reset warning
    EEPROM.write(51, warningID); //record the reason for the warning
  }
}

//raise a level 2 warning
inline void raiseError(int warningID){  //Level 2 Warning: not currently used by BPS
  if (!playingError){
    playingError =true;  
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LEDFAIL, HIGH);
    warningTime = millis() + longWarning;
    Serial.println("BPS Warning: Level 2");
    warning=0;
    EEPROM.write(51, warningID); //record the reason for the warning
  }
}

/*Music Functions */

//prep tetris song
void loadTetris(){     
  if (!playingSong) {
    playingSong = true;
    duration = tetrisDuration;
    notes = tetrisNotes;
    songSize = tetrisSize;
    currentNote = 0;
  } 
}

//prep 3 beep noise to indicate bad CAN communication
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

//prep badRomance to play next
void loadBadRomance(){
      //play bad romance
      if (!playingSong) {
        playingSong = true;
        duration = badRomanceDuration;
        notes = badRomanceNotes;
        songSize = badRomanceSize;
        songTempo= .75; //play at slower speed
        currentNote = 0;        
      }
}

//updates the buzzer to make it play the next notes in the currently loaded song
//does not use delays so should have minimal effect on code execution time
void playSongs(){
//shut off buzzer/LED if no longer sending warning
        if (playingError &&(millis() > warningTime)){
          digitalWrite(LEDFAIL, LOW);
          digitalWrite(BUZZER, LOW);
        }
        if (playingError &&(millis() > (warningTime+1000))) { 
          playingError=false; //turn off the warning.  Allowing another warning to be raised.
          digitalWrite(LEDFAIL, LOW);
          digitalWrite(BUZZER, LOW);
        }
        else if (playingSong && endOfNote < millis()) {
        //play some tunes
          int noteDuration = 1000/(duration[currentNote] *songTempo); 
          tone(BUZZER, notes[currentNote], noteDuration);
          int pause = noteDuration * 1.30 ;
          endOfNote = millis() + pause;
          currentNote++;
          if (currentNote >= songSize) {
            playingSong = false;
            digitalWrite(BUZZER, LOW);            
            songTempo=1;
          }
        }
}

//reads voltage from first voltage source (millivolts)
long readV1() {
  long reading = analogRead(V1);
  long voltage = reading * 5 * 1000 / 1023 ;  
  // 2.7M+110K +470K/ 110K Because a fuse kept blowing We also added
  // in another resistor to limit the current (470K).
  voltage = voltage * (270+3.9) / 3.9; // 2.7M+390K / 110K   voltage divider
  return voltage ;
}

//reads voltage from second voltage source (milliVolts)
long readV2() {
  long reading = analogRead(V2);
  long voltage = reading * 5 *1000 / 1023 ;  
  // 2.7M+110K +470K/ 110K Because a fuse kept blowing We also added
  // in another resistor to limit the current (470K).
  voltage = voltage * (270+3.9) / 3.9; // 2.7M+390K / 110K   voltage divider
  return voltage; 
}

//reads current from first hall effect sensor (milliAmps)
long readC1() {
  long cRead = analogRead(C1);
  //Serial.println(cRead);
  long gndRead = analogRead(CGND);
  //Serial.println(gndRead);
  long c1 = cRead * 5 * 1000 / 1023;
  long cGND = gndRead * 5 * 1000 / 1023;
  long current = 80 * (c1 - cGND); //Scaled over 25 ohm resistor. multiplied by 1000 V -> mV conversion
                                   //this current sensor uses a 1:2000 conversion
  return current;
}

//reads current from second hall effect sensor (milliAmps)
long readC2() {
  long cRead = analogRead(C2);
  long gndRead = analogRead(CGND);
  long c1 = cRead * 5 * 1000 / 1023;
  long cGND = gndRead * 5 * 1000 / 1023;
  long current = 40 * (c1 - cGND); //Scaled over 25 ohm resistor. multiplied by 1000 V -> mV conversion
                                   //this current sensor uses a 1:1000 conversion
  return current;
}


/* Turn of car if Off Switch is triggered */
char checkOffSwitch(){
      //normal shutdown initiated
      if (digitalRead(IO_T1) == HIGH) {
        if (state!=TURNOFF){ 
          Serial.print("key detected in off position\n");
          delay(50); //
        }
        if (digitalRead(IO_T1) == HIGH) { //double check.  
            //Had issues before when releasing key from precharge
            state = TURNOFF;            
            shutdownReason=KEY_OFF;
            //Serial.print("Normal Shutdown\n");
            return 1;
        }
      }
      return 0;
}

//checks to make sure system voltage (and current) are in acceptable levels
//current currently disabled, relies on fuse instead
void checkReadings() {
  CanMessage msg = CanMessage();
  long batteryV = readV1();
  long motorV = readV2();
  long batteryC = readC1();
  long otherC = readC2();
  long undervoltage = 94500;  //90,000 mV
  long overvoltage = 143500; //140,000 mV
  long overcurrent1 = 60000; //60,000 mA
  long overcurrent2 = 15000; //15,000 mA
  if (batteryV <= undervoltage) {
    shutdownReason=S_UNDERVOLT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x02;
    Can.send(msg);
  }
  else if (batteryV >= overvoltage || motorV >= overvoltage) {
    shutdownReason=S_OVERVOLT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x01;
    Can.send(msg);
  }
/*  else if (batteryC >= overcurrent1 || otherC >= overcurrent2) {
    shutdownReason=S_OVERCURRENT;
    state = ERROR;
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x04;
    Can.send(msg);
  }*/ //disabled until current sensors reliable
}

//encodes two flots into the proper format to send over CAN
void floatEncoder(CanMessage &msg,float f1, float f2) {
  /*float *floats = (float*)(msg.data);
  floats[0] = f1;
  floats[1] = f2;
  */
  msg.data[0] = *((char *)&f1);
  msg.data[1] = *(((char *)&f1)+1);
  msg.data[2] = *(((char *)&f1)+2);
  msg.data[3] = *(((char *)&f1)+3);
  msg.data[4] = *((char *)&f2);
  msg.data[5] = *(((char *)&f2)+1);
  msg.data[6] = *(((char *)&f2)+2);
  msg.data[7] = *(((char *)&f2)+3);
}

//sends system voltage readings over CAN
void sendVoltages(){
  CanMessage msg = CanMessage();
  long v1 = readV1(); //get Voltage 1 (in mV)
  long v2 = readV2(); //get Voltage 2 (in mV)
  float mvolt1 = v1; //convert to float (still in mV)
  float mvolt2 = v2; //convert to float (still in mV)
  float volt1=mvolt1/1000; //convert to Volts from mV
  float volt2=mvolt2/1000; //convert to Volts from mV
  
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
  /*
  msg.data[0] = volt1 & 0x00FF;
  msg.data[1] = (volt1 >>8 ) & 0x00FF;
  msg.data[2] = (volt1 >>16) & 0x00FF;
  msg.data[3] = (volt1 >>24) & 0x00FF;
  msg.data[4] = volt2 & 0x00FF;
  msg.data[5] = (volt2 >>8 ) & 0x00FF;
  msg.data[6] = (volt2 >>16) & 0x00FF;
  msg.data[7] = (volt2 >>24) & 0x00FF;
  */
  Can.send(msg);
  
}

//sends system current readings over CAN
void sendCurrents(){
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
  /*
  msg.data[0] = curr1 & 0x00FF;
  msg.data[1] = (curr1 >>8 ) & 0x00FF;
  msg.data[2] = (curr1 >>16) & 0x00FF;
  msg.data[3] = (curr1 >>24) & 0x00FF;
  msg.data[4] = curr2 & 0x00FF;
  msg.data[5] = (curr2 >>8 ) & 0x00FF;
  msg.data[6] = (curr2 >>16) & 0x00FF;
  msg.data[7] = (curr2 >>24) & 0x00FF;
  */
  Can.send(msg);
}

/* Send system voltage/current over CAN */
void sendReadings() {
  sendVoltages();
  sendCurrents();  
}


//sends a cutoff heartbeat over CAN
void sendHeartbeat() {
  CanMessage msg;
  msg.id = CAN_HEART_CUTOFF;
  msg.len = 1;
  msg.data[0] = 0x00;
  Can.send(msg);
}

//initialize all variables to ensure the car always starts up/restarts in the same state.
void initVariables(){
  last_readings = 0;
  last_heart_bps = 0;
  last_heart_driver_io = 0;
  last_heart_driver_ctl = 0;
  last_heart_telemetry = 0;
  last_heart_datalogger = 0;
  last_heart_cutoff = 0;
  last_can = 0;
  emergency = 0;
  warning = 0;
  bps_code = 0;
  shutdownReason = CODING_ERROR; //if no other reason is specified, the code on cutoff has an error
  playingError = false;
  prechargeTime =0;
  startTime=millis();
  precharge_voltage_reached = false;
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
  int prechargeTarget = 90; //~100V ?
  int voltageDiff= abs(prechargeV-batteryV);
  
  if ( checkOffSwitch() ) {
    /* Off switch engaged, Transition to off */
    state = TURNOFF; //actually redundant
    return;
  }
  else if ((prechargeV < prechargeTarget)  || (voltageDiff>3) 
      || (millis()-startTime < 1000)) { //wait for precharge to bring motor voltage up to battery voltage  
    /* Precharge incomplete */
    
    digitalWrite(LED1, HIGH);
    #ifdef DEBUG
      Serial.print("Precharge State -- Motor Voltage: ");
      Serial.print(prechargeV, DEC);
      Serial.print("V  Battery Voltage: ");
      Serial.print(batteryV, DEC);
      Serial.print("V\n");
      delay(100);
    #endif
    state = PRECHARGE;
  }
  else { //precharge voltage has been reached, wait for CAN   
    
      if(!precharge_voltage_reached){  //if this is the first time we've reached the precharge voltage
        /* Precharge complete */
          /* do once */
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
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
        precharge_voltage_reached = true; //set flag true to show we've turned on system
        prechargeTime=millis();
      }
     
      
     if(!last_heart_bps) { //initial wait for precharge is 10 seconds
       long waitingTime =millis()-prechargeTime;
       if(waitingTime>10000){ //10 second timeout
         shutdownReason=BPS_HEARTBEAT;
         state=ERROR;
         return;       
       }
       else{
        #ifdef DEBUG_CAN
          Serial.print("Last can: ");
          Serial.println(last_can);     
        #endif
        delay(10); 
       }
     }
     else{ //if bps heartbeat received
      /* Sound buzzer */
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(50);
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(50);
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);  
      
      digitalWrite(LED2, LOW);
      /* State Transition */
      state = NORMAL;
    }
  }
}





/* Car running state, check for errors, otherwise continue operation */
void do_normal() {
  #ifdef DEBUG_STATES
    Serial.println("car on");
  #endif
  CanMessage msg = CanMessage();
  lastState=NORMAL;
  
  volatile unsigned long LastHeartbeat= last_heart_bps;
  volatile unsigned long timeNow = millis();
  volatile long timeLastHeartbeat = timeNow - LastHeartbeat;
  
  
  if ( emergency ) { //check for emergency
    state = ERROR;
    //msg.id = CAN_EMER_CUTOFF;
    //msg.len = 1;
    //msg.data[0] = 0x08;
    //Can.send(msg);
	return;
  } else if ( checkOffSwitch() ) { //check for off switch
    #ifdef DEBUG
      Serial.println("Switch off. Normal -> Turnoff");
    #endif
    /* Check if switch is on "On" position */
    state = TURNOFF;
	return;
  } else if ( timeLastHeartbeat > 1000) { //check for missing bps heartbeats
    /* Did not receive heartbeat from BPS for 1 second */
    msg.id = CAN_EMER_CUTOFF;
    msg.len = 1;
    msg.data[0] = 0x10;
    Can.send(msg);
    if (Can.rxError() > 20) {
      shutdownReason = BAD_CAN;
    } else {
      shutdownReason = BPS_HEARTBEAT;
    }
    state = ERROR;
    //Serial.print("Last heartbeat:");
    //Serial.println(LastHeartbeat); 
    //Serial.print("time now:");   
    //Serial.println(timeNow);
    //Serial.print("time since heartbeat:");
    //Serial.println(timeLastHeartbeat);    
  } else if (warning){ //check for level 1 warning
      raiseWarning(warning);
  }
  playSongs();
}

//prints out the last shutdown reason
void lastShutdownReason(){
  unsigned int memoryIndex = EEPROM.read(0);
  if (memoryIndex > 50){//in case EEPROM data is bad
    memoryIndex=1;
  }
  Serial.print("Last ");
  unsigned int lastReason = EEPROM.read(memoryIndex);
  printShutdownReason(lastReason);
  
}

//prints out the last 50 shutdown reasons
void shutdownLog(){
  unsigned int memoryIndex = EEPROM.read(0);
  if (memoryIndex>50 || memoryIndex ==0){ //invalid value, start at 1
    memoryIndex = 1;
  }
  memoryIndex--; //Start with the last shutdown reason, not the currently prepped one.  
  for(int i=1;i<=49; i++){ 
    if (memoryIndex<1){
      memoryIndex=50;
    }

    unsigned int lastReason = EEPROM.read(memoryIndex);
    printShutdownReason(lastReason);
    memoryIndex--;
  }
}

//move to next index for shutdown reason
void prepShutdownReason(){ //increments position in the shutdown log
  unsigned int memoryIndex = EEPROM.read(0); 
  memoryIndex++; //go to next unused memory position
  if (memoryIndex > 50){ memoryIndex =1;} //valid index positions 1 - 50
  EEPROM.write(0, memoryIndex); //record current position in error log
  EEPROM.write(memoryIndex, POWER_LOSS); //if no other shutdown reason specified, assume power loss.
}

//writes the current shutdown reason to the shutdown log in EEPROM
void recordShutdownReason(){ //requires prepShutdownReason to increment memory Index.
  unsigned int memoryIndex = EEPROM.read(0); 
  if (memoryIndex > 50){ memoryIndex =1;} //valid index positions 1 - 50
  EEPROM.write(memoryIndex, shutdownReason); //record shutdown reason
}

/* Turn the car off normally */
void do_turnoff() {
  #ifdef DEBUG_STATES
    Serial.println("car off");
  #endif
  if (lastState != TURNOFF){
    lastState=TURNOFF;
    Can.send(CanMessage(CAN_CUTOFF_NORMAL_SHUTDOWN));
    /* Will infinite loop in "off" state as long as key switch is off */
    
    recordShutdownReason();
    printShutdownReason(shutdownReason);
    
  }
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
    //state = PRECHARGE;  //included in initialize function
  }   
  
}

/* Something bad has happened, we must beep loudly and turn the car off */
/* this code must never allow to change the state, forcing a hard reset of the car */
void do_error() {
  if (lastState!=ERROR){  //do these only when entering the error state
    //turn off relays first
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    digitalWrite(RELAY3, LOW);
    digitalWrite(LVRELAY, LOW);
    digitalWrite(LEDFAIL, HIGH);
    
    if (shutdownReason==BPS_EMERGENCY_OTHER){  //If the shutdown reason is a BPS emergency determine which type
      #ifdef DEBUG
         Serial.print("\tBPS Code: ");  
         Serial.println(bps_code & 0xFF, HEX); //print identifier of BPS error
      #endif
      if (bps_code == 0x01) //battery overvoltage case
        shutdownReason=BPS_OVERVOLT;
      else if (bps_code == 0x02) //battery undervoltage case
        shutdownReason=BPS_UNDERVOLT;
      else if (bps_code == 0x04) //battery overtemperature case
        shutdownReason=BPS_OVERTEMP;
      else                      //other BPS emergency, unknown.
       shutdownReason=BPS_EMERGENCY_OTHER;      
    }
    
    if (shutdownReason==BPS_HEARTBEAT ){ //If we shut down from missing BPS heartbeat play bad romance
      loadBadRomance();
    }
    else if (shutdownReason == BAD_CAN) { //if we shut down due to CAN communication errors, beep 3 times
      loadBadCan();
    }
    else{  //all other reasons we'll just beep/play tetris    
      digitalWrite(BUZZER, HIGH);  //for people who can't tell difference between bad romance and tetris
      //loadTetris(); 
    }
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
  lastState=ERROR;
  state=ERROR; //ensure we will not leave the error state
}

//reads from EEPROM the last BPS warning that was received.   This is useful
//for debugging via USB when the warning noise is heard.
void printLastWarning(){
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

//interprets any messages received over the USB serial line.
inline void processSerial(){
  /*Serial commands include "l" for shutdown error log of last 50 shutdown reasons
  and "w" for the last battery warning message received by the board.*/
  if(Serial.available()){
    char letter= Serial.read();
    if(letter=='l'){
      shutdownLog();
    }
    if(letter=='w'){
      printLastWarning();
    }
  }
}

//handles sending out all CAN messages from the cutoff
inline void sendCutoffCAN(){
  if (millis() - last_heart_cutoff > 200) {  //Send out the cutoff heartbeat to anyone listening.  
    last_heart_cutoff = millis();
    sendHeartbeat();
  }
  if (millis() - last_readings > 103){  // Send out system voltage and current measurements
                                        //chose a weird number so it wouldn't always match up with the heartbeat timing    
    last_readings = millis();
    sendReadings();
  }
  #ifdef DEBUG_CAN
    if (millis()-last_printout > 1000){//print out number of heartbeats received each second
      Serial.print("BPS heartbeats/sec: ");
      Serial.println(numHeartbeats);
      numHeartbeats=0;
      last_printout=millis();
    }
  #endif //debug_can
}

#endif //_CUTOFF_HELPER_H_
