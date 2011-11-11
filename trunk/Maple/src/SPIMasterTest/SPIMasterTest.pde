HardwareSPI spi(2);
uint8 NSS = spi.nssPin();
uint8 i = 0;

void setup() {
    pinMode(NSS, OUTPUT);
    digitalWrite(NSS, HIGH);
    spi.begin();
    SerialUSB.println("Starting up");
    digitalWrite(NSS, LOW);
}

void loop() {
    SerialUSB.print("Sending ");
    SerialUSB.println(i, BIN);
    uint8 recvd = spi.transfer(i);
    SerialUSB.print("Got ");
    SerialUSB.println(recvd, BIN);
    i++;
    delay(2000);
}
