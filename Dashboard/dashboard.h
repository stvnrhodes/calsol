/* CalSol - UC Berkeley Solar Vehicle Team 
 * dashboard.h - Dashboard Module
 * Purpose: Header file for the dashboard IO board.
 * Author(s): Ryan Tseng, Steven Rhodes.
 * Date: Oct 3rd 2011
 */

// #define CRUISE_DEBUG

/* User constants */
#define ACCEL_THRESHOLD_LOW    1000
#define ACCEL_THRESHOLD_HIGH   40
#define BRAKE_THRESHOLD_LOW    970
#define BRAKE_THRESHOLD_HIGH   520
#define LIGHT_BLINK_PERIOD     512
#define CRUISE_SPEED_INCREMENT 0.1
#define CRUISE_TORQUE_SETTING  0.7

// The length of time to flash for the error LEDs
#define SHORT_FLASH_TIME  80
#define MIDDLE_FLASH_TIME 200
#define LONG_FLASH_TIME   1000

// Above this speed, the car will not change from FORWARD to NEUTRAL or REVERSE
#define MAX_STATE_CHANGE_SPEED 2
// This speed is the minimum speed needed to enable cruise control
#define MIN_CRUISE_CONTROL_SPEED 2
// This is the fastest speed in m/s we can tell the Tritium to go
#define MAX_TRITIUM_SPEED 100.0

/* State constants */
enum states_enum {
  FORWARD,
  REVERSE,
  NEUTRAL,
  CRUISE,
  ERROR
};

// Health status
enum status_enum {
  OKAY_STATUS,
  ERROR_STATUS,
  CAN_ERROR_STATUS
} status = OKAY_STATUS;

/* Global variables */
volatile float current_speed = 0.0;  // Measured speed.
// Last time we received a speed packet.
volatile unsigned long last_updated_speed = 0;  
float set_speed = 0.0;      // Desired speed based on cruise control in m/s.
boolean regen_on = false;          // Flag to set if regen braking is enabled.
volatile char tritium_reset = 0;       // Number of times to reset the tritium.
// This is used to scale down our max output if an OC error occurs
float overcurrent_scale = 1.0;
volatile unsigned int tritium_limit_flags = 0x00;  // Tritium status from CAN
volatile unsigned int tritium_error_flags = 0x00;  // Tritium errors from CAN

// Possible TODO
/*typedef struct {
  byte hazard_state: 1;
  byte left_state: 1;
  byte right_state: 1;
  byte brake_state: 1;
  byte horn_state: 1;
} blinker_states;*/
// Blinker states
char hazard_state = false;
char left_state   = false;
char right_state  = false;

// Other 12V output states
char brake_state  = false;
char horn_state   = false;

// Global timekeeping variables
unsigned long last_sent_tritium = 0;
unsigned long last_debug_cycle = 0;
unsigned long last_auxiliary_cycle = 0;
unsigned long last_status_blink = 0;
// Time since the last Overcurrent Error, in millis()
volatile unsigned long time_of_last_oc = 0;

/* Helper functions */
// 8 chars and 2 floats sharing the same space. Used for converting two floats
// into a format that we can use to send CAN data.
typedef union {
  char  c[8];
  float f[2];
  unsigned int i[2];
} packed_data;

/***
 * ISR for reading Tritium speed readings off CAN
 */
void processCan(CanMessage &msg) {
#ifdef CAN_DEBUG
  Serial.print("CAN: ");
  Serial.println(msg.id, HEX);
#endif
  const packed_data *data = (packed_data *) msg.data;
  if (msg.id == CAN_TRITIUM_VELOCITY) {
    last_updated_speed = millis();
    current_speed = data->f[1];
#ifdef VERBOSE
    Serial.println("Tritium Speed: ");
    Serial.println(current_speed);
#endif
  } else if (msg.id == CAN_TRITIUM_STATUS) {
    tritium_limit_flags = data->i[0];
    tritium_error_flags = data->i[2];
    if (msg.data[2] & 0x02) {
      #ifdef ERRORS
        Serial.println("Warning: Overcurrent Error");
      #endif
      // We have an overcurrent error from the Tritium, better raise a flag.
      tritium_reset = 2;
    }
  }
}

