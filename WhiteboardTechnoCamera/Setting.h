#pragma once


extern bool drawMarkers;
extern camera_fb_t *fb;

void drawRect(int x0, int y0, int w, int h, uint16_t rgb);
void drawMarker(int x0, int y0, uint16_t rgb);
uint32_t pixelBrightness( int x, int y );

class Setting {

public:
char *name;
int x1;
int y1;
int width;
int height;
uint32_t colour;
int numValues;
int value;
int *lookup;


Setting(char *_name, int _x1, int _y1, int _width, int _height, uint32_t _colour, int _numValues, int*_lookup)
{
  name = _name;
  x1 = _x1;
  y1 = _y1;
  width = _width;
  height = _height;
  colour = _colour;
  numValues = _numValues;
  lookup = _lookup;
 
};

void drawBoxes()
{
 if( drawMarkers )
  {
    //Serial.println("drawing markers");
    drawRect( x1-1, y1-1, width+2, height+2, colour);
  }
};
void processSetting( )
{
 
  int bestPos = 0;
  int bestVelocity = -1;

  double x = x1+width/2;


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
  {
    allWhite = true;
    value = lookup[0];
    return;
  }

  //Serial.printf("\n%s max %d min %d mean %d threshold %d maxB-minB %d, allWhite %d\n",name, maxB, minB, sum/height, threshold, maxB-minB, allWhite);
  

  bool inBlack = false;
  int blackStart = -1;

  int bestPosY = -1;

  int end = 3; // a bit of space at the ends to allow for width of markers

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
        int velocity = h;
        int centerY = (y+blackStart)/2;

        // chosen value
        int pos = map(centerY, y1+end, y1+height-end, numValues-1, 0);
        // center of the zone so we can draw a nice marker
        int posY = map( pos, numValues-1, 0,y1+end, y1+height-end);

        if( pos < 0 )
          pos = 0;
        if( pos > numValues-1)
          pos = numValues-1;
        /*
        int pos = ((centerY - (y1+end)) * numValues)  / (height-2*end);  // y==0 is at the top here
        int posY = y1+end + ((pos * (height-2*end))/numValues); // y coord of center of selected value
        pos = numValues - pos;
        */

        if( pos < 0 || pos >= numValues)
        {
          Serial.printf("Bad pos %d (not 0-%d)\n", pos, numValues-1);
        }
        else
        {
          if( velocity > bestVelocity )
          {
            bestPos = pos;
            bestVelocity = velocity;
            bestPosY = posY;

            //Serial.printf("found setting %s pos %d \n", name, pos);
        }

       
      }
    
    }
  }

  if( bestPos >= 0 && bestPos < numValues )
  {
    int newValue = lookup[bestPos];
    if( value != newValue )
      Serial.printf("%s : %d (was %d)\n", name, newValue, value);

    value = newValue;

    if( drawMarkers)
         drawMarker(x, bestPosY, colour);
  }
  else
  {
    value = lookup[0];
    Serial.printf("%s Bad bestPos %d (not 0-%d)\n", name, bestPos, numValues-1);
  }

};




};