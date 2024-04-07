#include <MIDI.h>

// built for Nano

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
  Serial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);
  MIDI.begin();  

  // should turn all the notes off really
  for (int note = 36; note <= 51; note++)                 
    MIDI.sendNoteOff(note, 0, TD3_CHANNEL);
}

void loop()
{
  
    for (int note = 36; note <= 51; note++) 
    {
      Serial.println(note);
    digitalWrite(LED_BUILTIN, HIGH);
    MIDI.sendNoteOn(note, 127, TD3_CHANNEL);    // Send a Note (pitch 42, velo 127 on channel 1)
    delay(200);                    // Wait for a second
    MIDI.sendNoteOff(note, 0, TD3_CHANNEL);     // Stop the note
    digitalWrite(LED_BUILTIN, LOW);
    }
  
}