boolean isCruisePressed(void) {
  static boolean state = false;
  static boolean prev_reading = HIGH;
  boolean current_reading = digitalRead(IN_CRUISE_ON);
  if (current_reading == prev_reading) {
    // Only update the state if we have two identical readings in a row
    current_reading == HIGH ? state = false : state = true;
  }
  prev_reading = current_reading;
  return state;
}

/***
 * Keep track of timing for blinking lights such as hazard lights and turn
 * signal
 */
unsigned long _last_lights_blinked = 0;

void resetBlinkingLightsTimer() {
  _last_lights_blinked = millis();
}

char isBlinkingLightsOn() {
  unsigned long current_time = millis();
  // Check if the light timer has been on for a second or more
  if (current_time - _last_lights_blinked > LIGHT_BLINK_PERIOD * 2 ||
      current_time < _last_lights_blinked) { // Edge case
    _last_lights_blinked = current_time;
    return true;
  }
  // Check if the light timer has been on for between half a second and a second
  if (current_time - _last_lights_blinked > LIGHT_BLINK_PERIOD) {
    return false;
  }
  // The light is on if it's been less than half a second
  return true;
}
  
/***
 * Increments or decrements the speed of the cruise control based on the three
 * way momentary switch position.  Each drive cycle will increase or decrease
 * speed by a set.
 */
float adjustCruiseControl(float speed) {
  if (!digitalRead(IN_CRUISE_ACC)) {
    speed += CRUISE_SPEED_INCREMENT;
  } else if (!digitalRead(IN_CRUISE_DEC)) {
    speed -= CRUISE_SPEED_INCREMENT;
  }
  if (speed < 0) {
    speed = 0;
  }
  return speed;
}


/***
 * Change light and horn state based on switches.
 */
// TODO: Hazard latching
void updateAuxiliaryStates() {
  if (!digitalRead(IN_HAZ_SWITCH)){// || status != OKAY_STATUS) {
    // Hazard lights blink if Haz switch is on, or if the car is not okay.
    if (!hazard_state) {
      // If this state is off...
      hazard_state = true;
      left_state   = false;
      right_state  = false;
      resetBlinkingLightsTimer();
    }
  } else if (!digitalRead(IN_LTURN_SWITCH)) {
    // Left turn lights
    if (!left_state) {
      // If this state is off...
      hazard_state = false;
      left_state   = true;
      right_state  = false;
      resetBlinkingLightsTimer();
    }
  } else if (!digitalRead(IN_RTURN_SWITCH)) {
    // Right turn lights
    if (!right_state) {
      // If this state is off...
      hazard_state = false;
      left_state   = false;
      right_state  = true;
      resetBlinkingLightsTimer();
    }
  } else {
    // Nothing is on, let's turn all the lights off.
    hazard_state = false;
    left_state   = false;
    right_state  = false;
  }
}

/***
 * Turns blinkers and horn on and off based on the current state.
 */
void auxiliaryControl() {
  // Turn blinkers on and off.
  if (hazard_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, true);
    digitalWrite(OUT_RTURN, true);
    digitalWrite(OUT_LTURN_INDICATOR, true);
    digitalWrite(OUT_RTURN_INDICATOR, true);
  } else if (left_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, true);
    digitalWrite(OUT_LTURN_INDICATOR, true);
    digitalWrite(OUT_RTURN, false);
    digitalWrite(OUT_RTURN_INDICATOR, false);
  } else if (right_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, false);
    digitalWrite(OUT_LTURN_INDICATOR, false);
    digitalWrite(OUT_RTURN, true);
    digitalWrite(OUT_RTURN_INDICATOR, true);
  } else {
    // Turn all lights off.
    digitalWrite(OUT_LTURN, false);
    digitalWrite(OUT_RTURN, false);
    digitalWrite(OUT_LTURN_INDICATOR, false);
    digitalWrite(OUT_RTURN_INDICATOR, false);
  }
  
  
  // Turn on horn if the horn button is pressed.
  horn_state = !digitalRead(IN_HORN_BUTTON);
  digitalWrite(OUT_HORN, horn_state);
}

