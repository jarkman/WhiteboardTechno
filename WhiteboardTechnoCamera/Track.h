#pragma once

#include "I2CConstants.h"

extern bool drawMarkers;
extern camera_fb_t *fb;
void drawRect(int x0, int y0, int w, int h, uint16_t rgb);
void drawMarker(int x0, int y0, uint16_t rgb);
uint32_t pixelBrightness( int x, int y );

class Track {

public:

int midiChannel = 0;
int x1;
int y1;
int width;
int height;
uint32_t colour;
int numNoteNumbers;
uint8_t *noteNumbers;
bool alwaysNew;


#define MAX_NOTES 100
bool notes[MAX_NOTES];
int lastCv = -1;

Track(int _midiChannel, int _x1, int _y1, int _width, int _height, uint32_t _colour, int _numNoteNumbers, uint8_t *_noteNumbers, bool _alwaysNew)
{
  midiChannel = _midiChannel;
  x1 = _x1;
  y1 = _y1;
  width = _width;
  height = _height;
  colour = _colour;
  numNoteNumbers = _numNoteNumbers;
  noteNumbers = _noteNumbers;
  alwaysNew = _alwaysNew;

};

void drawBoxes(int32_t beat, int32_t beatsPerLoop)
{
 if( drawMarkers )
  {
    //Serial.println("drawing markers");
    drawRect( x1+1, y1+1, width-1, height-1, colour);

    double x = map(beat, 0, beatsPerLoop-1, x1, x1 + width );

    for( int b = 0; b < beatsPerLoop; b ++)
    {
      double x = map(b, 0, beatsPerLoop-1, x1, x1 + width );
      drawRect( x, y1+1, 1, b==beat?5:3, colour);

    }
  }
}
void processBeat( int32_t beat, int32_t beatsPerLoop,int autodrum)//, Frame frame )
{
 
  bool newNotes[MAX_NOTES];
  uint8_t velocities[MAX_NOTES];

  double x = map(beat, 0, beatsPerLoop-1, x1, x1 + width );

  for( int n = 0; n < numNoteNumbers; n ++)
    newNotes[n] = false;

  if( autodrum >= 0 && autodrum < numNoteNumbers )
    newNotes[autodrum]=true;

  int cv = -1;

  uint32_t sum = 0;
  uint32_t minB = 0xFFFFFFFF;
  uint32_t maxB = 0;

  for(int y = y1; y < y1 + height; y ++)
  {
    uint32_t b = pixelBrightness(x,y);
    sum += b;
    minB = min(b, minB);
    maxB = max( b, maxB);
    //Serial.printf("%d ", b);
  }

  bool allWhite = false;

  uint32_t threshold = (maxB+minB)/3;

  
  if( maxB - minB < threshold )
    allWhite = true;

  //Serial.printf("\nmax %d min %d mean %d threshold %d maxB-minB %d, allWhite %d\n", maxB, minB, sum/height, threshold, maxB-minB, allWhite);
  

  bool inBlack = false;
  int blackStart = -1;

  if( ! allWhite )
  {
  // scan the vertical line at x
    for(int y = y1; y < y1 + height+1; y ++) // do 1 extra px to let is finish the last bit
    {

      uint32_t b;
      
      if( y < y1+height )
        b = pixelBrightness(x,y);
      else
        b = threshold+1; // fake last pix which is white

      if( ! inBlack && b <= threshold )
      {
        inBlack = true;
        blackStart = y;
      }
      else if( inBlack && b > threshold )
      {
        // if we detect a black bit, find the middle

        inBlack = false;
        int h = y-blackStart;
        int velocity = 127; //TODO map(h,0,height/8,50,127); // full velocity at 1/8 of panel height
        int centerY = (y+blackStart)/2;
        int pos = ((centerY - y1) * numNoteNumbers)  / height;  // y==0 is at the top here
        pos = numNoteNumbers - pos;

        cv =   ((centerY - y1) * CV_MAX) / height;

        if( pos < 0 || pos > numNoteNumbers)
        {
          Serial.printf("Bad pos %d (not 0-%d)", pos, numNoteNumbers);
        }
        else
        {
          newNotes[pos] = true;
          velocities[pos] = velocity;
          //Serial.printf("found note channel %d at pos %d noteNumber %d\n", midiChannel, pos, noteNumbers[pos]);
        }

        if( drawMarkers)
          drawMarker(x, centerY, 0xffff-colour);
      }
    
    }
  }

  if( alwaysNew ) // for drums, do a note on every beat
  {
    for( int n = 0; n < numNoteNumbers; n ++)
    {
      if( ! notes[n] )
        sendNote(n, false,127);
        
      if( notes[n] )
        sendNote(n, true, velocities[n]);
        
    }
  }
  else
  {
    for( int n = 0; n < numNoteNumbers; n ++)
    {
      if( ! notes[n] && newNotes[n])
        sendNote(n, true, velocities[n]);
      else if( notes[n] && ! newNotes[n])
        sendNote(n, false,127);
    }
  }

  for( int n = 0; n < numNoteNumbers; n ++)
    notes[n] = newNotes[n];

  if( cv != lastCv )
    sendCv(cv);
};



void sendNote( int noteNumber, bool start, int velocity)
{
  if( noteNumber >= numNoteNumbers )
  {
    Serial.printf("Bad note number %d (not 0-%d) for channel %d\n", noteNumber, numNoteNumbers-1,midiChannel);
    return;
  }
  byte buf[4];
  if( start )
    buf[0]=MIDI_START_NOTE_OP;
  else
    buf[0]=MIDI_STOP_NOTE_OP;

  buf[1]=midiChannel;
  buf[2]=noteNumbers[noteNumber];
  buf[3]=velocity;

/*
  if( start )
    Serial.printf("Start channel %d note %d vel %d\n", midiChannel, buf[2], velocity);
  else
    Serial.printf("... stop channel %d note %d vel %d\n", midiChannel, buf[2], velocity);
*/
  sendI2C(I2C_NANO, buf, 4);
};

void sendDMX(byte address, byte r, byte g, byte b)
{
  byte buf[4];
 
  buf[0]=address;
  buf[1]=r;
  buf[2]=g;
  buf[3]=b;
  sendI2C(I2C_DMX, buf, 4);
};

void sendCv( int cv)
{

  byte buf[4];
  buf[0]=CV_OP;
  buf[1]=midiChannel;
  if( cv == -1 )
    cv = 255;
  buf[2]=cv;
  buf[3]=0;
 

  sendI2C(I2C_NANO, buf, 4);
};


void sendI2C(int target, byte *buf, int n)
{
/*
  Serial.print(">>>>>>>>>>>>>>>>>>>>sent ");
  for( int i = 0; i < 4; i ++ )
    Serial.print(buf[i], HEX);
  Serial.println();
  */
  Wire.beginTransmission(target); 
  Wire.write(buf, n);
  
  Wire.endTransmission();    // stop transmitting
  
};

};