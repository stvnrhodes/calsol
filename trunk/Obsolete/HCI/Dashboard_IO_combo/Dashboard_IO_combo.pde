//Now called Main Input Board

//recent change: added !digitalRead to accel() code to read VEHICLE_FWD and VEHICLE_REV correctly
boolean debug = false;



#define HAZ_SWITCH 10 //switched from STROBE_SWITCH
#define HORN_BUTTON 11
#define LTURN_SWITCH 18
#define RTURN_SWITCH 17
#define LTURN_INDICATOR 24
#define RTURN_INDICATOR 23
#define VEHICLE_FWD 19 
#define VEHICLE_REV 21
#define REGEN_SWITCH 20
#define HORN 30
#define LTURN 31
#define RTURN 0
#define BRAKELIGHT 1
#define CAMERA 2
/* 
 * Cruise control pins. Pin 15 was originally the CRUISE_SET pin, but after finding that this
 * pin connects to an LED, this was switched to pin 16, swapped conveniently with the CRUISE_IND
 * pin.
 */
#define CRUISE_SET 16 // Sets cruise on, or off.
#define CRUISE_DEC 25 // Rocker switch pushed to decrease speed.
#define CRUISE_ACC 26 // Rocker switch pushed to increase speed.
#define CRUISE_IND 15 // The light in the cruise switch to indicate state.

#define ACCEL_PEDAL 27
#define BRAKE_PEDAL 28

#define ACCEL_THRESHOLD 100
#define BRAKE_THRESHOLD 20
//User-Defined Constants
#define INTERVAL 500           // interval at which to blink (milliseconds)
#define MAX_SPEED 100

//Our necessary CAN Ids
#define outputID 0x481
#define TritiumMotor 0x501
#define heartbeatID 0x044
byte PEDALHIGHBYTE[2] = {0xbe,0xf3};

char inputMessage[8] = {0,0,0,0,0,0,0,0}; //this will transfer out inputs over
//0:RTurn 1:LTurn 2:Strobe(might NOT be used NOW) 3:Horn 4:brake 5:reverse 6:
char motorMessage[8] = {0,0,0,0,0,0,0,0};
float setspeed = 0.0;
float cruiseSpeed = 0.0;
float recordedSpeed = 15.0;
float voltage = 1.0;
boolean km = true;
//cruisemode defines
boolean cruise_active = false;   // Is cruise on?
int can_count = 0;  // To count number of times can buffer is read
float minimum_cruise_speed = 3.0; // Speed, in miles per hour.
           // MUST be same units as internal speed setting
           // MUST be same units as cruise speed
float cruise_speed = 0.0;  // Speed desired to hold. MUST be same units as internal speed setting

long millis_since_cruise_toggle; // The time, in milliseconds, since the cruise button was pressed
long minimum_millis_change_delay = 500; // Time, in milliseconds, the user needs to wait before
                //  changing the state of the cruise control.
boolean brakeOn = false;
char buff[]= "0000000000";
boolean rightSet = false;
boolean leftSet = false;



void turnSignals(boolean rTurnSwitchstate, boolean lTurnSwitchstate);

void initPins(void);
void floatEncoder(CanMessage &msg,float spd, float v);

void setup() {
  Can.begin(1000);
 
  initPins();

  CanMessage init = CanMessage();
  init.id = 0x503;
  init.len = 0;
  Can.send(init);
  init = CanMessage();
  init.id = 0x502;
  init.len = 8;
  floatEncoder(init,1.0,1.0);
  Can.send(init);

}

void loop() {
 

  setInputs();
  accel();
 cruiseControl(); // cruise control overrides measurements made by accel
  COMToOthers();
  if (!digitalRead(HAZ_SWITCH)) {
    turnSignals(true,true);
    rightSet = true;
    leftSet = true; 
  } else {
     rightSet = (digitalRead(RTURN_SWITCH) == LOW);                     
     leftSet = (digitalRead(LTURN_SWITCH) == LOW);
     turnSignals(rightSet, leftSet); 
  }


}


/********************************************
accel()
This function will read in the current input
note analog pin can only be read every 100 microseconds
********************************************/
int brakeSamples = 1;
int accelSamples = 1;

void accel(){
  int accel = analogRead(ACCEL_PEDAL);
  
  int brake = analogRead(BRAKE_PEDAL);
  brake = 1023-brake;
  brake = brake * 1.75;
  if (brake > 1023)
    brake = 1023;
  if(cruise_active){
    if(brake > BRAKE_THRESHOLD) {
      cruiseCancel();
    } else{
       return;
    }
  }
  if(brake > BRAKE_THRESHOLD) {
    setspeed = 0.0;
    brakeOn = true;
	if (digitalRead(REGEN_SWITCH)){
		voltage = 0;
	}
	else{
    voltage = brake/1023.0;
	}
    return;
  }
  if(!digitalRead(VEHICLE_REV)&&!digitalRead(VEHICLE_FWD)) {
    brakeOn = false;
    cruiseCancel(); 
    setspeed = 0;
    voltage = 0.0;
    return;
  }
  brakeOn = false;
  if(accel < ACCEL_THRESHOLD) {
    setspeed = 0.0;
    voltage = 0.0;
    return;
  }
  if(!digitalRead(VEHICLE_REV)) {
    cruiseCancel();
    setspeed = -100.0;
    voltage =  accel/1023.0;
    return;
  }  
  
cruiseCancel();  
  setspeed = 100.0;
  voltage = accel/900.0;
  return;
}
/********************************************


/*********************************************
cruiseMode()
This procedure reads the value of the cruise sswitch and sets the mode and speed accordingly.
**********************************************/

