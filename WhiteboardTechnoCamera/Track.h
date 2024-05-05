#pragma once

#include "I2CConstants.h"

class Track {

public:

int midiChannel = 0;
int x1;
int y1;
int width;
int height;
int noteNumberMin;
int noteNumbers;
// something about quantisation


#define MAX_NOTES 100
bool notes[MAX_NOTES];
int lastCv = -1;

Track(int _midiChannel, int _x1, int _y1, int _width, int _height, int _noteNumberMin, int _noteNumbers)
{
  midiChannel - _midiChannel;
  x1 = _x1;
  y1 = _y1;
  height = _height;
  noteNumberMin = _noteNumberMin;
  noteNumbers = _noteNumbers;
};

void processBeat( int32_t beat, int32_t beatsPerLoop)//, Frame frame )
{
  
  bool newNotes[MAX_NOTES];

  double x = map(beat, 0, beatsPerLoop-1, x1, x1 + width );

  for( int n = 0; n < noteNumbers; n ++)
    newNotes[n] = false;

  int cv = -1;

  for(int y = y1; y < y1 + height; y ++)
  {
    // scan the vertical line at x
    // if we detect a black bit, find the middle
    int centerY = 3;
    int note = ((centerY - y1) * noteNumbers) / height;
    cv =   ((centerY - y1) * CV_MAX) / height;
    newNotes[note] = true;
  }

  for( int n = 0; n < noteNumbers; n ++)
  {
    if( ! notes[n] && newNotes[n])
      sendNote(n, true);
    else if( notes[n] && ! newNotes[n])
      sendNote(n, false);
  }

  for( int n = 0; n < noteNumbers; n ++)
    notes[n] = newNotes[n];

  if( cv != lastCv )
    sendCv(cv);
};



void sendNote( int noteNumber, bool start)
{
  byte buf[4];
  buf[0]=MIDI_NOTE_OP;
  buf[1]=midiChannel;
  buf[2]=noteNumber;
  buf[3]=start;

  sendI2C(I2C_NANO, buf, 4);
};

void sendCv( int cv)
{
  byte buf[4];
  buf[0]=CV_OP;
  buf[1]=cv;
  buf[2]=0;
  buf[3]=0;

  sendI2C(I2C_NANO, buf, 4);
};


void sendI2C(int target, byte *buf, int n)
{

  Wire.beginTransmission(target); 
  Wire.write(buf, n);
  
  Wire.endTransmission();    // stop transmitting
  
};

};