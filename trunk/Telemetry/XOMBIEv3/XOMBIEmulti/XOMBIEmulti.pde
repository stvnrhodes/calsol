#include <XBee.h>
#include "pack.c"

#define DTR_PIN 16
#define RTS_PIN 13
#define CTS_PIN 14
#define SLEEP_PIN 15
#define DI01_PIN 12
#define RESET_PIN 17
#define OE_PIN 18

#define LISTENING 0
#define TRANSITION 1
#define SENDING 2
#define DEBUGGING 3

void print_state(int state) {
  if (state == 0)
      Serial.print("LISTENING");
  else if (state == 1)
      Serial.print("TRANSITION");
  else if (state == 2)
      Serial.print("SENDING");
  else
      Serial.print("DEBUGGING");
}

XBee xbee = XBee();
XBeeAddress64 chaseCarXBEE = XBeeAddress64(0x13A200, 0x405D032A);

int state = LISTENING;
int transitionCount = 0;
int transitionLimit = 100;
int beatCount = 0;
int heartBeat = 100;
boolean waiting = 0;
int waitStart = 0;
int waitLimit = 1000;

int handShake1 = 0x81;
int handShake2 = 0x82;
int handShake3 = 0x83;
int heartShake4 = 0x84;
int heartShake5 = 0x85;

int debuggingMode = 0xC1;
long debuggingStart = 0;
long debuggingTime = 10000;
int packetHistogram = 0xC2;

boolean debuggingFlag = 0;
boolean countingFlag = 0;
long baseTime = 0;
long previousTime = 0;
long interval = 5;
uint8_t histogram[91];

void setup() {
    pinMode(DTR_PIN, OUTPUT);
    pinMode(RTS_PIN, OUTPUT);
    pinMode(CTS_PIN, INPUT);
    pinMode(SLEEP_PIN, INPUT);
    pinMode(DI01_PIN, INPUT);
    pinMode(RESET_PIN, OUTPUT);
    pinMode(OE_PIN, OUTPUT);

    digitalWrite(DTR_PIN, HIGH);
    digitalWrite(RTS_PIN, LOW);
    digitalWrite(RESET_PIN, HIGH);
    digitalWrite(OE_PIN, HIGH);

    xbee.setSerial(Serial1);
    xbee.begin(115200);
    Serial.begin(115200);
    //Serial.println("Starting serial");
    Can.begin(500);
    CanBufferInit();
    //Serial.println("Starting CAN");
}

void stateChange(int new_state, const char *reason) {
    Serial.print("SWITCHED from ");
    print_state(state);
    Serial.print(" to ");
    print_state(new_state);
    Serial.print(":");
    Serial.println(reason);
    state = new_state;
}

//Returns whether the command packet contains the given handshake
boolean handshakePacketVerify(Rx64Response command, int handshake) {
  uint8_t* data = command.getData();
  XBeeAddress64 sender = command.getRemoteAddress64();
  //Serial.print("Sender Address: ");
  //Serial.print(sender.getMsb());
  //Serial.println(sender.getLsb());
  //Serial.print("Data: ");
  short dat1 = data[0];
  //Serial.println(dat1);
  return (sender.getMsb() == chaseCarXBEE.getMsb()) && (sender.getLsb() == chaseCarXBEE.getLsb()) && (data[0] == handshake);
}

//Responds to the heartbeat check from the chasecar
void handleHeartBeat() {
  //Serial.println("Heartbeat check FROM chase car");
  uint8_t hs5[] = {heartShake5};
  Tx64Request tx = Tx64Request(chaseCarXBEE, hs5, sizeof(hs5));
  xbee.send(tx);
  //Serial.println("Heartbeat check FROM car completed");
}
  
void Listening() {
  if (xbee.readPacket(1)) {
    XBeeResponse response = xbee.getResponse();
    //Serial.print("Packet read of type: ");
    short id = response.getApiId();
    //Serial.println(id);
    if (response.getApiId() == RX_64_RESPONSE) {
      Rx64Response command;
      response.getRx64Response(command);
      if(handshakePacketVerify(command, handShake1)) {
        //Serial.println("Handshake 1 received, sending handshake 2");
        long time = millis();
        uint8_t hs2[5];
        hs2[0] = handShake2;
        encode((uint32_t) time, (uint8_t*) (hs2 + 1));
        Tx64Request tx = Tx64Request(chaseCarXBEE, hs2, sizeof(hs2));
        xbee.send(tx);
        //Serial.println("Handshake 2 sent, entering transition mode");
        stateChange(TRANSITION, "Sent handshake 2");
      }
    }
  }
}

void Transition() {
  if (xbee.readPacket(1)) {
    XBeeResponse response = xbee.getResponse();
    //Serial.print("Packet read of type: ");
    short id = response.getApiId();
    //Serial.println(id);
    if (response.getApiId() == RX_64_RESPONSE) {
      Rx64Response command;
      response.getRx64Response(command);
      if (handshakePacketVerify(command, handShake3)) {
        //Serial.println("Handshake 3 received, entering sending mode");
        transitionCount = 0;
        stateChange(SENDING, "Got handshake 3");
        return;
      }
    }
  }
  
  transitionCount++;
  if (transitionCount > transitionLimit) {
    //Serial.println("Transition limit exceeded, entering listening mode");
    transitionCount = 0;
    stateChange(LISTENING, "transition limit exceeded");
  }
}

