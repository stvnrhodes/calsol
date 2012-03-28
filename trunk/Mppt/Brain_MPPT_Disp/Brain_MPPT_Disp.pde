/* Brain MPPT Disp: Originally written to display MPPT data onto an LCD screen */
#include <spi.h>

#define F_CPU 20000000
#define BAUD_RATE 9600

#define VOUT_MULT 0.19697 // Volts per unit
#define VIN_MULT 0.15049 // Volts per unit
#define CURR_MULT 5.86 // MilliAmps per unit

#define S_SEND 0
#define S_READ 1
#define S_MONITOR 2
#define S_SETTING 3

#define LOAD_TX_BUFFER_0 0x40

int numOptions = 17;
char * option_desc[17] = { 
  "BFPCTRL\t", "TXRTSCTRL", "CANSTAT\t", "CANCTRL\t", "TEC\t", "REC\t", 
  "CNF3\t", "CNF2\t", "CNF1\t", "CANINTE\t", "CANINTF\t", "EFLG\t", 
  "TXB0CTRL", "TXB1CTRL", "TXB2CTRL", "RXB0CTRL", "RXB1CTRL" };
char options[17] = { 
  BFPCTRL, TXRTSCTRL, CANSTAT, CANCTRL, TEC, REC,
  CNF3, CNF2, CNF1, CANINTE, CANINTF, EFLG,
  TXB0CTRL, TXB1CTRL, TXB2CTRL, RXB0CTRL, RXB1CTRL };

int id = 0;
int MPPT_num = 0;

// #####################
// SPI LIBRARY
// #####################

byte transfer(byte value)
{
  SPDR = value;
  while (!(SPSR & (1<<SPIF))) ;
  return SPDR;
}

byte transfer(byte value, byte period)
{
  SPDR = value;
  if (period > 0) delayMicroseconds(period);
  while (!(SPSR & (1<<SPIF))) ;
  return SPDR;
}

// #################
// MCP2515 Driver
// #################

char reset()
{
  SPI_START;
  char r = transfer(0xC0);
  SPI_END;
  return r;
}

char Can_Read(char addr)
{
  SPI_START;
  transfer(0x03); //READ
  transfer(addr); //at address
  char r = transfer(0xFF); //Reply
  SPI_END;
  return r;
}

void Can_Write(char addr, char value)
{
  SPI_START;
  transfer(0x02); //WRITE
  transfer(addr);
  transfer(value);
  SPI_END;
}

void Can_Send(byte length, int can_id, char * data) //Defults to buffer 0
{
  length = constrain(length, 0, 8);
  byte TXB0SIDH = can_id >> 3; //CAN ID High bits
  byte TXB0SIDL = (can_id & 0x07) << 5; //CAN ID Low bits
  byte TXB0EID8 = 0x00; //Extended bits not used
  byte TXB0EID0 = 0x00; //See above comment
  byte TXB0DLC  = 0x0F & length; //Data Frame & set length
  int i;
  SPI_START;
  transfer(LOAD_TX_BUFFER_0);
  transfer(TXB0SIDH);  //0x31
  transfer(TXB0SIDL);  //0x32
  transfer(TXB0EID8);
  transfer(TXB0EID0);
  transfer(TXB0DLC);   //0x35 (End of settings)
  SPI_END;
  for(i = 0; i < length; i++)
    Can_Write(0x36 + i, *(data+i));
  delay(1);
  SPI_START;
  transfer(0x81);
  SPI_END;
  
  //Debug
  /*
  Serial.print("Sending Message. Len: ");
  Serial.print(length, DEC);
  Serial.print(" Data: ");
  */
  for(i = length-1; i >= 0; i--)
  {
    if ((unsigned char) (*(data+i) & 0xFF) < 0x10)
      Serial.print("0");
    Serial.print(*(data+i) & 0xFF, HEX);
  }
  Serial.println();
}

/* Returns length */
unsigned char Can_Receive(byte channel, unsigned int * id, char * msg) //Things are returned via pointers
{
  //Get ID
  *id = 0;
  unsigned char addr = 0x60;
  if(channel) addr = 0x70;
  *id += ((unsigned int)Can_Read(addr + 1)) << 3;
  *id += ((unsigned int)Can_Read(addr + 2)) >> 5;
  
  //Get length
  unsigned char length = Can_Read(addr + 5) & 0x0F;
  
  //Get data
  for(int i = 0; i < length; i++)
    *(msg + i) = Can_Read(addr + 6 + i);
  *(msg + length) = '\0';
  
  //Clear buffer
  SPI_START;
  transfer(0x05); //Bit modify
  transfer(0x2C); //Address: CANINTF
  transfer(1 << channel); //Mask: 1 or 2
  transfer(0x00); //Set to 0
  SPI_END;
  
  //dEBUG: Clear all
  Can_Write(CANINTF, 0x00);
  Can_Write(EFLG, 0X00);
  
  
  return length;
}

