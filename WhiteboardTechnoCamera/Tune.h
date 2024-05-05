#pragma once

class Tune{

  public:

  #define TRACKS 3
  Track tracks[TRACKS] = {Track(1, 10,10,300,100, 20, 12), Track(2,10,10,300,100, 20, 12), Track(3,10,10,300,100, 20, 12)};

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