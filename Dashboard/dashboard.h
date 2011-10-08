/* CalSol - UC Berkeley Solar Vehicle Team 
 * dashboard.h - Dashboard Module
 * Purpose: Header file for the dashboard IO board.
 * Author(s): Ryan Tseng.
 * Date: Oct 3rd 2011
 */

/* User constants */
#define ACCEL_THRESHOLD_LOW  100
#define ACCEL_THRESHOLD_HIGH 1000
#define BRAKE_THRESHOLD_LOW  40
#define BRAKE_THRESHOLD_HIGH 1000
#define LIGHT_BLINK_PERIOD   512

/* State constants */
enum states_enum {
  FORWARD,
  REVERSE,
  NEUTRAL,
  ERROR
} state;

/* Global variables */
float accel = 0.0;
float brake = 0.0;
float current_speed = 0.0;  // Measured speed.
float set_speed = 0.0;      // Desired speed based on cruise control in m/s.
char cruise_on = 0;         // Flag to set if cruise control is on or off.

// Blinker states
enum blinker_state {
  OFF = 0,
  BLINKER_ON,
  BLINKER_OFF
};
blinker_state hazard_state = OFF;
blinker_state left_state   = OFF;
blinker_state right_state  = OFF;

// Other 12V output states
char brake_state  = 0;
char horn_state   = 0;


// Global timekeeping variables
unsigned long last_sent_tritium = 0;
unsigned long last_lights_blinked = 0;

/* Helper functions */
// 8 chars and 2 floats sharing the same space. Used for converting two floats
// into a format that we can use to send CAN data.
typedef union {
  char  c[8];
  float f[2];
} two_floats;

/***
 * Change light and horn state based on switches.
 */
// TODO: Hazard latching
void updateAuxiliaryStates() {
  if (!digitalRead(IN_HAZ_SWITCH)) {
    // Hazard lights
    if (!hazard_state) {
      // If this state is off...
      hazard_state = BLINKER_ON;
      left_state = OFF;
      right_state = OFF;
      last_lights_blinked = millis();
    } else if (hazard_state == BLINKER_ON) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        hazard_state = BLINKER_OFF;
      }
    } else if (hazard_state == BLINKER_OFF) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        hazard_state = BLINKER_ON;
      }
    }
  } else if (!digitalRead(IN_LTURN_SWITCH)) {
    // Left turn lights
    if (!left_state) {
      // If this state is off...
      hazard_state = OFF;
      left_state = BLINKER_ON;
      right_state = OFF;
      last_lights_blinked = millis();
    } else if (left_state == BLINKER_ON) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        left_state = BLINKER_OFF;
      }
    } else if (left_state == BLINKER_OFF) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        left_state = BLINKER_ON;
      }
    }
  } else if (!digitalRead(IN_RTURN_SWITCH)) {
    // Right turn lights
    if (!right_state) {
      // If this state is off...
      hazard_state = OFF;
      left_state = OFF;
      right_state = BLINKER_ON;
      last_lights_blinked = millis();
    } else if (right_state == BLINKER_ON) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        right_state = BLINKER_OFF;
      }
    } else if (right_state == BLINKER_OFF) {
      if (millis() - last_lights_blinked > LIGHT_BLINK_PERIOD) {
        last_lights_blinked = millis();
        right_state = BLINKER_ON;
      }
    }
  } else {
    // Nothin is on, lets turn all the lights off.
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
  if (hazard_state) {
    char light_state = hazard_state == BLINKER_ON;
    digitalWrite(OUT_LTURN, light_state);
    digitalWrite(OUT_RTURN, light_state);
    digitalWrite(OUT_LTURN_INDICATOR, light_state);
    digitalWrite(OUT_RTURN_INDICATOR, light_state);
  } else if (left_state) {
    char light_state = left_state == BLINKER_ON;
    digitalWrite(OUT_LTURN, light_state);
    digitalWrite(OUT_LTURN_INDICATOR, light_state);
    digitalWrite(OUT_RTURN, LOW);
    digitalWrite(OUT_RTURN_INDICATOR, LOW);
  } else if (right_state) {
    char light_state = right_state == BLINKER_ON;
    digitalWrite(OUT_LTURN, LOW);
    digitalWrite(OUT_LTURN_INDICATOR, LOW);
    digitalWrite(OUT_RTURN, light_state);
    digitalWrite(OUT_RTURN_INDICATOR, light_state);
  } else {
    // Turn all lights off.
    digitalWrite(OUT_LTURN, LOW);
    digitalWrite(OUT_RTURN, LOW);
    digitalWrite(OUT_LTURN_INDICATOR, LOW);
    digitalWrite(OUT_RTURN_INDICATOR, LOW);
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
  // First, set switch_state to what the current switch is set at
  states_enum switch_state = NEUTRAL;
  if (digitalRead(IN_VEHICLE_FWD)) {
    switch_state = FORWARD;
  } else if (digitalRead(IN_VEHICLE_REV)) {
    switch_state = REVERSE;
  }
  
  if (switch_state != state && current_speed < 5) {
    // Only switch states if the car is going at less than 10 mph.
    state = switch_state;
  } else if (state == FORWARD && switch_state == state &&
             !digitalRead(IN_CRUISE_ON)) {
    // Cruise is pressed, set cruise to whatever speed we are at.
    set_speed = current_speed;
    cruise_on = 1;
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
  
  // Map values to 0.0 - 1.0 based on thresholds
  int constrained_accel = constrain(accel_input_raw, ACCEL_THRESHOLD_LOW,
                                    ACCEL_THRESHOLD_HIGH);
  int constrained_brake = constrain(brake_input_raw, BRAKE_THRESHOLD_LOW,
                                    BRAKE_THRESHOLD_HIGH);
  accel = map(constrained_accel, ACCEL_THRESHOLD_LOW, 0, 
              ACCEL_THRESHOLD_HIGH, 1000) / 1000.0;
  brake = map(constrained_brake, BRAKE_THRESHOLD_LOW, 0, 
              BRAKE_THRESHOLD_HIGH, 1000) / 1000.0;

  // Send CAN data based on current state.
  if (brake > 0.0) {
    // If the brakes are tapped, cut off acceleration
    sendDriveCommand(0, brake);
    cruise_on = 0;
  } else {
    switch (state) {
      case FORWARD:
        if (cruise_on) {
          sendDriveCommand(set_speed, 0.8);
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