unsigned char Can_Status()
{
  SPI_START;
  transfer(0xB0);
  unsigned char r = transfer(0xFF);
  SPI_END;
  return r;
}

// ####################
// I/O Stuff
// ####################

int getHex()
{
  int out = 0;
  while(Serial.available() == 0);
  delay(50);
  while(Serial.available() > 0)
  {
    out = out << 4;
    char c = Serial.read();
    if (c >= '0' && c <= '9')
      out += c - '0';
    else if(c >= 'A' && c <= 'F')
      out += c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
      out += c - 'a' + 10;
  }
  return out;
}

char getNum()
{
  Serial.flush();
  while(Serial.available() == 0);
  byte out = 0;
  byte a = Serial.read();
  if (a >= '0' && a <= '9')
    out += (a - '0');
  return out;
}

// ####################
// Menus
// ######################

void send_can()
{
  delay(1);
  Serial.print("ID to use: 0x");
  delay(1);
  unsigned int can_id = getHex();
  Serial.println(can_id, HEX);
  Serial.print("Message to send: ");
  char * msg = "        ";
  char i = 0;
  do
  {
    while(Serial.available() == 0);
    msg[i] = Serial.read();
    i++;
  } while( msg[i-1] != '\n' && msg[i-1] != '\r' && msg[i-1] != '`' && i < 8);
  msg[i] = '\0';
  Serial.flush();
  Serial.print("\r\nSending Message: ");
  Serial.println(msg);
  Serial.println();
  Can_Send(i, can_id, msg);
}

void query()
{
  Serial.println();
  Serial.print("Address to query (hex): ");
  char addr = getHex();
  Serial.print("Querying 0x");
  Serial.println(addr, HEX);
  unsigned char result = Can_Read(addr);
  Serial.print("\r\nResult: 0b");
  Serial.println(result & 0xFF, BIN);
}

void read_can()
{
  //Read Status
  unsigned char rx_status = Can_Status();
  if(rx_status & 0xC0) //If theres a message in RXB0 or RXB1
  {


    unsigned int id = 0;
    char * buffer = "        ";
    unsigned int length = 0;
    if(rx_status & 0x40) //RXB0
      length = Can_Receive(0, &id, buffer);
    else
      length = Can_Receive(1, &id, buffer);

    //Print out
    Serial.print("ID: ");
    Serial.print(id & 0x7FF, HEX);
    //Serial.print("   Data (Hex): ");
    Serial.print(" Data: ");
    
    for(int i = length - 1; i >= 0 ; i--)
    {
      Serial.print(" ");
      Serial.print( *(buffer+i) & 0xFF, HEX);
    }
      
    Serial.print(" Len :");
    Serial.print(length, DEC);
  }
}

void monitor()
{
  Serial.println("Monitor Mode");
  while(Serial.available() == 0)
  {
    read_can();
    delay(1);
  }
  Serial.flush();
}

void summery()
{
  Serial.println();
  Serial.println("Summery");
  int i;
  for(i = 0; i < numOptions; i++)
  {
    Serial.print(option_desc[i]);
    Serial.print("\t0b");
    Serial.println(Can_Read(options[i]) & 0xFF, BIN);
  }

}

void edit_setting()
{
  Can_Write(CNF1, 0x09);
  Can_Write(CNF2, 0x90);
  Can_Write(CNF3, 0x02);
  
  Can_Write(RXB0CTRL, 0x60);
  Can_Write(RXB1CTRL, 0x60);
  
  Can_Write(CANCTRL, 0x00);
}

void edit_ID()
{
  Serial.println();
  Serial.print("Enter Desired ID: ");
  id = getHex();
  Serial.println(id, HEX);
}

void knob()
{
  pinMode(38, OUTPUT);
  pinMode(40, OUTPUT);
  digitalWrite(38, HIGH);
  digitalWrite(40, LOW);
  while(Serial.available() == 0)
  {
    unsigned int value = analogRead(1);
    float out = ((float) value) / 1023.0;
    unsigned int loc = (unsigned int) &out;
    Can_Send(4, 0x90, (char *) loc);
    Serial.print("Debug: ");
    Serial.print((unsigned int) *(&out), HEX);
    delay(150);
  }
  Serial.flush();
}

void settings()
{
  Serial.println();
  Serial.println("Settings Menu");
  Serial.println("Select from the following options");
  Serial.println("1. Summery");
  Serial.println("2. Change Setting");
  Serial.println("3. Change ID");  
  Serial.println("4. Query Register");
  Serial.println("5. Send Reset");
  delay(10);
  char option = getNum();

  switch(option)
  {
  case 1: 
    summery(); 
    break;
  case 2: 
    edit_setting(); 
    break;
  case 3: 
    edit_ID(); 
    break;
  case 4:
    query();
    break;
  case 5:
    SPI_START;
    transfer(0xC0);
    SPI_END;
    break;
  default: 
    Serial.println("Error - Invalid Option");
  }
}

