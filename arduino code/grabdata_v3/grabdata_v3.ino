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
#include <TimerOne.h>

// command definitions
#define WRITE    0x10
#define READ     0x20
#define SETRATE  0x30

// TCS-3414 register addresses 
#define TCS       0x39
#define CONTROL   0x80
#define TIMING    0x81

// Sync integration constants
#define INT_TIME  500000
#define SYNC_PIN 17

// Function prototypes
void readSlave(void);
void syncInt(void);

IntervalTimer slaveTimer;
size_t addr=0, len;
volatile boolean intVar = 1;

void setup()
{
  pinMode(LED_BUILTIN,OUTPUT);            // LED blink output
  pinMode(SYNC_PIN, OUTPUT);              // set sync pin to output
  digitalWrite(SYNC_PIN, LOW);            // set teensy SYNC pin low 
                                          // (rising edge tells TCS sensor to start integration)
    
  // setup for Master mode, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  // setup for Slave mode (sensor), TCS address, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, TCS, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
    
  Serial.begin(9600);
  delay(10);                              

  Wire.beginTransmission(TCS);           
  Wire.write(CONTROL);                    // access control register
  Wire.write(0x03);                       // powers up TCS chip / enable ADC
  Wire.endTransmission();            

  Wire.beginTransmission(TCS);            
  Wire.write(TIMING);                     // access timing register
  Wire.write(0x70);                       // sync integration over rising edge
  Wire.endTransmission();              
  delay(10);

  //while(!Serial);                         // wait to start
  
  digitalWrite(SYNC_PIN, HIGH);           // start ADC integration cycle
  slaveTimer.begin(syncInt, INT_TIME);    // start integration timer
  delayMicroseconds(150);                 // minimum pulse width 50us
  digitalWrite(SYNC_PIN, LOW);            // end pulse
}

// sync interrupt function - ends the integration cycle by pulling the 
// sync pin high and setting intVar to 0 so that loop commences
void syncInt(void) 
{
  digitalWrite(SYNC_PIN, HIGH);                 // end ADC integration cycle
  intVar = 0;
}

void loop()
{
    while(intVar == 1);                 // wait until ADC integration cycle finishes

    slaveTimer.end();
    delayMicroseconds(150);               // minimum pulse width 50 us
    digitalWrite(SYNC_PIN, LOW);              // finish ending pulse
         
    noInterrupts();
    readSlave();
    intVar = 1;
    interrupts();

    digitalWrite(SYNC_PIN, HIGH);                 // start ADC integration cycle
    slaveTimer.begin(syncInt, INT_TIME);    // start integration timer
    delayMicroseconds(100);                 // minimum pulse width 50us
    digitalWrite(SYNC_PIN, LOW);                  // end pulse
}
    


void readSlave(void)
{
    // storage of channel data; g_low = green channel low byte
    uint8_t g_low, g_high, r_low, r_high,
            b_low, b_high, c_low, c_high;
   
    // variables for total channel values (2 bytes per channel)
    int green, red, blue, clr;
        
    digitalWrite(LED_BUILTIN,HIGH);     // pulse LED when reading

     
    // diable ADC integration
    Wire.beginTransmission(TCS);        // talk to slave
    Wire.write(CONTROL);                // access control register
    Wire.write(0x01);                   // disable ADC integration (toggle)
    Wire.endTransmission();             // blocking Tx
      
    // read values
    Wire.beginTransmission(TCS);        // talk to slave
    Wire.write(0xB6);                   // command for clear lower data byte
    Wire.endTransmission(I2C_NOSTOP);   // blocking Tx, no STOP
    Wire.requestFrom(TCS,2,I2C_STOP);   // request 2 bytes
    c_low = Wire.readByte();
    c_high = Wire.readByte();
    clr = c_low+c_high*256;
    
    Wire.beginTransmission(TCS);        // talk to slave
    Wire.write(0xB0);                   // access lower green data byte
    Wire.endTransmission(I2C_NOSTOP);   // blocking Tx, no STOP
    Wire.requestFrom(TCS,2,I2C_STOP);   // request 2 bytes
    g_low = Wire.readByte();
    g_high = Wire.readByte();
    green = g_low+g_high*256;

    Wire.beginTransmission(TCS);        // talk to slave
    Wire.write(0xB2);                   // access lower red data byte
    Wire.endTransmission(I2C_NOSTOP);   // blocking Tx, no STOP
    Wire.requestFrom(TCS,2,I2C_STOP);   // request 2 bytes
    r_low = Wire.readByte();
    r_high = Wire.readByte();
    red = r_low+r_high*256;

    Wire.beginTransmission(TCS);        // talk to slave
    Wire.write(0xB4);                   // access lower blue data byte
    Wire.endTransmission(I2C_NOSTOP);   // blocking Tx, no STOP
    Wire.requestFrom(TCS,2,I2C_STOP);    // request 2 bytes
    b_low = Wire.readByte();
    b_high = Wire.readByte();
    blue = b_low+b_high*256;
   
    // Print data from channels    
    Serial.printf("clr: %d, g: %d, b: %d, r: %d\n", clr, green, blue, red);

    digitalWrite(LED_BUILTIN,LOW);
    
    // enable ADC integration
    Wire.beginTransmission(TCS);        
    Wire.write(CONTROL);                // access control register
    Wire.write(0x03);                   // enable ADC integration
    Wire.endTransmission();                              
}
