/* CalSol - UC Berkeley Solar Vehicle Team 
 * dashboard.h - Dashboard Module
 * Purpose: Header file for the dashboard IO board.
 * Author(s): Ryan Tseng, Steven Rhodes.
 * Date: Oct 3rd 2011
 */


/* User constants */
#define ACCEL_THRESHOLD_LOW    1000
#define ACCEL_THRESHOLD_HIGH   40
#define BRAKE_THRESHOLD_LOW    60
#define BRAKE_THRESHOLD_HIGH   480
#define LIGHT_BLINK_PERIOD     512
#define CRUISE_SPEED_INCREMENT 0.1
#define CRUISE_TORQUE_SETTING  0.8

// The length of time to flash for the error LEDs
#define SHORT_FLASH_TIME  80
#define MIDDLE_FLASH_TIME 200
#define LONG_FLASH_TIME   1000

// Above this speed, the car will not change from FORWARD to NEUTRAL or REVERSE
#define MAX_STATE_CHANGE_SPEED 2
// This speed is the minimum speed needed to enable cruise control
#define MIN_CRUISE_CONTROL_SPEED 2
#define VERBOSE
#define OFF   0
#define ON    1
#define FALSE 0
#define TRUE  1

/* State constants */
enum states_enum {
  FORWARD,
  REVERSE,
  NEUTRAL,
  ERROR
} state;

// Health status
enum status_enum {
  OKAY_STATUS,
  ERROR_STATUS,
  CAN_ERROR_STATUS
} status;

/* Global variables */
float accel = 0.0;
float brake = 0.0;
volatile float current_speed = 0.0;  // Measured speed.
// Last time we received a speed packet.
volatile unsigned long last_updated_speed = 0;  
float set_speed = 0.0;      // Desired speed based on cruise control in m/s.
char cruise_on = OFF;         // Flag to set if cruise control is on or off.
char regen_on = OFF;          // Flag to set if regen braking is enabled.
char accel_cruise = OFF;  // Flag to allow using accelerator in cruise control
volatile char tritium_reset = 0;       // Number of times to reset the tritium.
// This is used to scale down our max output if an OC error occurs
float overcurrent_scale = 1.0;
volatile unsigned int tritium_limit_flags = 0x00;  // Status of the tritium from CAN
volatile unsigned int tritium_error_flags = 0x00;  // Errors from tritium from CAN


// Blinker states
char hazard_state = OFF;
char left_state   = OFF;
char right_state  = OFF;

// Other 12V output states
char brake_state  = OFF;
char horn_state   = OFF;

// Global timekeeping variables
unsigned long last_sent_tritium = 0;
unsigned long last_debug_cycle = 0;
unsigned long last_auxiliary_cycle = 0;
unsigned long last_status_blink = 0;
// Time since the last Overcurrent Error, in millis()
volatile unsigned long time_of_last_oc = 0;

// Global debug
volatile unsigned long num_can = 0;

/* Helper functions */
// 8 chars and 2 floats sharing the same space. Used for converting two floats
// into a format that we can use to send CAN data.
typedef union {
  char  c[8];
  float f[2];
} two_floats;

/***
 * ISR for reading Tritium speed readings off CAN
 */
