/*
 * Pocket LED VU Meter
 * Original code credit to Adafruit Industries (original code: http://learn.adafruit.com/led-ampli-tie/)
 * Modified by Avery Brock. Original license owned by Adafruit Industries. 
 * 
 * Code overview
 * Reads the analog input from a microphone attached to an analog input pin. Arduino then converts input into a signal output to drive
 * a 2*10 WS2812 or "NeoPixel" matrix as a VU meter. Pressing a button attached to interrupt 0 changes the matrix from VU mode to a 
 * repeating pattern mode. See youtube video for hardware details https://youtu.be/FtzGb0rgvxo. This should work on most Arduino boards.
 * 
 * Requires Adafruit NeoPixel Library
 * 
 *Written by Adafruit Industries.  Distributed under the BSD license.
 This paragraph must be included in any redistribution.
 
 fscale function:
 Floating Point Autoscale Function V0.1
 Written by Paul Badger 2007
 Modified from code by Greg Shakar
 
 */

#include <Adafruit_NeoPixel.h>
#include <math.h>

#define N_PIXELS  20  // Number of pixels in strand
#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define LED_PIN    6  // NeoPixel LED strand is connected to this pin
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG 15 //Time of pause before peak dot falls
#define PEAK_FALL 6 //Rate of falling peak dot
#define INPUT_FLOOR 50 //Lower range of analogRead input BEST 40
#define INPUT_CEILING 400 //Max range of analogRead input, the lower the value the more sensitive (1023 = max) BEST 400



byte peak = 20;      // Peak level of column; used for falling dots
unsigned int sample;

byte dotCount = 0;  //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


 
volatile int LEDmode = 0;   // counter for the number of button presses
 

volatile int state = LOW;

void setup() 
{
  // This is only needed on 5V Arduinos (Uno, Leonardo, etc.).
  // Connect 3.3V to mic AND TO AREF ON ARDUINO and enable this
  // line.  Audio samples are 'cleaner' at 3.3V.
  // COMMENT OUT THIS LINE FOR 3.3V ARDUINOS (FLORA, ETC.):
  //  analogReference(EXTERNAL);

  // Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

attachInterrupt(0, Read, CHANGE); //button is attached to Interrupt zero

}

void loop() {

/* LEDmode 1 is a repeating pattern, when you want something less jumpy or the music is low
 *  LEDmode 2 is the VU meter output. LEDmode is changed by the interrupt button
 */


  
if(LEDmode == 2){
  LEDmode = 0;
}


while(LEDmode == 1){
   
  colorWipe(strip.Color(0, 0, 255), 150); // Blue
   colorWipe(strip.Color(255, 0,0), 150); //Red
  colorWipe(strip.Color(200, 200, 200), 150); // white
 colorWipe(strip.Color(255, 0,0), 150); //Red
  colorWipe(strip.Color(0, 0, 255), 150); // Blue
  // Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 80); // White
 theaterChase(strip.Color(127,   0,   0), 80); // Red
 theaterChase(strip.Color(127,   0,   0), 80); // Red
  theaterChase(strip.Color(  0,   0, 127), 80); // Blue

  rainbow(20);
  //rainbowCycle(20);
  theaterChaseRainbow(80);
}

while(LEDmode == 0){
    
  VUmeter(); //Adafruit VU code placed into its own function
}
}


void Read(){
  LEDmode++;

}

void VUmeter() //Adafruit Code
{
  unsigned long startMillis= millis();  // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int c, y;


  // collect data for length of sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
 
  // Serial.println(peakToPeak);


  //Fill the strip with rainbow gradient
  
  
  for (int i=0;i<=strip.numPixels()-1;i++){
    strip.setPixelColor(i,Wheel(map(i,0,strip.numPixels()-1,10,200)));
  }

  //Scale the input logarithmically instead of linearly
  c = fscale(INPUT_FLOOR, INPUT_CEILING, strip.numPixels(), 0, peakToPeak, 2);
  


  if(c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if (c <= strip.numPixels()) { // Fill partial column with off pixels
    drawLine(strip.numPixels(), strip.numPixels()-c, strip.Color(0, 0, 0));
  }

  // Set the peak dot to match the rainbow gradient
  y = strip.numPixels() - peak;
  
  strip.setPixelColor(y-1,Wheel(map(y,0,strip.numPixels()-1,10,200)));

  strip.show();

  // Frame based peak dot animation
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate 
      peak++;
      dotCount = 0;
    }
  } 
  else {
    dotHangCount++; 
  }
}

//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    strip.setPixelColor(i, c);
  }
}

float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution  
   Serial.println(); 
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