boolean firstSet = true;
void cruiseSetMode() {
  /*
   * If the cruise is turned on, but the recorded speed is less than the speed
   *   maintainable by the car, or the brake is being pressed, cancel cruise.
   */
  // where rval represents the speed of the car
  if (cruise_active && (recordedSpeed < minimum_cruise_speed || brakeOn||digitalRead(VEHICLE_FWD))) {
    cruiseCancel(5);
    return;
  }
  // Read the state of the cruise switch
  int state = !digitalRead(CRUISE_SET);
  /* 
   * If the button is being pressed, and enough time has elapsed since the last time we've
   *   pressed the button, then toggle the cruise mode. (This may set it to true OR false.)
   */
  if (!digitalRead(VEHICLE_FWD)&&state == LOW && millis() - millis_since_cruise_toggle > minimum_millis_change_delay) {
    toggleCruise();
    // voltage to 1, set speed to whatever you want
    // Update the number of milliseconds since we've pressed the button.
    millis_since_cruise_toggle = millis();
    // Act based on the status of cruise when we push the button
    if (cruise_active) {
      cruiseSet();
    } else {
      // If the cruise was actually turned off, then cancel here.
      cruiseCancel(6);
    }
  }
  if (cruise_active) {
    maintainSpeed(); 
  }
}

/* 
 * This is the ONLY mehtod where cruise_active should be changed to
 *   where it can _possibly_ be true. 
 */
void toggleCruise() {

//  Serial.println("cruise activated");
   cruise_active = !cruise_active; 
}

/* 
 * Sets cruise speed. Note that this should not change the cruise to true! 
 */
void cruiseSet() {
  cruise_speed = recordedSpeed;
  //Serial.println(cruise_speed);
}

/* 
 * Maintain the speed for the cruise control.
 */
void maintainSpeed(){
  setspeed = cruise_speed;
  voltage = 0.5;
  
}
/* Cancels cruise. This also resets the firstSet variable. */
void cruiseCancel(int i) {
  cruiseCancel(); 
}

void cruiseCancel() {
  firstSet = true;
  cruise_active = false;
  cruise_speed = 0;
}

/********************************************
cruiseSetSpeed()
This procedure reads the state of the cruise switch state (again) and if it’s set,
change the speed according to whether or not the rocker switch is being pressed.
**********************************************/
long millis_since_cruise_speed_change = 0; // The time, in milliseconds, since cruise speed changed.
float cruise_speed_increment_unit = 0.2;

void cruiseSetSpeed() {
  if (cruise_active) { 
    int acc = digitalRead(CRUISE_ACC);
    if (acc == LOW && millis() - millis_since_cruise_speed_change >
        minimum_millis_change_delay && cruise_active) {
      cruise_speed += cruise_speed_increment_unit;
      millis_since_cruise_speed_change = millis();
    }
    int dec = digitalRead(CRUISE_DEC);
    if (dec == LOW && millis() - millis_since_cruise_speed_change >
        minimum_millis_change_delay && cruise_active) {
      cruise_speed -= cruise_speed_increment_unit;
      millis_since_cruise_speed_change = millis();
    }
  }
}

/********************************************
cruiseControl()
This procedure separates the cruise mode into the on/off state (cruiseSetMode) and the
increase/decrease speed (cruiseSetSpeed)
**********************************************/
void cruiseControl() {
  digitalWrite (CRUISE_IND, cruise_active);
  cruiseSetMode();
  cruiseSetSpeed(); 
}

/********************************************
turnSignals(rTurnSwitch,lTurnSwitch)
This function controls the turn signal lights
when given the states of their switches
********************************************/
static long previousMillis = 0; // will store last time LED was updated
static int lightState = LOW;       // lightState used to set the LED

void turnSignals(boolean rTurnSwitchstate,boolean lTurnSwitchstate){  
  if (!rTurnSwitchstate)
    digitalWrite(RTURN_INDICATOR, LOW);
  if (!lTurnSwitchstate)
    digitalWrite(LTURN_INDICATOR, LOW);
  if ((millis() - previousMillis > INTERVAL)&& ((rTurnSwitchstate||lTurnSwitchstate))) {
    // save the last time you blinked the LED 
    previousMillis = millis();   
    lightState = (lightState == LOW ? HIGH : LOW);
      // set the LED with the lightState of the variable:
    if(rTurnSwitchstate){
      digitalWrite(RTURN_INDICATOR, lightState);
      digitalWrite(RTURN, lightState);
    }
    if(lTurnSwitchstate){
      digitalWrite(LTURN_INDICATOR, lightState);
      digitalWrite(LTURN,lightState);
    }
  }
}

