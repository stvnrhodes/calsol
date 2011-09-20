/* CalSol - UC Berkeley Solar Vehicle Team 
 * brain_wifly.pde - WiFly Module
 * Author(s): Ryan Tseng.
 * Date: Sept 16th 2011
 */


void setup() {
  Serial1.begin(115200);
  Can.begin();
  CanBufferInit();
}

void loop() {
  CanMessage msg;
  if (CanBufferSize()) {
    msg = CanBufferRead();
    Serial.print(msg.id, HEX);
    Serial.print("-");
    Serial.print(msg.len, HEX);
    Serial.print("-");
    for (int i = 0; i < msg.len; i++) {
      Serial.print(msg.data[i], HEX);
    }
    Serial.print(",");
  }
}

