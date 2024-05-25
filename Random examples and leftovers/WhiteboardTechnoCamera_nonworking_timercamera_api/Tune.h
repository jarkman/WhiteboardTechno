#pragma once

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


int drumNotes[] = {RD6_BOINK,RD6_CLAP,RD6_MEDIUM_CYMBAL};
int bassNotes[] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33};
int leadNotes[] = {50,51,52,53,54,55,56,57,58,59,50,51,52,53};

class Tune{

  public:

  
  
  #define TRACKS 3

  Track tracks[TRACKS] = {Track(RD6_CHANNEL, 10,10,300,100, 4, (int*)drumNotes), Track(TD3_CHANNEL,10,10,300,100, 12, (int*)bassNotes), Track(3,10,10,300,100, 12, (int*)leadNotes)};

  double BPM = 120;
  double nominalBeats = 16;
  int lastBeat = -1;
  uint32_t beatsPerLoop = 16;
  uint32_t loopDurationMillis = 16000;
  uint32_t loopStartMillis = 0;

  void loop()
  {
    uint32_t now = millis();

    if( now - loopStartMillis > loopDurationMillis)
      loopStartMillis = now;

    int32_t beat = ((now - loopStartMillis) * beatsPerLoop)/loopDurationMillis;

    if( beat == lastBeat)
      return;

    lastBeat = beat;


    for( int t = 0; t < TRACKS; t ++ )
    {
      tracks[t].processBeat(beat, beatsPerLoop);
    }
  };


};