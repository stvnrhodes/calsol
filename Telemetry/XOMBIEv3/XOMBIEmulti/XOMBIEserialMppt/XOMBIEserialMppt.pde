#include "constants.h"
//#define DEBUG

HardwareCan Can1 = HardwareCan(2, 1);
struct mppt_msg {
    uint8_t flags; //Battery Voltage Level Reached Flag, Overtemperature Flag, No Charge Flag, Undervoltage Flag
    uint16_t Vin;  //Input (Array) Voltage
    uint16_t Iin;  //Input (Array) Current
    uint16_t Vout; //Output (Battery) Voltage
    uint8_t Tamb;  //Ambient Temp. in Celsius (steps of 5 degrees Celsius)
};

void setup() {
    pinMode(DTR_PIN, OUTPUT);
    pinMode(RTS_PIN, OUTPUT);
    pinMode(CTS_PIN, INPUT);
    pinMode(SLEEP_PIN, INPUT);
    pinMode(DI01_PIN, INPUT);
    pinMode(RESET_PIN, OUTPUT);
    pinMode(OE_PIN, OUTPUT);
    
    pinMode(ASSOC_LED, OUTPUT);
    pinMode(DEBUG_LED1, OUTPUT);
    pinMode(DEBUG_LED2, OUTPUT);

    digitalWrite(DTR_PIN, HIGH);
    digitalWrite(RTS_PIN, LOW);
    digitalWrite(RESET_PIN, HIGH);
    digitalWrite(OE_PIN, HIGH);
    
    digitalWrite(ASSOC_LED, LOW);
    digitalWrite(DEBUG_LED1, LOW);
    digitalWrite(DEBUG_LED2, LOW);

    Serial1.begin(115200);
    Serial.begin(115200);
    Can.begin(1000);
    CanBufferInit();
    Can1.begin(125);
    Serial.println("Starting up");
}
const uint8_t START_CHAR = 0xE7;
const uint8_t ESCAPE_CHAR = 0x75;

uint8_t isSpecialChar(uint8_t data) {
    return (data == START_CHAR) || (data == ESCAPE_CHAR);
}

/**
 * Takes in a ptr to a string to encode into, and a data byte
 * to encode. Returns a pointer to the next spot in the string.
 */
char* encode(char* ptr, uint8_t data) {
    if (isSpecialChar(data)) {
        *ptr++ = ESCAPE_CHAR;
        *ptr++ = ESCAPE_CHAR ^ data;
    } else {
        *ptr++ = data;
    }
    return ptr;
}


uint16_t createPreamble(uint16_t id, uint8_t len) {
    return id << 4 | len;
}

/**
 * Copies len amount of characters of src to dst. If len is -1,
 * copies src as a string.
 */
void strcpy(char* dst, char* src, uint32_t len) {
    if (len == -1) {
        while(src)
            *dst++ = *src++;
        *dst = 0;
    } else {
        for (int i = 0; i < len; i++) {
            dst[i] = src[i];
        }
    }
}

/**
 * Takes in the id and len of the raw message and puts it in msg.
 */
void initMsg(char* msg, uint16_t id, uint8_t len, uint8_t* data) {
    uint16_t preamble = createPreamble(id, len);
    msg[0] = (uint8_t) preamble;
    msg[1] = (uint8_t) (preamble >> 8);
    strcpy(msg+2, (char*)data, len);
}

/**
 * Creates in dst the message contained in src with proper start symbol
 * and escape sequences. Return the len of the new message.
 */
uint8_t createEscapedMessage(char* dst, char* src, uint8_t len) {
    char* start = dst;
    *dst++ = START_CHAR;
    for (uint8_t i = 0; i < len; i++) {
        dst = encode(dst, src[i]);
    }
    return dst - start;
}

inline uint8_t messageAvailable() {
  return CanBufferSize() > 0;
}

inline void transmitMessage(uint16_t id, uint8_t* data, uint8_t len) {
    char unesc_buffer[16];
    char esc_buffer[32];
    initMsg(unesc_buffer, id, len, data);
    uint8_t esc_len = createEscapedMessage(esc_buffer, unesc_buffer, len+2);
    for (uint8_t i = 0; i < esc_len; i++) {
        Serial1.write(esc_buffer[i]);
        //Serial.write(esc_buffer[i]);
    }
}

void loop() {
    
    if (messageAvailable()) {
        CanMessage can_msg = CanBufferRead();
        transmitMessage(can_msg.id, (uint8_t*)(can_msg.data), can_msg.len);
    }
    
    //Sends a CAN request to the MPPTs
    //1110001 (0x710) for Master Request Base ID
    //mppt_index ranges from 1 to 5 corresponding to the MPPT index
    
    if (millis() - last_mppt_can > 200) {
        last_mppt_can = millis();
        /*
        Serial.print("Sending message for ");
        Serial.println(mppt_index);
        */
        Can1.send(CanMessage(0x710 + mppt_index, 0, 0));
        mppt_index++;
        if (mppt_index > 5)
            mppt_index = 1;
    }
    //Parsing MPPT CAN packet
    PCICR &=~ 0x02;  // Disable PC1 Interrupt
    char avail = Can1.available();
    PCICR |= 0x02;   // Re-enable PC1 interrupt
    if (avail) {
        CanMessage msg;
        //Serial.print("Saw packet from MPPT w/id ");
        //Serial.println(msg.id, HEX);
        if ((msg.id & 0x770) == 0x770) { 
            struct mppt_msg data;
            data.flags = msg.data[0] & 0xf0;
            data.Vin  = ((msg.data[0] & 0x03) << 8) | msg.data[1];
            data.Iin  = ((msg.data[2] & 0x03) << 8) | msg.data[3];
            data.Vout = ((msg.data[4] & 0x03) << 8) | msg.data[5];
            data.Tamb = msg.data[6];
            //Serial.println(data.Vin);
            //Serial.println(data.Iin);
            //Serial.println(data.Vout);
            //Serial.println(data.Tamb, HEX);
            transmitMessage(msg.id, (uint8_t*)(&data), 8);
        }
    }
    
}
