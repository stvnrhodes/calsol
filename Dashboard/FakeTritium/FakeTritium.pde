/* CalSol - UC Berkeley Solar Vehicle Team 
 * FakeTritium.pde - For Testing
 * Purpose: Emulate Tritium's pattern of CAN messages
 * Author(s): Steven Rhodes.
 * Date: March 29 2012
 */
 
#include "can_id.h"
 
typedef union {
  char c[8];
  float f[2];
} two_floats;
 
float spd = 0;
two_floats f;

void process_packet(CanMessage &msg) {
  switch(msg.id) {
    case CAN_TRITIUM_DRIVE:
      f = *((two_floats*) msg.data);
      spd = f.f[1];
      Serial.print("Go at ");
      Serial.print(f.f[1]);
      Serial.print(" using ");
      Serial.print(f.f[0]);
      Serial.println(" power");
      break;
    case CAN_TRITIUM_RESET:
      Serial.println("Reset Tritium");
      break;
    default:
      Serial.print("Unidentified message: ");
      Serial.println(msg.id);
      break;
  }
}
 
void setup() {
  Serial.begin(115200);
  Can.attach(&process_packet);
  Can.begin(1000);
}

void loop() {
  static unsigned long id_cycle = 0;
  static unsigned long status_cycle = 0;
  static unsigned long aux_cycle = 0;
  char zero[] = {0, 0, 0, 0, 0, 0, 0, 0};
  
  unsigned long time = millis();
  if(time - aux_cycle > 5000){
    Serial.print(" aux,");
    Can.send(CanMessage(CAN_TRITIUM_AIR_TEMP, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_CAP_TEMP, zero, 8));
    delay(1);
    aux_cycle = time;
  }
  if(time - id_cycle > 1000){
    Serial.print(" id,");
    char data[] = {'T', 'R', 'I', 'a', 0x00, 0x00, 0x00, 0x01};
    Can.send(CanMessage(CAN_TRITIUM_ID, data, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_15V_RAIL, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_LV_RAIL, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_FAN_SPEED, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_MOTOR_TEMP, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_ODOMETER, zero, 8));
    delay(1);
    id_cycle = time;
  }
  if(time - status_cycle > 200){
    Serial.println(" status");
    two_floats data;
    data.f[0] = 0;
    data.f[1] = 0;
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_STATUS, data.c, 8));
    data.f[0] = 120;
    data.f[1] = 0;
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_BUS, data.c, 8));
    data.f[0] = spd;
    data.f[1] = spd/2;
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_VELOCITY, data.c, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_PHASE_CURR, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_MOTOR_VOLT, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_MOTOR_CURR, zero, 8));
    delay(1);
    Can.send(CanMessage(CAN_TRITIUM_MOTOR_BEMF, zero, 8));
    delay(1);
    status_cycle = time;
  }
}

