// runs on a Lilygo T-CAN485 https://github.com/Xinyuan-LilyGO/T-CAN485/tree/main
// select ESP32 Dev Module

#include <esp_dmx.h> //esp_dmx by Mitch Weisbrod
#include "config.h"
#include <Wire.h>
#include "I2CConstants.h"
#define CIRCULAR_BUFFER_INT_SAFE // turn on some interrupt safety in the buffer
#include <CircularBuffer.hpp> // https://github.com/rlogiacco/CircularBuffer


CircularBuffer<uint32_t,25> notes;




/* First, lets define the hardware pins that we are using with our ESP32. We
  need to define which pin is transmitting data and which pin is receiving data.
  DMX circuits also often need to be told when we are transmitting and when we
  are receiving data. We can do this by defining an enable pin. */

int transmitPin = RS485_TX_PIN;
int receivePin = RS485_RX_PIN;
int enablePin =  RS485_EN_PIN;// RS485_EN_PIN or RS485_SE_PIN; ??  TODO


/* Make sure to double-check that these pins are compatible with your ESP32!
  Some ESP32s, such as the ESP32-WROVER series, do not allow you to read or
  write data on pins 16 or 17, so it's always good to read the manuals. */

/* Next, lets decide which DMX port to use. The ESP32 has either 2 or 3 ports.
  Port 0 is typically used to transmit serial data back to your Serial Monitor,
  so we shouldn't use that port. Lets use port 1! */
dmx_port_t dmxPort = 1;  // 2 from https://github.com/Xinyuan-LilyGO/T-CAN485/blob/main/example/Arduino/RS485/RS485.ino
// crashes wih 2!

/* Now we want somewhere to store our DMX data. Since a single packet of DMX
  data can be up to 513 bytes long, we want our array to be at least that long.
  This library knows that the max DMX packet size is 513, so we can fill in the
  array size with `DMX_PACKET_SIZE`. */


byte data[DMX_PACKET_SIZE];

/* This variable will allow us to update our packet and print to the Serial
  Monitor at a regular interval. */
unsigned long lastUpdate = millis();

void setup() {
  /* Start the serial connection back to the computer so that we can log
    messages to the Serial Monitor. Lets set the baud rate to 115200. */
  Serial.begin(115200);

  Serial.println("Setup begin");

 Wire.begin(I2C_DMX);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event

  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);

  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  /* Now we will install the DMX driver! We'll tell it which DMX port to use,
    what device configuration to use, and what DMX personalities it should have.
    If you aren't sure which configuration to use, you can use the macros
    `DMX_CONFIG_DEFAULT` to set the configuration to its default settings.
    Because the device is being setup as a DMX controller, this device won't use
    any DMX personalities. */

    Serial.println("Driver install");

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  
  const int personality_count = 1;
  dmx_personality_t personalities[] = {
    {1, "Default Personality"}
  };

  dmx_driver_install(dmxPort, &config, personalities, personality_count);


  /* Now set the DMX hardware pins to the pins that we want to use and setup
    will be complete! */

     Serial.println("Set pin");
    
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}

void setDmxData(byte address, byte r, byte g, byte b)
{
    
      data[0] = 0; // for lighting

      int a = 1 + (address-1)*6;   // DMX address 1 is first in the array

      data[a++] = r;
      data[a++] = g; //actuators.g;
      data[a++] = b; //actuators.b;
      data[a++] = 0x00; //actuators.w; white
      data[a++] = 0x00; //actuators.am; amber
      data[a++] = 0x00; //actuators.uv; uv

    
      
}

void loop() {
 
 
 while( ! notes.isEmpty())
  {
  
    uint32_t b = notes.shift();

    //Serial.print("popped "); Serial.println(b, HEX);

    byte *buf = (byte*) &b;

  
      //Serial.print("On ch ");
      //Serial.print(buf[1]);
      //Serial.print(" note ");
      //Serial.println(buf[2]);
      //Serial.print(" vel ");
      //Serial.println(buf[3]);
      setDmxData(buf[0],buf[2], buf[3], buf[1]);    
    
  
    

    Serial.println("dmx_write");
    
    dmx_write(dmxPort, data, DMX_PACKET_SIZE);

    /* Log our changes to the Serial Monitor. */
    Serial.printf("Sending DMX 0x%02X\n", data[1]);
    
  }

   Serial.println("dmx_send_num");
    
  /* Now we can transmit the DMX packet! */
  dmx_send_num(dmxPort, DMX_PACKET_SIZE);

  /* We can do some other work here if we want. */

 Serial.println("dmx_wait_sent");
    
  /* If we have no more work to do, we will wait until we are done sending our
    DMX packet. */
  dmx_wait_sent(dmxPort, DMX_TIMEOUT_TICK);

   Serial.println("dmx_wait_sent done");
    
}

void receiveEvent(int howMany)
{
  uint32_t b;
  byte *buf = (byte*) &b;

  if( Wire.available() >= 4) 
  {
    buf[0] = Wire.read(); 
    buf[1] = Wire.read(); 
    buf[2] = Wire.read(); 
    buf[3] = Wire.read(); 


    //Serial.print("rec "); Serial.println(b, HEX);
    notes.push(b);

    
  }
  
}
