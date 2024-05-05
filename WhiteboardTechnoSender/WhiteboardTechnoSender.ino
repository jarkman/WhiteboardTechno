
// build for Arduino Nano

// Receives i2c data from WhiteboardTechnoCamera, sends it as MIDI and CV

// A4 SDA
// A5 SCL

// on Grove cable
// SDA yellow
// SCL white

#include <Wire.h>
#include "I2CConstants.h"
#include "MIDI.h"
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


// I made up these names, must be official ones
#define RD6_CRASH 36
#define RD6_CLAP 39
#define RD6_BASH  40
#define RD6_SHORT_CYMBAL 42
#define RD6_BOINK 45
#define RD6_MEDIUM_CYMBAL  46
#define RD6_BINK 50
#define RD6_LONG_CYMBAL 51

#define RD6_CHANNEL 1
#define TD3_CHANNEL 2


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

  // should turn all the notes off really
  for (int note = 36; note <= 51; note++)                 
    MIDI.sendNoteOff(note, 0, TD3_CHANNEL);

}

void loop()
{
  delay(1000);
  Serial.println(".");
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  byte buf[3];

  if( Wire.available() >= 3) 
  {
    buf[0] = Wire.read(); 
    buf[1] = Wire.read(); 
    buf[2] = Wire.read(); 

    if( buf[0] == MIDI_NOTE_OP)
    {
      if( buf[3])
        MIDI.sendNoteOn(buf[2], 127, buf[1]);    
      else
        MIDI.sendNoteOff(buf[2], 127, buf[1]); 
    } 
    else if( buf[0] == CV_OP)
    {
      ; //TODO
    } 
    else
    {
      Serial.print("Bad i2c packet ");
      Serial.println((int)buf[0]);
    }
  }
  
}