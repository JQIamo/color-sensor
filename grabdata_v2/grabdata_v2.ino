// -------------------------------------------------------------------------------------------
// Teensy3.0/3.1/LC I2C Interrupt Test
// 15May13 Brian (nox771 at gmail.com)
// -------------------------------------------------------------------------------------------
//
// This creates an I2C master device which will issue I2C commands from inside a periodic
// interrupt (eg. reading a sensor at regular time intervals).  For this test the Slave device
// will be assumed to be that given in the i2c_t3/slave sketch.
//
// The test will start when the Serial monitor opens.
//
// This example code is in the public domain.
//
// -------------------------------------------------------------------------------------------
// Slave protocol is as follows:
// -------------------------------------------------------------------------------------------
// WRITE - The I2C Master can write to the device by transmitting the WRITE command,
//         a memory address to store to, and a sequence of data to store.
//         The command sequence is:
//
// START|I2CADDR+W|WRITE|MEMADDR|DATA0|DATA1|DATA2|...|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       WRITE     = WRITE command
//       MEMADDR   = memory address to store data to
//       DATAx     = data byte to store, multiple bytes are stored to increasing address
//       STOP      = I2C STOP sequence
// -------------------------------------------------------------------------------------------
// READ - The I2C Master can read data from the device by transmitting the READ command,
//        a memory address to read from, and then issuing a STOP/START or Repeated-START,
//        followed by reading the data.  The command sequence is:
//
// START|I2CADDR+W|READ|MEMADDR|REPSTART|I2CADDR+R|DATA0|DATA1|DATA2|...|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       READ      = READ command
//       MEMADDR   = memory address to read data from
//       REPSTART  = I2C Repeated-START sequence (or STOP/START if single Master)
//       I2CADDR+R = I2C Slave address + I2C read flag
//       DATAx     = data byte read by Master, multiple bytes are read from increasing address
//       STOP      = I2C STOP sequence
// -------------------------------------------------------------------------------------------
// SETRATE - The I2C Master can adjust the Slave configured I2C rate with this command
//           The command sequence is:
//
// START|I2CADDR+W|SETRATE|RATE|STOP
//
// where START     = I2C START sequence
//       I2CADDR+W = I2C Slave address + I2C write flag
//       SETRATE   = SETRATE command
//       RATE      = I2C RATE to use (must be from i2c_rate enum list, eg. I2C_RATE_xxxx)
// -------------------------------------------------------------------------------------------

#include <i2c_t3.h>

// Command definitions

#define WRITE    0x10
#define READ     0x20
#define SETRATE  0x30

// TCS-3414 Command definitions 
// Upper four bytes with MSB = 1 is interpreted as command byte by the TCS sensor
// Lower four bits of the byte form the register address
#define COMMAND     0x80                   
#define TIMING      0x81                   
#define INTERRUPT   0x82
#define INTSOURCE   0x83

// Function prototypes
void readSlave(void);

IntervalTimer slaveTimer;
uint8_t tcs=0x39;
size_t addr=0, len;


void setup()
{
    pinMode(LED_BUILTIN,OUTPUT);        // LED

    // Setup for Master mode, sensor address, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
    
    // Setup for Slave mode, address void, pins 18/19, external pullups, 400kHz
    Wire.begin(I2C_SLAVE, tcs, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
    
    Serial.begin(115200);
    delay(5);                           // Slave powerup wait

    // Setup Slave
    Wire.beginTransmission(tcs);        // slave addr
    Wire.write(COMMAND);                // Command to access control register
    Wire.write(0x03);                   // Power on and enable ADC
    delay(15);                          // Chip starts 12ms integrations
    Wire.endTransmission();             // blocking Tx

    Wire.beginTransmission(tcs);        // master addr
    Wire.write(TIMING);                 // Command to access timing register
    Wire.write(0x01);                   // Set integration cycles to 100ms
    Wire.endTransmission();             // blocking Tx

    while(!Serial);                     // wait to start

    // Start reading Slave device
    slaveTimer.begin(readSlave,5000000); // 1s timer
}

void loop()
{
    readSlave();
    delay(1000);
}

void readSlave(void)
{
    // 8 variables to hold the data 
    // g_low = green channel low byte, etc.
    int g_low, g_high,                     
        r_low, r_high,
        b_low, b_high,
        c_low, c_high;
   
    // variables for total channel values
    int green, red, blue, clr;
        
    digitalWrite(LED_BUILTIN,HIGH);         // pulse LED when reading

    Wire.beginTransmission(tcs);            // slave addr
    Wire.write(0xB6);                       // command for clear lower data byte
    Wire.endTransmission(I2C_NOSTOP);       // blocking Tx, no STOP
    Wire.requestFrom(tcs,2,I2C_STOP);       // request 2 bytes
    
    c_low = Wire.readByte();
    c_high = Wire.readByte()*256;
    clr = c_low+c_high;
    
    Wire.beginTransmission(tcs);            // slave addr
    Wire.write(0xB0);                       // command for green lower data byte
    Wire.endTransmission(I2C_NOSTOP);       // blocking Tx, no STOP
    Wire.requestFrom(tcs,2,I2C_STOP);       // request 2 bytes

    g_low = Wire.readByte();
    g_high = Wire.readByte()*256;
    green = g_low+g_high;

    Wire.beginTransmission(tcs);            // slave addr
    Wire.write(0xB2);                       // command for red lower data byte
    Wire.endTransmission(I2C_NOSTOP);       // blocking Tx, no STOP
    Wire.requestFrom(tcs,2,I2C_STOP);       // request 2 bytes

    r_low = Wire.readByte();
    r_high = Wire.readByte()*256;
    red = r_low+r_high;

    Wire.beginTransmission(tcs);            // slave addr
    Wire.write(0xB4);                       // command for blue lower data byte
    Wire.endTransmission(I2C_NOSTOP);       // blocking Tx, no STOP
    Wire.requestFrom(tcs,2,I2C_STOP);       // request 2 bytes

    b_low = Wire.readByte();
    b_high = Wire.readByte()*256;
    blue = b_low+b_high;
   
    
    
    Serial.printf("Clear channel: %d\n", clr);
    Serial.printf("Green channel: %d\n", green);
    Serial.printf("Red channel: %d\n", red);
    Serial.printf("Blue channel: %d\n", blue);
    //Serial.printf("Data read from clear channel upper byte: %d\n", Wire.readByte()*256);
    
   
    //addr = (addr < 7) ? addr+1 : 0;         // loop reading memory address

    digitalWrite(LED_BUILTIN,LOW);
}