/***
 * Blinks the BRAIN debug LED to indicate any status errors.
 * If okay, do not blink
 * If general error, blink at 5Hz
 * If CAN error, blink at 15Hz
 */
void blinkStatusLED() {
  if (Can.rxError() > 50) {
    status = CAN_ERROR_STATUS;
  } else if (millis() - last_updated_speed > 1200) {
    status = ERROR_STATUS;
  } else {
    status = OKAY_STATUS;
  }
  if (status == OKAY_STATUS) {
    if (millis() - last_status_blink > LONG_FLASH_TIME) {
      last_status_blink = millis();
      digitalWrite(OUT_BRAIN_LED, 0);
    }
  } else if (status == CAN_ERROR_STATUS) {
    if (millis() - last_status_blink > SHORT_FLASH_TIME) {
      #ifdef ERRORS
        Serial.println("CAN error!");
      #endif
      last_status_blink = millis();
      digitalWrite(OUT_BRAIN_LED, !digitalRead(OUT_BRAIN_LED));
    }
  } else {
    if (millis() - last_status_blink > MIDDLE_FLASH_TIME) {
      #ifdef ERRORS
        Serial.println("Error: Cannot get Tritium speed");
      #endif
      last_status_blink = millis();
      digitalWrite(OUT_BRAIN_LED, !digitalRead(OUT_BRAIN_LED));
    }
  }
}

/***
 * Sends a drive command packet to the Tritium motor controller.  Velocity is
 * in meters per second, and current is a float between 0.0 and 1.0.
 */
void sendDriveCommand(const float motor_velocity, const float motor_current) {
  packed_data data;
  data.f[0] = motor_velocity;
  data.f[1] = motor_current;
  #ifdef VERBOSE
    Serial.print("CAN Packet To Tritium, Velocity ");
    Serial.print(motor_velocity);
    Serial.print(", Current");
    Serial.println(motor_current);
  #endif
  CanMessage msg = CanMessage(CAN_TRITIUM_DRIVE, data.c);
  Can.send(msg);
}

/***
 * Sends a reset packet to the Tritium.  This is called whenever there is an
 * overcurrent error.
 */
void resetTritium() {
  char data[8];
  Can.send(CanMessage(CAN_TRITIUM_RESET, data, 8));
  #ifdef ERRORS
    Serial.print("Sending Reset. Limit: ");
    Serial.print(tritium_limit_flags, HEX);
    Serial.print(" Error: ");
    Serial.println(tritium_error_flags, HEX);
  #endif
  if (overcurrent_scale > 0.6 && millis() - time_of_last_oc > 50) {
   overcurrent_scale -= 0.02;
  }
  time_of_last_oc = millis();
}


/***
 * Updates whether or not the car is in cruise control.
 */
boolean getCruiseState(const states_enum old_state, const float brake) {
  static boolean last_button_state = false;
  boolean cruise_pressed = isCruisePressed();
#ifdef CRUISE_DEBUG
  Serial.print("old_state: ");
  Serial.print(old_state, DEC);
  Serial.print(", last_button_state: ");
  Serial.print(last_button_state, DEC);
  Serial.print(", cruise_pressed: ");
  Serial.println(cruise_pressed, DEC);
#endif
  if (old_state == CRUISE && brake > 0.02) {
#ifdef CRUISE_DEBUG
    Serial.println("Braking, so disabling cruise control")
#endif
    digitalWrite(OUT_CRUISE_INDICATOR, false);
    return false;
  }
 // If the button was just pressed
  if (!last_button_state && cruise_pressed) {
    last_button_state = cruise_pressed;
    // If we're not in cruise, try to turn it on
    if (old_state != CRUISE) {
      // We'll turn on cruise if these conditions are met
      if (current_speed > MIN_CRUISE_CONTROL_SPEED && old_state == FORWARD) {
        digitalWrite(OUT_CRUISE_INDICATOR, true);
        return true;
      }
      return false;
    // Otherwise, we're already in cruise and want to turn it off
    } else {
      digitalWrite(OUT_CRUISE_INDICATOR, false);
      return false;
    }
  } else {
    // No change
    last_button_state = cruise_pressed;
    return (old_state == CRUISE);
  }
}


/***
 * Updates the current state of the car (Forward, Reverse, Neutral) based on
 * switches.
 */
