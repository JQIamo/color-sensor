#include <SPI.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

SPISettings settingsA(65000, MSBFIRST, SPI_MODE1);

void setup()
{
    
    //Start serial comms
    Serial.begin(9600);
    delay(10);                           

    //Start & configure SPI settings
    SPI.begin();
    delay(5);
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    delay(5);
    SPI.setBitOrder(MSBFIRST);
    delay(5);
    SPI.setDataMode(SPI_MODE1);
    delay(5);
}

void loop()
{
    int configMSB, configLSB, dataMSB, dataLSB;
    float temp;
    
    dataMSB = SPI.transfer(B10000101);
    dataLSB = SPI.transfer(B10011011);
    configMSB = SPI.transfer(B10000101);    
    configLSB = SPI.transfer(B10011011);
 
    Serial.printf("%d, %d, %d, %d\n", configMSB, configLSB, dataMSB,dataLSB);

    Serial.printf("dataMSB = %s\n", byte_to_binary(dataMSB));
    Serial.printf("dataLSB = %s\n", byte_to_binary(dataLSB));

    temp = (dataMSB*64+dataLSB) * 0.03125;

    Serial.printf("%5.5f\n\n", temp);
    
    delay(2000);
}

const char *byte_to_binary(int x)
{
  static char b[9];
  b[0] = '\0';

  int z;
  for (z = 128; z > 0; z >>=1) {
    strcat(b, ((x & z) == z) ? "1" : "0");
  }
  
  return b;
}


