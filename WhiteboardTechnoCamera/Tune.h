#pragma once


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
uint8_t bassLinear[] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33};
uint8_t leadLinear[] = {24,25,26,27,28,29,30,31,32,33,34,35,36,37};  // ove 37 doesn't seem to work the Crave properly ? 

                  // C D D# E G A
uint8_t bluesCMajor[] = { 0, 2, 3, 4, 7, 9 };

                  // C Eb F F# G Bb
uint8_t bluesCMinor[] = {0, };


uint8_t bassNotes[12];  // 20 to 33 is a known good range
uint8_t leadNotes[12];  // 24 to 37 is good ? 

extern bool drawMarkers;
extern camera_fb_t *fb;
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

  double BPM = 120;
  //double nominalBeats = 64;
  int lastBeat = -1;
  uint32_t beatsPerLoop = 64;
  uint32_t loopDurationMillis = 8000;
  uint32_t loopStartMillis = 0;


  Tune()
  {
    int b = 0;
    int o = 1;
    for(int i = 0; i < sizeof( bassNotes ); i ++)
    {
      bassNotes[i] = (o*12) + bluesCMajor[b];
      b++;
      if( b >= sizeof( bluesCMajor ))
      {
        b = 0;
        o++;
      }
    }

    b = 0;
    o = 2;
    for(int i = 0; i < sizeof( leadNotes ); i ++)
    {
      leadNotes[i] = (o*12) + bluesCMajor[b];
      b++;
      if( b >= sizeof( bluesCMajor ))
      {
        b = 0;
        o++;
      }
    }
  };

  void loop()
  {
    uint32_t now = millis();

    if( now - loopStartMillis > loopDurationMillis)
      loopStartMillis = now;

    int32_t beat = ((now - loopStartMillis) * beatsPerLoop)/loopDurationMillis;

    if( beat == lastBeat)
      return;
 
    lastBeat = beat;
    process();
  };

  void process()
  {
  
  uint32_t now = millis();

    if( now - loopStartMillis > loopDurationMillis)
      loopStartMillis = now;

    int32_t beat = ((now - loopStartMillis) * beatsPerLoop)/loopDurationMillis;

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


};