void processCan(CanMessage &msg) {
  num_can++;
  #ifdef CAN_DEBUG
    Serial.print("CAN: ");
    Serial.println(msg.id, HEX);
  #endif
  if (msg.id == CAN_TRITIUM_VELOCITY) {
    last_updated_speed = millis();
    current_speed = ((two_floats*)msg.data)->f[1];
    #ifdef VERBOSE
      Serial.println("Tritium Speed: ");
      Serial.println(current_speed);
    #endif
  } else if (msg.id == CAN_TRITIUM_STATUS) {
    tritium_limit_flags = *((unsigned int*)&msg.data[0]);
    tritium_error_flags = *((unsigned int*)&msg.data[2]);
    if (msg.data[2] & 0x02) {
      #ifdef ERRORS
        Serial.println("Warning: Overcurrent Error");
      #endif
      // We have an overcurrent error from the Tritium, better raise a flag.
      tritium_reset = 2;
    }
  }
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
    return ON;
  }
  // Check if the light timer has been on for between half a second and a second
  if (current_time - _last_lights_blinked > LIGHT_BLINK_PERIOD) {
    return OFF;
  }
  // The light is on if it's been less than half a second
  return ON;
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
      hazard_state = ON;
      left_state   = OFF;
      right_state  = OFF;
      resetBlinkingLightsTimer();
    }
  } else if (!digitalRead(IN_LTURN_SWITCH)) {
    // Left turn lights
    if (!left_state) {
      // If this state is off...
      hazard_state = OFF;
      left_state   = ON;
      right_state  = OFF;
      resetBlinkingLightsTimer();
    }
  } else if (!digitalRead(IN_RTURN_SWITCH)) {
    // Right turn lights
    if (!right_state) {
      // If this state is off...
      hazard_state = OFF;
      left_state   = OFF;
      right_state  = ON;
      resetBlinkingLightsTimer();
    }
  } else {
    // Nothing is on, let's turn all the lights off.
    hazard_state = OFF;
    left_state   = OFF;
    right_state  = OFF;
  }
}

/***
 * Turns blinkers and horn on and off based on the current state.
 */
