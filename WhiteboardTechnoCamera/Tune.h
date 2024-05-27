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

#define AUTODRUM_OFF 0
#define AUTODRUM_SIMPLE 1
#define AUTODRUM_MAX 2


// settings lookup values
int bpls[] = {16,32,64};
int bpms[] = {90,100,110,120,130,140, 150,160};
int scales[] = {0,1,2};
int autodrums[] = {AUTODRUM_OFF,AUTODRUM_SIMPLE,AUTODRUM_MAX};

uint8_t bassNotes[12];  // 20 to 33 is a known good range
uint8_t leadNotes[12];  // 24 to 37 is good ? 

//extern bool drawMarkers;
//extern camera_fb_t *fb;
void drawRect(camera_fb_t *fb, int x0, int y0, int w, int h, uint16_t rgb);


class Tune{

  public:

  
  
  #define TRACKS 3

  int y0 = 12;
  int ymax = 108;
  int yspace = 2;
  int hTune = (ymax-y0-2*yspace-3)/3;
  int hSetting = (ymax-y0-2*yspace)/3;

  int x0 = 21;
  int xmax = 148;

  Track tracks[TRACKS] = {Track(RD6_CHANNEL,x0, y0+2*hTune+2*yspace,   xmax-x0,hTune, 0x0f00, 5, (uint8_t*)drumNotes, true), 
                          Track(TD3_CHANNEL,x0, y0+hTune+yspace,       xmax-x0,hTune, 0xf000, 12, (uint8_t*)bassNotes, false), 
                          Track(3,          x0, y0,                    xmax-x0,hTune,  0x000f, 12, (uint8_t*)leadNotes, false)};

  int settingX = 10;
  int settingWidth = 4;

  int h3 = hSetting/2;
  int h16 = hSetting;

  int ys1 = y0 + yspace;

  Setting scaleSetting =        Setting("scale",        settingX, ys1, settingWidth, h3,0xff00,3,scales);
  int ys2 = ys1 + h3 + yspace;

  Setting BPMSetting =          Setting("BPM",          settingX, ys2, settingWidth, h16,0x00ff,8,bpms);
  int ys3 = ys2 + h16 + yspace;
  
  Setting beatPerLoopSetting =  Setting("beatsPerLoop", settingX, ys3, settingWidth, h3, 0xf00f,3,bpls);
  int ys4 = ys3 + h3 + yspace;
 

  Setting autodrumSetting =     Setting("autodrum",     settingX, ys4, settingWidth, h3, 0xff00,3,scales);
 

  
  double BPM = 120;
  //double nominalBeats = 64;
  int lastBeat = -1;
  uint32_t beatsPerLoop = 64;
  uint32_t beatDurationMillis = 8000;
  uint32_t loopStartMillis = 0;
  int32_t beat = 0;
  int lastBeatFraction = 0 ;
  long lastBeatMillis = 0;

  int lastSetScale = 0;

  int autodrum = AUTODRUM_OFF;


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
    {
      loopStartMillis = now;
      
    }

    beat = ((now - loopStartMillis) * beatsPerLoop)/loopDurationMillis;

    /*
    // fandango to keep beat consistent over change in bpm
    long millisSinceLastBeat = now-lastBeatMillis;
    long beatDurationMillis = loopDurationMillis/beatsPerLoop;
    if( millisSinceLastBeat > beatDurationMillis )
    {
      beat = 1 + (lastBeatFraction * beatsPerLoop/64); 
    

      lastBeatFraction = beat * 64/beatsPerLoop; // beat in 64-note space
      lastBeatMillis = now;
    }
    */
  };

  // called from loop() above but also from the web stream handler
  void process(bool force)
  {
  
    
    calculateBeat();

    if( beat == lastBeat && ! force)
      return;
 
    lastBeat = beat;

    if( fb == NULL )
    {
      Serial.println("Tune - no frame!");
      return;
    }

    processSettings(); // on every beat

    int beatIn4Space = beat * 16 / beatsPerLoop;  // 0 to 15
    int beatOfBar = beatIn4Space%4; // 0 to 3

    for( int t = 0; t < TRACKS; t ++ )
    {
      int autodrumNote = -1;

      if( t == 0 )
      {
        switch( autodrum )
        {
          case AUTODRUM_OFF:
            break;
          case AUTODRUM_SIMPLE:
            if(beatOfBar == 0)
              autodrumNote = 0; // bass
            break;
          case AUTODRUM_MAX:
            if(beatOfBar == 0)
              autodrumNote = 0; // bass
            else
              autodrumNote = 4; // closed snare
            break;
          default:
            break;
        }
      }

      tracks[t].processBeat(beat, beatsPerLoop,autodrumNote);
    }

    // dont' draw on the bitmap before we've analysed it
    //TODO - what if we reuse this bitmap ? Hm
    for( int t = 0; t < TRACKS; t ++ )
    {
      tracks[t].drawBoxes(beat, beatsPerLoop);
    }
  };

  void processSettings()
  {
    beatPerLoopSetting.processSetting();
    beatPerLoopSetting.drawBoxes();
    beatsPerLoop = beatPerLoopSetting.value;

    autodrumSetting.processSetting();
    autodrumSetting.drawBoxes();
    autodrum = autodrumSetting.value;

    BPMSetting.processSetting();
    BPMSetting.drawBoxes();
    BPM = BPMSetting.value;

    scaleSetting.processSetting();
    scaleSetting.drawBoxes();
    int s = scaleSetting.value;


    int bassOctave = 2;
    int leadOctave = 4;

    if( s != lastSetScale )
    {
      switch(s)
      {
        case 0:
          setScales(bassOctave,leadOctave,bluesCMajorScale, sizeof(bluesCMajorScale));
          break;
        case 1:
          setScales(bassOctave,leadOctave,bluesCMinorScale, sizeof(bluesCMinorScale));
          break;
        case 2:
        default:
          setScales(bassOctave,leadOctave,straightScale,sizeof(straightScale));
          break;
      }
      
     lastSetScale = s;   
    }
  
  }

};