/**********************************
isBitSet(x,pos)
This function will return the value 
of the bit at the given position.
**********************************/
boolean isBitSet(byte x, unsigned char pos){
  return ((x >> pos) & B00000001);
}

/**************************************************
initPins()
This function will initialize all pins 
as inputs.  It is called in setup().

digitalWrite(PIN, HIGH) used to fix floating error
***************************************************/
void initPins(void){

  pinMode (RTURN_SWITCH, INPUT);
  digitalWrite(RTURN_SWITCH, HIGH);
  pinMode (LTURN_SWITCH, INPUT);
  digitalWrite(LTURN_SWITCH, HIGH);
  pinMode(HAZ_SWITCH, INPUT);
  digitalWrite(HAZ_SWITCH, HIGH);
  pinMode(HORN_BUTTON, INPUT);
  digitalWrite(HORN_BUTTON, HIGH);
  pinMode(LTURN_INDICATOR, OUTPUT);
  pinMode(RTURN_INDICATOR, OUTPUT);
  pinMode(CRUISE_SET, INPUT);
  digitalWrite(CRUISE_SET, HIGH);
  pinMode(CRUISE_DEC, INPUT);
  digitalWrite(CRUISE_DEC, HIGH);
  pinMode(CRUISE_ACC, INPUT);
  digitalWrite(CRUISE_ACC, HIGH);
  pinMode(CRUISE_IND, OUTPUT);
  pinMode(VEHICLE_FWD, INPUT);
  digitalWrite(VEHICLE_FWD, HIGH);
  pinMode(VEHICLE_REV, INPUT);
  digitalWrite(VEHICLE_REV, HIGH);
  digitalWrite(ACCEL_PEDAL,LOW);
 digitalWrite(BRAKE_PEDAL, LOW);
 pinMode(REGEN_SWITCH, INPUT);
 digitalWrite(REGEN_SWITCH, HIGH);
 pinMode(HORN, OUTPUT);
 digitalWrite(HORN, LOW);
 pinMode(LTURN, OUTPUT);
 digitalWrite(LTURN, LOW);
 pinMode(RTURN, OUTPUT);
 digitalWrite(RTURN, LOW);
 pinMode(BRAKELIGHT, OUTPUT);
 digitalWrite(BRAKELIGHT, LOW);
 pinMode(CAMERA, OUTPUT);
 digitalWrite(CAMERA, HIGH);

}
        
/************************************
This function will assign all digital
inputs to a single integer as well as
setting both analog inputs. Each bit 
is assigned to an index in the 
inputMessage char array.
0: Right Turn
1: Left Turn
2: Vehicle's Brake Engaged
3: Horn
4: Vehicle in Reverse
*************************************/
void setInputs() {
  digitalWrite(BRAKELIGHT, brakeOn);
  digitalWrite(HORN, !digitalRead(HORN_BUTTON));

}
  
  
int state = 0;
long previousMillisOutput = 0; 
long previousMillisHeart = 0;
long previousMillisDrive = 0;
//sends CAN messages 
void COMToOthers() {
  if(state > 2)
    state = 0;
  CanMessage msg ;
  if (state == 0) {  
    if((millis() - previousMillisOutput > 100)) {
      
    }
  }
  if(state == 1) {
    if(millis() - previousMillisDrive > 100){
      msg = sendMotorControl(); //Allocates space for new CanMessage
      Can.send(msg);
      previousMillisDrive = millis();
    }
  }  
  if(state == 2) {
    if(millis() - previousMillisHeart > 200){
      msg = sendHeartbeat();
      Can.send(msg);
      previousMillisHeart = millis();
    }
  }
  state ++;
}



CanMessage sendHeartbeat() {
  CanMessage outputMsg = CanMessage();
  outputMsg.id = heartbeatID;
  outputMsg.len = 1;
  outputMsg.data[0] = 0;
  return outputMsg;
}

CanMessage sendMotorControl() {
  CanMessage outputMsg = CanMessage();
  outputMsg.id = TritiumMotor;
  outputMsg.len = 8;
  floatEncoder(outputMsg,setspeed,voltage);
  if(debug){
  Serial.print("speed =");
  Serial.println(setspeed);
  Serial.print("voltage =");
  Serial.println(voltage);
  }
  return outputMsg;
}
void floatEncoder(CanMessage &msg,float spd, float v) {
  msg.data[0] = *((char *)&spd);
  msg.data[1] = *(((char *)&spd)+1);
  msg.data[2] = *(((char *)&spd)+2);
  msg.data[3] = *(((char *)&spd)+3);
  msg.data[4] = *((char *)&v);
  msg.data[5] = *(((char *)&v)+1);
  msg.data[6] = *(((char *)&v)+2);
  msg.data[7] = *(((char *)&v)+3);
}