states_enum getDrivingState(const states_enum old_state, const float brake) {

  states_enum switch_state;
 
  // First check for cruise
  boolean cruise_on = getCruiseState(old_state, brake);
  if (cruise_on) {
    switch_state = CRUISE;
  // Then, set switch_state to what the current switch is set at
  } else if (!digitalRead(IN_VEHICLE_FWD)) {
    switch_state = FORWARD;
  } else if (!digitalRead(IN_VEHICLE_REV)) {
    switch_state = REVERSE;
  } else {
    switch_state = NEUTRAL;
  }
  
  // TODO(stvn): Refactor to remove global variable
  regen_on = !digitalRead(IN_REGEN_SWITCH);
  return switch_state;

// I've disabled the code below to see if anybody notices  
//  if (switch_state != state && current_speed < MAX_STATE_CHANGE_SPEED) {
    // Only switch states if the car is going at less than 10 mph.
//    return switch_state;
//  }
//  return old_state;
}

void createDriveCommands(const states_enum state, const float accel,
    const float brake) {
  switch (state) {
    case CRUISE:
      if (brake > 0.0) {
        // If the brakes are tapped, cut off acceleration
        if (regen_on) {
          sendDriveCommand(0.0, brake);
        } else {
          sendDriveCommand(0.0, 0.0);
        }
      } else {
        set_speed = adjustCruiseControl(set_speed);
#ifdef CRUISE_DEBUG
        Serial.print("Go ");
        Serial.println(set_speed);
#endif
        sendDriveCommand(set_speed, CRUISE_TORQUE_SETTING);
      }
      break;
    case FORWARD:
      if (brake > 0.0) {
        // If the brakes are tapped, cut off acceleration
        if (regen_on) {
          sendDriveCommand(0.0, brake);
        } else {
          sendDriveCommand(0.0, 0.0);
        }
      } else if (accel == 0.0) {
        // Accelerator is not pressed
        sendDriveCommand(0.0, 0.0);
      } else {
        sendDriveCommand(MAX_TRITIUM_SPEED, accel);
      }
      break;
    case REVERSE:
      if (brake > 0.0) {
        // If the brakes are tapped, cut off acceleration
        if (regen_on) {
          sendDriveCommand(0.0, brake);
        } else {
          sendDriveCommand(0.0, 0.0);
        }
      } else {
        sendDriveCommand(-MAX_TRITIUM_SPEED, accel);
      }
      break;
    case NEUTRAL:
      sendDriveCommand(0.0, 0.0);
      break;
    default:
      break;
  }  
}

float getPedal(const byte port, const int lower, const int upper) {
  // Read raw pedal values
  int input_raw = analogRead(port);
#ifdef VERBOSE
  Serial.print("pedal reading: ");
  Serial.print(input_raw);
  Serial.print(" on ");
  Serial.println(port);
#endif

  // Map values to 0.0 - 1.0 based on thresholds
  int constrained = constrain(input_raw,
      min(lower, upper),
      max(lower, upper));

  float pedal = map(constrained, lower, upper, 0, 1000) / 1000.0;

              
  // In case we get overcurrent errors, reduce the power we send.
  pedal *= overcurrent_scale;
  return pedal;
}

/***
 * Calls a single routine of the driver routine loop.  This function reads
 * pedal inputs, adjusts for thresholds, and send CAN data to the Tritium.
 */
states_enum driverControl() {

  float accel = getPedal(ANALOG_ACCEL_PEDAL, ACCEL_THRESHOLD_LOW,
      ACCEL_THRESHOLD_HIGH);
  float brake = getPedal(ANALOG_BRAKE_PEDAL, BRAKE_THRESHOLD_LOW,
      BRAKE_THRESHOLD_HIGH);

  // Turn on brake lights if the brake pedal is stepped on.
  // TODO(stvn): Figure out a better place to put this
  brake_state = brake > 0.0;
  digitalWrite(OUT_BRAKELIGHT, brake_state);

  
  // Send CAN data based on current state.
  static states_enum state = NEUTRAL;
  state = getDrivingState(state, brake);
  createDriveCommands(state, accel, brake);
  return state;
}