void auxiliaryControl() {
  // Turn blinkers on and off.
  if (hazard_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, ON);
    digitalWrite(OUT_RTURN, ON);
    digitalWrite(OUT_LTURN_INDICATOR, ON);
    digitalWrite(OUT_RTURN_INDICATOR, ON);
  } else if (left_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, ON);
    digitalWrite(OUT_LTURN_INDICATOR, ON);
    digitalWrite(OUT_RTURN, OFF);
    digitalWrite(OUT_RTURN_INDICATOR, OFF);
  } else if (right_state && isBlinkingLightsOn()) {
    digitalWrite(OUT_LTURN, OFF);
    digitalWrite(OUT_LTURN_INDICATOR, OFF);
    digitalWrite(OUT_RTURN, ON);
    digitalWrite(OUT_RTURN_INDICATOR, ON);
  } else {
    // Turn all lights off.
    digitalWrite(OUT_LTURN, OFF);
    digitalWrite(OUT_RTURN, OFF);
    digitalWrite(OUT_LTURN_INDICATOR, OFF);
    digitalWrite(OUT_RTURN_INDICATOR, OFF);
  }
  
  // Turn on brake lights if the brake pedal is stepped on.
  brake_state = brake > 0.0;
  digitalWrite(OUT_BRAKELIGHT, brake_state);
  
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
void sendDriveCommand(float motor_velocity, float motor_current) {
  two_floats data;
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
 * Updates the current state of the car (Forward, Reverse, Neutral) based on
 * switches.
 */
// TODO: Cruise control
void updateDrivingState() {

  // First, set switch_state to what the current switch is set at
  states_enum switch_state = NEUTRAL;
  if (!digitalRead(IN_VEHICLE_FWD)) {
    switch_state = FORWARD;
  } else if (!digitalRead(IN_VEHICLE_REV)) {
    switch_state = REVERSE;
  }
  
//  regen_on = !digitalRead(IN_REGEN_SWITCH);
regen_on = true; //temporary fix since we are driving without a regen switch.
  
  if (switch_state != state && current_speed < MAX_STATE_CHANGE_SPEED) {
    // Only switch states if the car is going at less than 10 mph.
    state = switch_state;
  }
}

/***
 * Updates whether or not the car is in cruise control.
 */
void updateCruiseState() {
  static char is_cruise_on = FALSE;
  if (current_speed > MIN_CRUISE_CONTROL_SPEED && state == FORWARD &&
      !cruise_on) {
    if (!digitalRead(IN_CRUISE_DEC) || !digitalRead(IN_CRUISE_ON)) {
      // Cruise is pressed, set cruise to whatever speed we are at.
      set_speed = current_speed;
      cruise_on = ON;
      accel_cruise = OFF;
      digitalWrite(OUT_CRUISE_INDICATOR, ON);
    } else if (set_speed != 0.0 && !digitalRead(IN_CRUISE_ACC)) {
      // Resume old cruise speed 
      cruise_on = ON;
      accel_cruise = OFF;
      digitalWrite(OUT_CRUISE_INDICATOR, ON);
    }
  } else if (cruise_on && digitalRead(IN_CRUISE_ON)) {
    // We are currently in cruise
    is_cruise_on = TRUE;
  } else if (is_cruise_on && !digitalRead(IN_CRUISE_ON)) {
    // Cruise is on, so we want to turns it off
    cruise_on = OFF;
    digitalWrite(OUT_CRUISE_INDICATOR, OFF);
  } else {
    is_cruise_on = FALSE;
  }
}


/***
 * Calls a single routine of the driver routine loop.  This function reads
 * pedal inputs, adjusts for thresholds, and send CAN data to the Tritium.
 */
void driverControl() {
  // Read raw pedal values
  int accel_input_raw = analogRead(ANALOG_ACCEL_PEDAL);
  int brake_input_raw = analogRead(ANALOG_BRAKE_PEDAL);
  
  // The raw brake value decreases as you press it, so we reverse it here
  brake_input_raw = 1023 - brake_input_raw;
  // Map values to 0.0 - 1.0 based on thresholds
  int constrained_accel = constrain(accel_input_raw, min(ACCEL_THRESHOLD_LOW, ACCEL_THRESHOLD_HIGH),
                                    max(ACCEL_THRESHOLD_LOW, ACCEL_THRESHOLD_HIGH));
  int constrained_brake = constrain(brake_input_raw, min(BRAKE_THRESHOLD_LOW, BRAKE_THRESHOLD_HIGH),
                                    max(BRAKE_THRESHOLD_LOW, BRAKE_THRESHOLD_HIGH));
  accel = map(constrained_accel, ACCEL_THRESHOLD_LOW, 
              ACCEL_THRESHOLD_HIGH, 0, 1000) / 1000.0;
  brake = map(constrained_brake, BRAKE_THRESHOLD_LOW, 
              BRAKE_THRESHOLD_HIGH, 0, 1000) / 1000.0;
              
  // In case we get overcurrent errors, reduce the power we send.
  accel *= overcurrent_scale;
  brake *= overcurrent_scale;
  #ifdef VERBOSE
    Serial.print("accel: ");
    Serial.println(accel_input_raw);
    Serial.print("brake: ");
    Serial.println(brake_input_raw);
  #endif
  
  // Send CAN data based on current state.
  if (brake > 0.0) {
    // If the brakes are tapped, cut off acceleration
    if (regen_on) {
      sendDriveCommand(0.0, brake);
    } else {
      sendDriveCommand(0.0, 0.0);
    }
    cruise_on = OFF;
    digitalWrite(OUT_CRUISE_INDICATOR, OFF);
  } else {
    switch (state) {
      case FORWARD:
        if (cruise_on) {
          set_speed = adjustCruiseControl(set_speed);
          if (accel == 0.0) {
            // If pedal is not pressed, allow acceleration
            accel_cruise = ON;
          }
          if (accel_cruise) {
            // Pressing the accelerator during cruise increases speed
            float cruise_accel = accel * (100.0 - set_speed) + set_speed;
            float cruise_accel_torque = accel * (1.0 - CRUISE_TORQUE_SETTING) + CRUISE_TORQUE_SETTING;
            sendDriveCommand(cruise_accel, cruise_accel_torque);
          } else {
            sendDriveCommand(set_speed, CRUISE_TORQUE_SETTING);
          }
        } else if (accel == 0.0) {
          // Accelerator is not pressed
          sendDriveCommand(0.0, 0.0);
        } else {
          sendDriveCommand(100.0, accel);
        }
        break;
      case REVERSE:
        sendDriveCommand(-100.0, accel);
        break;
      case NEUTRAL:
        sendDriveCommand(0.0, 0.0);
        break;
      default:
        state = NEUTRAL;
        break;
    }
  }  
}
