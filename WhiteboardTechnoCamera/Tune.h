#pragma once

#include "Setting.h"

#define RD6_BASS_DRUM 36
#define RD6_SNARE_DRUM 40
#define RD6_LOW_TOM  45
#define RD6_HIGH_TOM 50
#define RD6_CLAP 39
#define RD6_CYMBAL 51
#define RD6_OPEN_HAT 46
#define RD6_CLOSED_HAT 42

#define RD6_CHANNEL 1
#define TD3_CHANNEL 2


uint8_t drumNotes[] = {RD6_BASS_DRUM,RD6_SNARE_DRUM,RD6_LOW_TOM,RD6_CLAP,RD6_CLOSED_HAT};
//uint8_t bassLinear[] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33};
//uint8_t leadLinear[] = {24,25,26,27,28,29,30,31,32,33,34,35,36,37};  // over 37 doesn't seem to work the Crave properly ? 

                  // C D D# E G A
uint8_t bluesCMajorScale[] = { 0, 2, 3, 4, 7, 9 };

                  // C Eb F F# G Bb
uint8_t bluesCMinorScale[] = {0, 3, 5, 6, 7, 10 };

uint8_t straightScale[] = {0,1,2,3,4,5,6,7,8,9,10,11 };

// settings lookup values
int bpls[] = {16,32,64};
int bpms[] = {90,95,100,105,110,115,120,125,130,135};
int scales[] = {0,1,2};

uint8_t bassNotes[12];  // 20 to 33 is a known good range
uint8_t leadNotes[12];  // 24 to 37 is good ? 

//extern bool drawMarkers;
//extern camera_fb_t *fb;
void drawRect(camera_fb_t *fb, int x0, int y0, int w, int h, uint16_t rgb);


class Tune{

  public:

  
  
  #define TRACKS 3

  int y0 = 14;
  int ymax = 108;
  int yspace = 2;
  int h = (ymax-y0-2*yspace)/3;

  int x0 = 21;
  int xmax = 148;

  Track tracks[TRACKS] = {Track(RD6_CHANNEL,x0, y0+2*h+2*yspace,   xmax-x0,h, 0x0f00, 5, (uint8_t*)drumNotes), 
                          Track(TD3_CHANNEL,x0, y0+h+yspace,       xmax-x0,h, 0xf000, 12, (uint8_t*)bassNotes), 
                          Track(3,          x0, y0,               xmax-x0,h,  0x000f, 12, (uint8_t*)leadNotes)};

  int settingX = 13;
  int settingWidth = 5;

  Setting beatPerLoopSetting =  Setting("beatsPerLoop", settingX, y0+2*h+2*yspace, settingWidth,h,0xf00f,3,bpls);
  Setting BPMSetting =          Setting("BPM",          settingX, y0+h+yspace,     settingWidth,h,0x00ff,10,bpms);
  Setting scaleSetting =        Setting("scale",        settingX, y0,              settingWidth,h,0xff00,3,scales);

  double BPM = 120;
  //double nominalBeats = 64;
  int lastBeat = -1;
  uint32_t beatsPerLoop = 64;
  uint32_t beatDurationMillis = 8000;
  uint32_t loopStartMillis = 0;
  int32_t beat = 0;


  Tune()
  {
    setScales(2,1,bluesCMajorScale, sizeof(bluesCMajorScale));
  };

  void setScales(int bassOctave, int leadOctave, uint8_t *noteList, int size)
  {
    int b = 0;
    int o = bassOctave;
    for(int i = 0; i < sizeof( bassNotes ); i ++)
    {
      bassNotes[i] = (o*12) + noteList[b];
      b++;
      if( b >= size)
      {
        b = 0;            
        o++;
      }
    }

    b = 0;
    o = leadOctave;
    for(int i = 0; i < sizeof( leadNotes ); i ++)
    {
      leadNotes[i] = (o*12) + noteList[b];
      b++;
      if( b >= size)
      {
        b = 0;
        o++;
      }
    }
  };

  void loop()
  {
    process(false);
  };

  void calculateBeat()
  {
    uint32_t now = millis();

    uint32_t nominalBeatDuration = 60000/BPM;

    uint32_t loopDurationMillis = nominalBeatDuration * 16;

    if( now - loopStartMillis > loopDurationMillis)
      loopStartMillis = now;

    beat = ((now - loopStartMillis) * beatsPerLoop)/loopDurationMillis;

  };

  // called from loop() above but also from the web stream handler
  void process(bool force)
  {
  
    processSettings();

    calculateBeat();

    if( beat == lastBeat && ! force)
      return;
 
    lastBeat = beat;

    if( fb == NULL )
    {
      Serial.println("Tune - no frame!");
      return;
    }

    for( int t = 0; t < TRACKS; t ++ )
    {
      tracks[t].processBeat(beat, beatsPerLoop);
    }

    for( int t = 0; t < TRACKS; t ++ )
    {
      tracks[t].drawBoxes();
    }
  };

  void processSettings()
  {
    beatPerLoopSetting.processSetting();
    beatPerLoopSetting.drawBoxes();
    beatsPerLoop = beatPerLoopSetting.value;

    BPMSetting.processSetting();
    BPMSetting.drawBoxes();
    BPM = BPMSetting.value;

    scaleSetting.processSetting();
    scaleSetting.drawBoxes();
    int s = scaleSetting.value;

    switch(s)
    {
      case 0:
        setScales(2,1,bluesCMajorScale, sizeof(bluesCMajorScale));
        break;
      case 1:
        setScales(2,1,bluesCMinorScale, sizeof(bluesCMinorScale));
        break;
      case 2:
      default:
        setScales(2,1,straightScale,sizeof(straightScale));
        break;
    }
    
  
  }

};