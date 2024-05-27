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
int dmxAddress;
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

Track(int _midiChannel, int _dmxAddress, int _x1, int _y1, int _width, int _height, uint32_t _colour, int _numNoteNumbers, uint8_t *_noteNumbers, bool _alwaysNew)
{
  midiChannel = _midiChannel;
  dmxAddress = _dmxAddress;
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
    drawRect( x1-1, y1-1, width+2, height+2, colour);

    double x = map(beat, 0, beatsPerLoop-1, x1, x1 + width );

    for( int b = 0; b < beatsPerLoop; b ++)
    {
      double x = map(b, 0, beatsPerLoop-1, x1, x1 + width );
      drawRect( x, y1-1, 1, b==beat?-2:-1, colour);

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
  {
    newNotes[autodrum]=true;
    Serial.printf("autodrum %d\n", autodrum);
  }
  else
  {
    if( autodrum != -1 )
      Serial.printf("bad autodrum %d\n", autodrum);
  }

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

  uint32_t threshold = minB + (maxB+minB)/5;

  
  if( maxB - minB < threshold )
    allWhite = true;

  //Serial.printf("\nmax %d min %d mean %d threshold %d maxB-minB %d, allWhite %d\n", maxB, minB, sum/height, threshold, maxB-minB, allWhite);
  

  bool inBlack = false;
  int blackStart = -1;

  byte colourWheel = 0;

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

        colourWheel = cv;

        if( pos < 0 || pos > numNoteNumbers)
        {
          Serial.printf("Bad pos %d (not 0-%d)", pos, numNoteNumbers);
        }
        else
        {
          newNotes[pos] = true;
          velocities[pos] = velocity;
          //if( midiChannel == 2 )
          //  Serial.printf("found note channel %d beat %d x %d at pos %d noteNumber %d\n",  (int) midiChannel, (int) beat, (int) x, (int) pos, (int) noteNumbers[pos]);
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
        
      if( newNotes[n] )
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

  if( dmxAddress > 0 )
    sendWheelDMX(dmxAddress,colourWheel);
 
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

void sendWheelDMX(byte address, byte WheelPos) // use wheel (ie, hue, kind of) so we get roughly constant brightness so we don't bemuse the camera
{
  //Serial.printf("wheel %d  ", WheelPos);

    byte r,g,b;

  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    r = 255 - WheelPos * 3; g = 0; b = WheelPos * 3;
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    r = 0; g = WheelPos * 3; b = 255 - WheelPos * 3;
  }
  WheelPos -= 170;
  r = WheelPos * 3; g = 255 - WheelPos * 3; b = 0;

  sendDMX(address,r,g,b);

/*
  if( WheelPos >= 255 )
    WheelPos = 254;

  if( WheelPos == 0 )
    WheelPos = 1;

  
  byte r,g,b;
  switch(WheelPos >> 5)
  {
    case 0:
      r=31- WheelPos % 32;   //Red down
      g=WheelPos % 32;      // Green up
      b=0;                  //blue off
      break; 
    case 1:
      g=31- WheelPos % 32;  //green down
      b=WheelPos % 32;      //blue up
      r=0;                  //red off
      break; 
    case 2:
      b=31- WheelPos % 32;  //blue down 
      r=WheelPos % 32;      //red up
      g=0;                  //green off
      break; 
  }
  r = r<<4;
  g = g<<4;
  b = b<< 4;
  sendDMX(address,r,g,b);

  */
}

void sendDMX(byte address, byte r, byte g, byte b)
{
  byte buf[4];
 
  buf[0]=address;
  buf[1]=r;
  buf[2]=g;
  buf[3]=b;

  //Serial.printf("SendDMX %d  %d %d %d\n", address, r, g, b);
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