void MPPT(int num)
{
  num++;
  Serial1.write(12);
  Serial1.write(17);
  Serial1.write(0);
  Serial1.write(0);
  Serial1.print(num, DEC);
  Serial1.print(": ");
  Can_Send(0, 0x710 + num, 0x0000);
  delay(500);
  //Read Status
  unsigned char rx_status = Can_Status();
  if(rx_status & 0xC0) //If theres a message in RXB0 or RXB1
  {
    unsigned int id = 0;
    char * buffer = "        ";
    unsigned int length = 0;
    if(rx_status & 0x40) //RXB0
      length = Can_Receive(0, &id, buffer);
    else
      length = Can_Receive(1, &id, buffer);
    unsigned int vin = (unsigned int)(((buffer[0] << 8) & 0x0300) | (unsigned char) buffer[1]);
    float fvin = (float)vin;
    fvin = fvin * VIN_MULT;
    unsigned int vout = (unsigned int)((buffer[4] & 0x03) << 8 | (unsigned char) buffer[5]);
    float fvout = (float)vout;
    fvout = fvout * VOUT_MULT;
    unsigned int curr = (buffer[2] & 0x03) << 8 | (unsigned char) buffer[3];
    float fcurr = (float)curr;
    fcurr = fcurr * CURR_MULT;
    
    /*
    Serial.println("MPPT Query Success");
    Serial.print("Flags: ");
    Serial.println(buffer[0], BIN);
    Serial.print("Battery Voltage Level Reached Flag: ");
    Serial.println(buffer[0] & 0x80, BIN);
    Serial.print("OverTemp Flag: ");
    Serial.println(buffer[0] & 0x40, BIN);
    Serial.print("No Charge Flag: ");
    Serial.println(buffer[0] & 0x20, BIN);
    Serial.print("UnderVolt Flag: ");
    Serial.println(buffer[0] & 0x10, BIN);
    Serial.print("Voltage In: ");
    Serial.print( fvin);
    Serial.println("V");
    Serial.print("Current In: ");
    Serial.println( (buffer[2] & 0x03) << 8 | (unsigned char) buffer[3], DEC);
    Serial.print("Voltage Out: ");
    Serial.print(fvout);
    Serial.println("V");
    Serial.print("Temp: ");
    Serial.print( buffer[6], DEC);
    Serial.println("C");
    */
    
    //Format: 1:119/50/25
    Serial.print(num,DEC);
    Serial.print(":");
    Serial.print(fvout);
    Serial.print("/");
    Serial.print(fvin);
    Serial.print("/");
    Serial.println(fcurr);
    
    
    Serial1.print("T: ");
    Serial1.print(buffer[6], DEC);
    Serial1.print("C ");
    Serial1.print(" I:");
    Serial1.print(fcurr);
    Serial1.write(17); //Next Line
    Serial1.write(0);
    Serial1.write(1);
    Serial1.print("V:");
    Serial1.print(fvin);
    Serial1.print("/");
    Serial1.print(fvout);
    
  }
  else
  {
    Serial.print(num);
    Serial.println(":N/A");
    
    Serial1.write(17); // Newline
    Serial1.write(0);
    Serial1.write(1);
    Serial1.print("Not Available");
  }
}

void menu()
{
  Serial.println();
  Serial.println();
  Serial.println("Main Menu");
  Serial.println("Select from the following options");
  Serial.println("1. Send CAN Message");
  Serial.println("2. Read CAN Buffer");
  Serial.println("3. Monitor Mode");
  Serial.println("4. Settings");
  Serial.println("5. Knob Mode");
  delay(10);
  char option = getNum();

  switch(option)
  {
  case 1: 
    send_can(); 
    break;
  case 2: 
    read_can(); 
    break;
  case 3: 
    monitor();  
    break;
  case 4: 
    settings(); 
    break;
  case 5:
    knob();
    break;
  case 6:
    MPPT(0);
    break;
  default: 
    Serial.println("Error - Invalid Option");
  }
}

void setup()
{

  //SPI Setup
  SPI0_Init();

  //Serial
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1000);
  Serial1.write(15); // Contrast Control
  Serial1.write(40); // Set Contrast to 50
  Serial1.write(12); // Form Feed - Clear Display
  Serial1.print("CalSol MPPT Disp");
  Serial1.write(17);
  Serial1.write(0);
  Serial1.write(1);
  Serial1.print("Starting up...");
  delay(1000);
  
  
  // CAN stuff
  SPI_START;
  transfer(0xC0);
  SPI_END;
  delay(1);
  edit_setting();
  delay(1);
}


void loop()
{
  MPPT(MPPT_num);
  MPPT_num = (MPPT_num + 1) % 5;
  delay(2000);
}

