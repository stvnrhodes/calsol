HardwareSPI spi(2);
uint8 recvd = 0xFF;

void setup() {
    spi.beginSlave();
    SerialUSB.println("Starting up");
}

void loop() {
    SerialUSB.print("Sending ");
    SerialUSB.println(recvd, BIN);
    recvd = spi.transfer(recvd);
    SerialUSB.print("Got ");
    SerialUSB.println(recvd, BIN);
}
