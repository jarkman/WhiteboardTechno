#define CIRCULAR_BUFFER_INT_SAFE // turn on some interrupt safety in the buffer
#include <CircularBuffer.hpp> // https://github.com/rlogiacco/CircularBuffer

#include <Wire.h>
#include "I2CConstants.h"
#include "MIDI.h"

// build for Arduino Nano

// Receives i2c data from WhiteboardTechnoCamera, sends it as MIDI and CV

// A4 SDA
// A5 SCL

// on Grove cable
// SDA yellow
// SCL white


// Midi sent on pin 3

// Some 303 MIDI notes I don't understand: 
// https://antonsavov.net/articles/303andmidi/
// https://www.elektronauts.com/t/behringer-td-3-303-clone/110053/871

// changing some TD3 filter and pitch bend params over MIDI: https://squarp.community/t/behringer-td-3-mo-cc-list/9761/3

// To set channels

// RD6 - To change it, press the SCALE FUNCTION and PATTERN GROUP buttons, then press one of the steps to select the respective MIDI channel
// this does not work for me!

// TD3 Function & F# to enter the mode then Pattern Section B (under Slide) then the channel - choose 2 - video: https://youtu.be/5nhXp3MT8WQ?si=f8Io9lsrcFCs7zWD&t=144

// or you can connect over USB and use Synthtribe to set channels

// PWM pins are 3, 5, 6, 9, 10, 11

#define CV_OUT_1_PIN 9   // has a 16 bit timer, do we get 16 bit PWM ?
                        // jack output via RC network, 1k / 10 uF

#define CV_OUT_2_PIN 10
#define CV_OUT_3_PIN 11

#define GATE_OUT_1_PIN 5 // from thresholded CV out - jack output via 1k resistor
#define GATE_OUT_2_PIN 6
#define GATE_OUT_3_PIN 7



CircularBuffer<uint32_t,25> notes;

#if defined(ARDUINO_SAM_DUE) || defined(SAMD_SERIES)
   /* example not relevant for this hardware (SoftwareSerial not supported) */
   MIDI_CREATE_DEFAULT_INSTANCE();
#else
   #include <SoftwareSerial.h>
   using Transport = MIDI_NAMESPACE::SerialMIDI<SoftwareSerial>;
   int rxPin = 2;
   int txPin = 3;
   SoftwareSerial mySerial = SoftwareSerial(rxPin, txPin);
   Transport serialMIDI(mySerial);
   MIDI_NAMESPACE::MidiInterface<Transport> MIDI((Transport&)serialMIDI);
#endif



void setup()
{
  Wire.begin(I2C_NANO);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output
  Serial.println("slave receiver");
  
  Serial.println("... waiting");
  delay(5000);
  Serial.println("..waited");

  MIDI.begin();  

  // turn off all notes
  for( int c = 1; c<=3; c ++)
    MIDI.sendControlChange(123,0,c);

  

}

void loop()
{
  
  while( ! notes.isEmpty())
  {
  
    uint32_t b = notes.shift();

    byte *buf = (byte*) &b;

    if( buf[0] == MIDI_NOTE_OP)
    {
      if( buf[3])
        MIDI.sendNoteOn(buf[2], 127, buf[1]);    
      else
        MIDI.sendNoteOff(buf[2], 127, buf[1]); 
    } 
    else if( buf[0] == CV_OP)
    {
      
      // buf[1] is the midi channel (1,2,3)
      // buf[2] is the CV value, 0 to 254. 255 means no note, set gate to 0
      switch(buf[1])
      {
        case 1:
          if( buf[2]=255)
          {
            digitalWrite( GATE_OUT_1_PIN, 0 );    
          }
          else
          {
            digitalWrite( GATE_OUT_1_PIN, 1 ); 
            analogWrite(CV_OUT_1_PIN, buf[2]);
          }
          break;
       case 2:
          if( buf[2]=255)
          {
            digitalWrite( GATE_OUT_2_PIN, 0 );    
          }
          else
          {
            digitalWrite( GATE_OUT_2_PIN, 1 ); 
            analogWrite(CV_OUT_2_PIN, buf[2]);
          }
          break;
       case 3:
          if( buf[2]=255)
          {
            digitalWrite( GATE_OUT_3_PIN, 0 );    
          }
          else
          {
            digitalWrite( GATE_OUT_3_PIN, 1 ); 
            analogWrite(CV_OUT_3_PIN, buf[2]);
          }
          break;
        default:
          Serial.print("Bad channel for CV ");
          Serial.println(buf[2]);
          break;


      }
    } 
    else
    {
      Serial.print("Bad i2c op ");
      Serial.println((int)buf[0]);
    }
  }
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
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

    notes.push(b);

    
  }
  
}