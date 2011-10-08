/* CalSol - UC Berkeley Solar Vehicle Team 
 * dashboard.h - Dashboard Module
 * Purpose: Header file for the dashboard IO board.
 * Author(s): Ryan Tseng, Steven Rhodes.
 * Date: Oct 3rd 2011
 */

/* User constants */
#define ACCEL_THRESHOLD_LOW    100
#define ACCEL_THRESHOLD_HIGH   850
#define BRAKE_THRESHOLD_LOW    20
#define BRAKE_THRESHOLD_HIGH   330
#define LIGHT_BLINK_PERIOD     512
#define CRUISE_SPEED_INCREMENT 0.1
#define CRUISE_TORQUE_SETTING  0.8

// Above this speed, the car will not change from FORWARD to NEUTRAL or REVERSE
#define MAX_STATE_CHANGE_SPEED 5

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
  ERROR_STATUS
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

// Blinker states
char hazard_state = OFF;
char left_state   = OFF;
char right_state  = OFF;

// Other 12V output states
char brake_state  = OFF;
char horn_state   = OFF;

// Global timekeeping variables
unsigned long last_sent_tritium = 0;
unsigned long last_auxiliary_cycle = 0;
unsigned long last_status_blink = 0;

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
  if (msg.id == CAN_TRITIUM_VELOCITY) {
    last_updated_speed = millis();
    current_speed = ((two_floats*)msg.data)->f[1];
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
  return speed;
}


/***
 * Change light and horn state based on switches.
 */
// TODO: Hazard latching
void updateAuxiliaryStates() {
  if (!digitalRead(IN_HAZ_SWITCH)) {
    // Hazard lights
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
  digitalWrite(OUT_BRAKELIGHT, brake > 0.0);
  
  // Turn on horn if the horn button is pressed.
  digitalWrite(OUT_HORN, !digitalRead(IN_HORN_BUTTON));
}

/***
 * Sends a drive command packet to the Tritium motor controller.  Velocity is
 * in meters per second, and current is a float between 0.0 and 1.0.
 */
void sendDriveCommand(float motor_velocity, float motor_current) {
  two_floats data;
  data.f[0] = motor_velocity;
  data.f[1] = motor_current;
  CanMessage msg = CanMessage(CAN_TRITIUM_DRIVE, data.c);
  Can.send(msg);
}

/***
 * Updates the current state of the car (Forward, Reverse, Neutral) based on
 * switches.
 */
// TODO: Cruise control
void updateDrivingState() {
  static char is_cruise_on = FALSE;

  // First, set switch_state to what the current switch is set at
  states_enum switch_state = NEUTRAL;
  if (digitalRead(IN_VEHICLE_FWD)) {
    switch_state = FORWARD;
  } else if (digitalRead(IN_VEHICLE_REV)) {
    switch_state = REVERSE;
  }
  
  regen_on = digitalRead(IN_REGEN_SWITCH);
  
  if (switch_state != state && current_speed < MAX_STATE_CHANGE_SPEED) {
    // Only switch states if the car is going at less than 10 mph.
    state = switch_state;
  } else if (state == FORWARD && switch_state == state &&
             !is_cruise_on &&!digitalRead(IN_CRUISE_ON)) {
    // Cruise is pressed, set cruise to whatever speed we are at.
    set_speed = current_speed;
    cruise_on = ON;
    digitalWrite(OUT_CRUISE_INDICATOR, ON);
  } else if (cruise_on && digitalRead(IN_CRUISE_ON)) {
    // Cruise is not pressed, but cruise control is still on
    is_cruise_on = TRUE;
  } else if (is_cruise_on && !digitalRead(IN_CRUISE_ON)) {
    // Cruise is pressed, so we want to turn it off
    is_cruise_on = FALSE;
    cruise_on = OFF;
    digitalWrite(OUT_CRUISE_INDICATOR, OFF);
  }
}

/***
 * Calls a single routine of the driver routine loop.  This function reads
 * pedal inputs, adjusts for thresholds, and send CAN data to the Tritium.
 */
// TODO: Regen switch
void driverControl() {
  // Read raw pedal values
  int accel_input_raw = analogRead(ANALOG_ACCEL_PEDAL);
  int brake_input_raw = analogRead(ANALOG_BRAKE_PEDAL);
  
  // The raw brake value decreases as you press it, so we reverse it here
  brake_input_raw = 1023 - brake_input_raw;
  
  // Map values to 0.0 - 1.0 based on thresholds
  int constrained_accel = constrain(accel_input_raw, ACCEL_THRESHOLD_LOW,
                                    ACCEL_THRESHOLD_HIGH);
  int constrained_brake = constrain(brake_input_raw, BRAKE_THRESHOLD_LOW,
                                    BRAKE_THRESHOLD_HIGH);
  accel = map(constrained_accel, ACCEL_THRESHOLD_LOW, 
              ACCEL_THRESHOLD_HIGH, 0, 1000) / 1000.0;
  brake = map(constrained_brake, BRAKE_THRESHOLD_LOW, 
              BRAKE_THRESHOLD_HIGH, 0, 1000) / 1000.0;

  // Send CAN data based on current state.
  if (brake > 0.0) {
    // If the brakes are tapped, cut off acceleration
    if (regen_on) {
      Serial.println("Regen on");
      sendDriveCommand(0.0, brake);
    } else {
      Serial.println("Regen off");
      sendDriveCommand(0.0, 0.0);
    }
    cruise_on = OFF;
    digitalWrite(OUT_CRUISE_INDICATOR, OFF);
  } else {
    switch (state) {
      case FORWARD:
        if (cruise_on) {
          set_speed = adjustCruiseControl(set_speed);
          sendDriveCommand(set_speed, CRUISE_TORQUE_SETTING);
        } else if (accel == 0.0) {
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