void Sending(CanMessage *msg) {
  while (xbee.readPacket(1)) {
    //Serial.println("Reading XBee packet");
    XBeeResponse response = xbee.getResponse();
    if (response.getApiId() == RX_64_RESPONSE) {
      //Serial.println("Command message obtained");
      Rx64Response command;
      response.getRx64Response(command);
      if (handshakePacketVerify(command, heartShake4)) {
        handleHeartBeat();
      } else if (handshakePacketVerify(command, heartShake5)) {
        //Serial.println("Heartbeat check TO chase car complete");
        waitStart = 0;
        waiting = 0;
      } else if (handshakePacketVerify(command, debuggingMode)) {
        debuggingStart = millis();
        stateChange(DEBUGGING, "got debugging request");
        return;
      } else if (handshakePacketVerify(command, handShake1)) {
        waitStart = 0;
        waiting = 0;
        beatCount = 0;
        stateChange(LISTENING, "saw a handshake");
        return;
      }
    }
  }
  
  if (waiting == 1) {
    if ((millis() - waitStart) > waitLimit) {
      //Serial.println("Heartbeat check TO chase car timed out, entering listening mode");
      waitStart = 0;
      waiting = 0;
      stateChange(LISTENING, "heartbeat timed out");
      return;
    }
  } else {
    if (beatCount > heartBeat) {
      beatCount = 0;
      uint8_t hs4[] = {heartShake4};
      Tx64Request tx = Tx64Request(chaseCarXBEE, hs4, sizeof(hs4));
      xbee.send(tx);
      waitStart = millis();
      //Serial.println("Heartbeat check TO chase car sent");
      waiting = 1;
    }
  }
  
  if (CanBufferSize() > 4) {
    //Serial.println("CAN message obtained");
    Serial.print("CAN Buffer: ");
    Serial.println(CanBufferSize());
    uint8_t message[14];
    uint16_t id = msg->id;
    uint8_t len = msg->len;
    uint16_t preamble = (id << 4) | len;
    message[0] = (uint8_t)(preamble);
    message[1] = (uint8_t)(preamble >> 8);
    long now = millis();
    encode((uint32_t) now, (message + 2));
    for (int iii = 0; iii < msg->len; iii++) {
      message [iii + 6] = msg->data[iii];
    }
    
    Tx64Request tx = Tx64Request(chaseCarXBEE, message, msg->len + 6);
    xbee.send(tx);
    //Serial.println("CAN message sent");
    if (waiting == 0) {
      beatCount++;
    }
  }
}

void Debugging(CanMessage *msg, long time) {
  if ((millis() - debuggingStart) > debuggingTime) {
    debuggingStart = 0;
    countingFlag = 0;
    previousTime = 0;
    stateChange(SENDING, "done debugging");
    return;
  }
  
  while (xbee.readPacket(1)) {
    //Serial.println("Reading XBee packet");
    XBeeResponse response = xbee.getResponse();
    if (response.getApiId() == RX_64_RESPONSE) {
      //Serial.println("Command message obtained");
      Rx64Response command;
      response.getRx64Response(command);
      if (handshakePacketVerify(command, heartShake4)) {
        handleHeartBeat();
      } else if (handshakePacketVerify(command, heartShake5)) {
        //Serial.println("Heartbeat check TO chase car complete");
        waitStart = 0;
        waiting = 0;
      } else if (handshakePacketVerify(command, debuggingMode)) {
        debuggingStart = millis();
      } else if (handshakePacketVerify(command, handShake1)) {
        waitStart = 0;
        waiting = 0;
        debuggingStart = 0;
        countingFlag = 0;
        previousTime = 0;
        stateChange(LISTENING, "saw handshake 1");
        return;
      }
    }
  }

  if (waiting == 1) {
    if ((millis() - waitStart) > waitLimit) {
      //Serial.println("Heartbeat check TO chase car timed out, entering listening mode");
      waitStart = 0;
      waiting = 0;
      debuggingStart = 0;
      countingFlag = 0;
      previousTime = 0;
      stateChange(LISTENING, "heartbeat timed out");
      return;
    }
  } else {
    if (beatCount > heartBeat) {
      beatCount = 0;
      uint8_t hs4[] = {heartShake4};
      Tx64Request tx = Tx64Request(chaseCarXBEE, hs4, sizeof(hs4));
      xbee.send(tx);
      //Serial.println("Heartbeat check TO chase car sent");
      waiting = 1;
    }
  }
  
  if (msg != NULL) {
    if (countingFlag) {
      if ((time - baseTime) > (90*interval)) {
        histogram[0] = packetHistogram;
        Tx64Request tx = Tx64Request(chaseCarXBEE, histogram, 91);
        xbee.send(tx);
        countingFlag = 0;
        baseTime = 0;
        previousTime = 0;
        if (waiting == 0) {
          beatCount++;
        }
        return;
      }
      long timeDiff = time - previousTime;
      uint8_t index = timeDiff/interval;
      if (index < 90) {
        histogram[index+1] = histogram[index+1] + 1;
        previousTime = time;
      }
    } else {
      baseTime = time;
      countingFlag = 1;
      for (int i = 1; i < 91; i++) {
        histogram[i] = 0;
      }
    }
  }
}
  
  

void loop() {
  
  //Serial.println("Entering loop");
  //Serial.print("CAN Buffer: ");
  //Serial.println(CanBufferSize());
  CanMessage *msg = NULL;
  if (CanBufferSize()) {
    msg = &CanBufferRead();
  }
  long time = millis();
  
  switch (state) {
    case LISTENING:
      Listening();
      break;
    
    case TRANSITION:
      Transition();
      break;
    
    case SENDING:
      Sending();
      break;
      
    case DEBUGGING:
      Debugging(time);
      break;
      
  }
}
