#define  FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include "Buttons.h"

// Use qsuba for smooth pixel colouring and qsubd for non-smooth pixel colouring
#define qsubd(x, b)  ((x>b)?b:0)    // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)  // Analog Unsigned subtraction macro. if result <0, then => 0

const int ledCount = 300;
const int longestTube = 273;
  
// there are 16 virtual led strips (14 on tubes, one for the tail lights and one for the ground effect lights)
// three of the strips are on one pin and two are on another.
const byte numStrips = 16;
CRGB leds[numStrips][longestTube];

// I'm using the teensy 3.2 with 16 pins (DMA)
// I'm not using 3 of the pins, but I'm keeping this retangular for ease of thinking
const byte numVirtualStrips = 16;
CRGB realLeds[numVirtualStrips][ledCount];

// there are 14 tubes (the front is atached to strip 2 and the 2nd is attached to tube 1)
// zk:2022 not sure why numtubes and numstrips are different
const byte numTubes = 16;

//const byte tubes[numStrips] =   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};
const byte tubes[numStrips] =     { 8, 0, 1, 7, 2, 3, 4, 6,10, 5, 9,12,15,14,11,13};

byte tailLights = 15;
byte groundEffect = 0;

const byte UPDATES_PER_SECOND = 200;
const byte FRAMES_PER_SECOND = 240;

// the front of the car has shorter lights
const int tubeLength[numStrips] = {16, 79, 136, 163, 201, 254, 273, 273, 273, 273, 273, 273, 273, 273, 273,100} ;
//const int tubeLength[numStrips] = {16,16,16,16,16,16,16,16,16,16,16,16,16,16};
CButtons *buttons = new CButtons;

// globals for FX animations
int BOTTOM_INDEX = 0;
int TOP_INDEX = int(longestTube/2);
int FIRST_THIRD = int(longestTube/3);
int SECOND_THIRD = FIRST_THIRD * 2;
int EVENODD = longestTube%2;


CRGB ledsX[longestTube]; //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, ETC)

//-PERISTENT VARS
int idex = 0;        //-LED INDEX (0 to longestTube-1
byte ihue = 0;        //-HUE (0-360)
int ibright = 0;     //-BRIGHTNESS (0-255)
int isat = 0;        //-SATURATION (0-255)
bool bounceForward = true;  //-SWITCH FOR COLOR BOUNCE (0-1)
int bouncedirection = 0;
float tcount = 0.0;      //-INC VAR FOR SIN LOOPS
int lcount = 0;      //-ANOTHER COUNTING VAR

// color palette related stuff
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;
TBlendType currentBlending = LINEARBLEND;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// The 16 bit version of our coordinates
static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
uint16_t speed = 20; // speed is set dynamically once we've started up

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 30; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
uint8_t noise[numTubes][longestTube];

const uint8_t kMatrixWidth = numTubes;
const uint8_t kMatrixHeight = longestTube/2;

uint8_t colorLoop = 1;

// i don't know why I need to declare these...
void Fire2012WithPalette();
void rotatingRainbow(); 
void rainbowWithGlitter();
void spiralRainbow();
void police_lightsALL();
void sinelon();
void juggle();
void mapNoiseToLEDsUsingPalette();

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

//------------------SETUP------------------
void setup()
{
  delay(500);

  Serial.begin(57600);


  // there's an unknown problem with PORTDC, so this is code I got from daniel garcia (may he rest in peace)
  // (https://forum.pjrc.com/threads/31482-Problem-with-FastLED-and-16-way-parallel-output?highlight=WS2811_PORTDC)
  // to use PORTD and PORTC seperately
  //LEDS.addLeds<WS2811_PORTDC,16, RGB>(*realLeds, ledCount);
  LEDS.addLeds<WS2811_PORTD,8, RGB>(*realLeds, ledCount);
  LEDS.addLeds<WS2811_PORTC,8, RGB>(*realLeds + (ledCount * (8)), ledCount);

  LEDS.setBrightness(16);
  //LEDS.setBrightness(32);

  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

  digitalWrite(3, LOW);
        
  currentPalette = HeatColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(57600);
  Serial.flush();
  for(byte tube = 0;tube < numTubes;tube++)
    fill_solid(realLeds[tube],ledCount,0); //-BLANK STRIP
    
  showLeds();
  Serial.println("https://github.com/zekekoch/QueenFX");
  printStrips();
}

void printStrips()
{
    for (byte iStrip = 0;iStrip<numStrips;iStrip++)
    {
        Serial.print("[");
        Serial.print(iStrip);
        if (iStrip == tailLights)
            Serial.print("->");
        else
            Serial.print(":");
        Serial.print(tubes[iStrip]);
        Serial.print("]");
    }
}

// roughly map leds dealing with the fact that
// I want to stay integer math, but need fractions
// this is sloppy, but close enough for now
int mapLed(int led, int from, int to)
{
    if (from == to)
        return led;
    else
        return led * ((from * 1024)/to) / (1024); 
}

// the leds for the tubes are in an array called leds
// the array that FastLED uses to write to the actual strips is 
// called realLeds.
//
// I need to copy the data from the leds array to the realLeds array 
// taking into consideration the fact that I'm sharing some led strips
// between different physical tubes (tube 0 and 1 share the same pin as tube 4),
// and tubes 2 and 3 share another pin.
// To make things slightly more difficult the secondary tubes are backwards and 
// the order of the strips on my art car doesn't match the order of FastLeds' parallel output.
// phew.
void showLeds()
{
  for(int currentStrip = 0; currentStrip < numStrips; currentStrip++) 
  {
      // map from my model (the order of the strips on the car) to fastled
      // to be honest I don't know how fastled determines the order. it's not
      // based on the pin numbers so it must have something to do with the dma

    // ground effects come first, they're stored at the end of pin 4
    if(currentStrip == 0)
    {
        for(int iLed = 0; iLed < tubeLength[currentStrip]; iLed++) 
        {
            realLeds[tubes[4]][tubeLength[4] + tubeLength[1] + iLed + 1] = leds[currentStrip][iLed];
        }
    }
    // tube 1 also shares a pin with tube 4 (but it's backwards)
    else if(currentStrip == 1)
    {
        // virtual strip 0 is on the same strip as physical tube 3 and backwards!
        for(int iLed = 0; iLed < tubeLength[currentStrip]; iLed++) 
        {
            realLeds[tubes[4]][tubeLength[currentStrip] + tubeLength[4]-iLed - 1] = leds[currentStrip][iLed];
        }
    }
    // tubes 2 and 3 share pin 3 (and 2 is backwards)
    else if(currentStrip == 2)
    {
        // this one is backwards
        for(int iLed = 0; iLed < tubeLength[currentStrip]; iLed++) 
        {
            realLeds[tubes[3]][tubeLength[currentStrip] + tubeLength[3]-iLed -1] = leds[currentStrip][iLed];
        }
    }
    else
    {
        for(int iLed = 0; iLed < tubeLength[currentStrip]; iLed++) 
        {
            realLeds[tubes[currentStrip]][iLed] = leds[currentStrip][iLed];
        }
    }
  }

  LEDS.show();
}

void colorPaletteLoop()
{
    ChangePalettePeriodically();
    
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors( startIndex, 3);
    
    showLeds();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex, uint8_t width)
{
    uint8_t brightness = 255;
    
    for(int iStrip = 0;iStrip < numTubes;iStrip++)
    {
        for( int iLed = 0; iLed < longestTube;iLed++) 
        {
            leds[iStrip][iLed] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
            // how quickly you move through the colors is how wide the strips are of a given color
            colorIndex+=width;
        }
    }
}

// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 10)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}

// This function sets up a palette of purple and green stripes.
void SetupAmericaPalette()
{
    currentPalette = CRGBPalette16
    (
        CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red,  
        CRGB::Red, CRGB::White, CRGB::White, CRGB::White, 
        CRGB::White, CRGB::White, CRGB::White, CRGB::Blue, 
        CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue 
    );
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    // Gray looks better than White because white is brighter than blue and red
    CRGB::Red, CRGB::Gray, CRGB::Blue, CRGB::Black,
    CRGB::Red, CRGB::Gray, CRGB::Blue, CRGB::Black,
    CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray,    
    CRGB::Blue, CRGB::Blue, CRGB::Black, CRGB::Black
};


//------------------------------------- UTILITY FXNS --------------------------------------

//-SET THE COLOR OF A SINGLE RGB LED on all tubes
void setPixel(int adex, int cred, int cgrn, int cblu) {
    if (adex < 0 || adex > longestTube-1)
        return;

    for(int i = 0;i<numTubes;i++)
    {
        leds[i][adex] = CRGB(cred, cgrn, cblu);
    }
}

void setPixel(int adex, CRGB c) {
    if (adex < 0 || adex > longestTube-1)
        return;

    for(int i = 0;i<numTubes;i++)
    {
        leds[i][adex] = c;
    }
}

// assumes all of the strips have the same stuff on them
CRGB getPixel(int pixel)
{
    return leds[0][pixel];
}

//-FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
    //-ONLY WORKS WITH INDEX < TOPINDEX
    if (i == BOTTOM_INDEX) {
        return BOTTOM_INDEX;
    }
    if (i == TOP_INDEX && EVENODD == 1) {
        return TOP_INDEX + 1;
    }
    if (i == TOP_INDEX && EVENODD == 0) {
        return TOP_INDEX;
    }
    return longestTube - i;
}


//-FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
    //int N2 = int(longestTube/2);
    int iN = i + TOP_INDEX;
    if (i >= TOP_INDEX) {
        iN = ( i + TOP_INDEX ) % longestTube; 
    }
    return iN;
}

int nextThird(int i) {
    int iN = i + (int)(longestTube / 3);
    if (iN >= longestTube) {
        iN = iN % longestTube;
    }
    return iN;
}

int adjacent_cw(int i) {
    int r;  
    if (i < 0) {
        r = 0;
    }
    else if (i >= longestTube - 1) {
        r = longestTube;
    }
    else {
        r = i + 1;
    }

    //Serial.print("cw from ");Serial.print(i);Serial.print(" to ");Serial.print(r);Serial.println();
    return r;
}

//-FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
    int r;

    if (i <= 0) {
        r = 0;
    }
    else if (i > longestTube) {
        r = longestTube - 1;
    }
    else {
        r = i - 1;
    }

    //Serial.print("ccw from ");Serial.print(i);Serial.print(" to ");Serial.print(r);Serial.println();
    return r;
}

//-CONVERT HSV VALUE TO RGB
void HSVtoRGB2(int hue, int sat, int val, int colors[3]) {
    Serial.println("depricate HSVtoRGB with int array");
    CRGB c;
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
    colors[0] = c.r;
    colors[1] = c.g;
    colors[2] = c.b;
}

void HSVtoRGB(int hue, int sat, int val, CRGB& c) {
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
}

CRGB HSVtoRGB(int hue, int sat, int val) {
    CRGB c;
    hsv2rgb_rainbow(CHSV(hue, sat, val), c);
    return c;
}

// todo: make this work with multiple arrays
void copy_led_array(){
    for(int i = 0; i < longestTube; i++ ) {
        ledsX[i][0] = leds[0][i].r;
        ledsX[i][1] = leds[0][i].g;
        ledsX[i][2] = leds[0][i].b;
    }
}

// todo: make this work with multiple arrays
void print_led_arrays(int ilen){
    copy_led_array();
    Serial.println("~~~***ARRAYS|idx|led-r-g-b|ledX-0-1-2");
    for(int i = 0; i < ilen; i++ ) {
        Serial.print("~~~");
        Serial.print(i);
        Serial.print("|");
        Serial.print(leds[0][i].r);
        Serial.print("-");
        Serial.print(leds[0][i].g);
        Serial.print("-");
        Serial.print(leds[0][i].b);
        Serial.print("|");
        Serial.print(ledsX[i][0]);
        Serial.print("-");
        Serial.print(ledsX[i][1]);
        Serial.print("-");
        Serial.println(ledsX[i][2]);
    }
}

//------------------------LED EFFECT FUNCTIONS------------------------

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[random8(numTubes)][ random16(longestTube-1) ] += CRGB::White;
  }
}

void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
    byte lineStartHue = startHue8;
    for( byte y = 0; y < kMatrixHeight; y++) 
    {
        lineStartHue += yHueDelta8;
        byte pixelHue = lineStartHue;
        for( byte x = 0; x < kMatrixWidth; x++) 
        {
            pixelHue += xHueDelta8;
            leds[x][y] = CHSV( pixelHue, 255, 255);
        }
    }
    mirror();
}


void fillSolid(byte strand, CRGB color)
{
    // fill_solid -   fill a range of LEDs with a solid color
 fill_solid( leds[strand], tubeLength[strand], color);
}

void fillSolid(CRGB color)
{
    for(int i = 0;i<numTubes;i++)
        fillSolid(i, color);
}

void fillSolid(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    fillSolid(0, CRGB(cred, cgrn, cblu));    
}

void plasma() 
{ // This is the heart of this program. Sure is short. . . and fast.
  int thisPhase = beatsin8(6,-64,64);                           // Setting phase change for a couple of waves.
  int thatPhase = beatsin8(7,-64,64);

  // For each of the LED's in the strand, set a brightness based on a wave as follows:
  for (int k=0; k<longestTube; k++) {                              
    int colorIndex = cubicwave8((k*23)+thisPhase)/2 + cos8((k*15)+thatPhase)/2;           // Create a wave and add a phase change and add another wave with its own phase change.. Hey, you can even change the frequencies if you wish.
    int thisBright = qsuba(colorIndex, beatsin8(7,0,96));                                 // qsub gives it a bit of 'black' dead space by setting sets a minimum value. If colorIndex < current value of beatsin8(), then bright = 0. Otherwise, bright = colorIndex..


    // Let's now add the foreground colour. setpixel sets the same animation on all tubes
    setPixel(k,ColorFromPalette(currentPalette, colorIndex, thisBright, currentBlending));
  }

} // plasma()

void blendwave() 
{
  static CRGB clr1;
  static CRGB clr2;
  static uint8_t speed;
  static uint8_t loc1;

  
    for(int i = 0;i < numTubes;i++)
    {
  speed = beatsin8(6,0,255);

  clr1 = blend(CHSV(beatsin8(3,0,255),255,255), CHSV(beatsin8(4,0,255),255,255), speed);
  clr2 = blend(CHSV(beatsin8(4,0,255),255,255), CHSV(beatsin8(3,0,255),255,255), speed);

  loc1 = beatsin8(10,0,longestTube-1);

        fill_gradient_RGB(leds[i], 0, clr2, loc1, clr1);
        fill_gradient_RGB(leds[i], loc1, clr2, longestTube-1, clr1);
    }
} // blendwave()

void ripple() {
    static uint8_t colour;                                               // Ripple colour is randomized.
    static int center = 0;                                               // Center of the current ripple.
    static int step = -1;                                                // -1 is the initializing step.
    static uint8_t myfade = 255;                                         // Starting brightness.
    const byte maxsteps = 16;                                           // Case statement wouldn't allow a variable.

    for(byte iStrip =0;iStrip < numTubes;iStrip++)
    {
        fadeToBlackBy( leds[iStrip], longestTube, 20);
    } 
    
    switch (step) {
        case -1:                                                          // Initialize ripple variables.
        center = random(longestTube);
        colour = random8();
        step = 0;
        break;

        case 0:
        setPixel(center,ColorFromPalette(currentPalette, colour, myfade, currentBlending));
        
        step ++;
        break;

        case maxsteps:                                                    // At the end of the ripples.
        step = -1;
        break;

        default:                                                          // Middle of the ripples.
        // Simple wrap from Marc Miller
        setPixel((center + step + longestTube) % longestTube, getPixel((center + step + longestTube) % longestTube) + ColorFromPalette(currentPalette, colour, myfade/step*2, currentBlending));       
        setPixel((center - step + longestTube) % longestTube, getPixel((center + step + longestTube) % longestTube) + ColorFromPalette(currentPalette, colour, myfade/step*2, currentBlending));
        step ++;                                                         // Next step.
        break;  
    } // switch step
  
} // ripple()

void rotatingRainbow()
{
    static byte hue = 0;
    for(int i = 0;i < numTubes;i++)
    {
        fill_rainbow(leds[i], longestTube, hue++, 10);
        if(buttons->isIntensifierOn())  
            addGlitter(255);
    }
}

void spiralRainbow()
{
    static CHSV hsv = CHSV(0,255,255);
    static unsigned long lastTime = millis();
    static byte iStrip = 0;

    if (millis() < lastTime + 1)
    {
        return;
    }

    iStrip++;
    if (iStrip == numTubes)
        iStrip = 0;

    hsv.hue += 10;

    for(int i = 0; i < numTubes;i++)
    {
        fadeToBlackBy(leds[i], longestTube, 30);
    }

    for( int iLed = 0; iLed < longestTube; iLed++) 
    {
        leds[iStrip][iLed] = hsv;
    }



    lastTime = millis();
}

void juggle() 
{
  // eight colored dots, weaving in and out of sync with each other
    for(byte iStrip =0;iStrip < numTubes;iStrip++)
    {
        fadeToBlackBy( leds[iStrip], longestTube, 20);
    }
    
    byte dothue = 0;
    for( int i = 0; i < numTubes; i++) {
        leds[i][beatsin16(i+7,0,longestTube/2)] |= CHSV(dothue, 200, 255);
    dothue += 32;
    }
    mirror();

}

void simpleColors()
{
  static byte d = 0;
  byte hue = d;
  for(byte tube = 0;tube<numTubes;tube++)
  {
      fillSolid(tube, CHSV(hue, 255, 255));
      hue += 50;
  }
  d+=1;
}

void sinelon()
{
    static byte gHue;
  // a colored dot sweeping back and forth, with fading trails
    for(byte i = 0;i<numTubes;i++)
    {

        fadeToBlackBy( leds[i], longestTube, 20);
        int ypos = beatsin16(13,0,longestTube);
        leds[i][ypos] += CHSV( gHue, 255, 192);
    }
}

void rainbow_fade() { //-FADE ALL LEDS THROUGH HSV RAINBOW
    ihue++;
    CRGB thisColor;
    HSVtoRGB(ihue, 255, 255, thisColor);
    for(int idex = 0 ; idex < longestTube; idex++ ) {
        setPixel(idex,thisColor);
    }
}


void rainbow_loop(int istep, int idelay) { //-LOOP HSV RAINBOW
    static int idex = 0;
    static int offSet = 0;
    idex++;
    ihue = ihue + istep;
    
    if (idex >= longestTube) {
        idex = ++offSet;
    }
    
    CRGB icolor;
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void random_burst(int idelay) { //-RANDOM INDEX/COLOR
    CRGB icolor;
    
    idex = random(0,longestTube);
    ihue = random(0,255);
    
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void color_bounce(int idelay) { //-BOUNCE COLOR (SINGLE LED)
    if (bounceForward)
    {
        idex = idex + 1;
        if (idex == longestTube)
        {
            bounceForward = false;
            idex = idex - 1;
        }
    }
    else
    {
        idex = idex - 1;
        if (idex == 0)
        {
            bounceForward = true;
        }
    }

    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, CRGB::Red);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void police_lightsONE() { //-POLICE LIGHTS (TWO COLOR SINGLE LED)
    idex++;
    if (idex >= longestTube) {
        idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idexR) {
            setPixel(i, CRGB::Red);
        }
        else if (i == idexB) {
            setPixel(i, CRGB::Blue);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}

void police_lightsALL()
{
    static byte colorIndex;
    SetupAmericaPalette();
    currentBlending = LINEARBLEND;

    // the first variable is the speed at which the lights march, the second is the width of the bars
    FillLEDsFromPaletteColors(colorIndex+=12, 3);
}

void police_lightsALLOld() { //-POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= longestTube) {idex = 0;}
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    setPixel(idexR, 255, 0, 0);
    setPixel(idexB, 0, 0, 255);
}

void fourthOfJuly() { //-red, white and blue
    idex++;
    if (idex >= longestTube) {idex = 0;}
    int idexR = idex;
    int idexW = nextThird(idexR);
    int idexB = nextThird(idexW);
    setPixel(idexR, CRGB::Red);
    setPixel(idexW, CRGB::White);
    setPixel(idexB, CRGB::Blue);
}

void yxyy() { //-red, white and blue
    idex++;
    if (idex >= longestTube) 
    {
      idex = 0;
    }
    int idexR = idex;
    int idexW = nextThird(idexR);
    int idexB = nextThird(idexW);
    setPixel(idexR, CRGB::DarkBlue);
    setPixel(idexW, CRGB::Orange);
    setPixel(idexB, CRGB::Fuchsia);
}


/*
void musicReactiveFade(byte eq[7]) { //-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
    static long lastBounceTime;
    const int bounceInterval = 500;

    int bass = (eq[0] + eq[1] + eq[3]) / 3;
    int high = (eq[4] + eq[5] + eq[6]) / 3;
    Serial.print("init bass ");Serial.print(bass);Serial.print(" and high ");Serial.println(high);
    
    
    if((bass > 150) && (millis() > (lastBounceTime + bounceInterval)))
    {
        Serial.println("reversing");
        bounceForward = !bounceForward;
        lastBounceTime = millis();
    }
    
    byte trailLength = map(bass, 0,255, 3,19);
    //Serial.print("trailLength: ");Serial.println(trailLength);
    byte trailDecay = (255-64)/trailLength;
    //Serial.print("trailDecay: ");Serial.println(trailDecay);
    byte hue = high;

    fillSolid(CRGB::Black);
    
    if (bounceForward) {
        Serial.print("bouncing forward idex:");Serial.print(idex);Serial.println();

        if (idex < longestTube) {
            idex++;
        } else if (idex == longestTube) {
            bounceForward = !bounceForward;
        }

        for(int i = 0;i<trailLength;i++)
        {
            //Serial.print(" to ");Serial.println(adjacent_cw(idex+i));
            setPixel(adjacent_cw(idex-i), HSVtoRGB(hue, 255, 255 - trailDecay*i));
        }
    } else {
        if (idex > longestTube) {
            idex = longestTube;
        }
        if (idex >= 0) {
            idex--;
        } else if (idex == 0) {
            bounceForward = !bounceForward;
        } else
        {
            idex = 0;
        }

        Serial.print("bouncing backwards idex:");Serial.print(idex);Serial.println();
        for(int i = 0;i<trailLength;i++)
        {
            //Serial.print(" to ");Serial.println(adjacent_ccw(idex+i));
            setPixel(adjacent_ccw(idex+i), HSVtoRGB(hue, 255, 255 - trailDecay*i));
        }
    }    
}
*/


void color_bounceFADE(int idelay) { //-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
    if (bouncedirection == 0) {
        idex++;
        if (idex == longestTube) {
            bouncedirection = 1;
            idex--;
        }
    }
    if (bouncedirection == 1) {
        idex--;
        if (idex == 0) {
            bouncedirection = 0;
        }
    }
    int iL1 = adjacent_cw(idex);
    int iL2 = adjacent_cw(iL1);
    int iL3 = adjacent_cw(iL2);
    int iR1 = adjacent_ccw(idex);
    int iR2 = adjacent_ccw(iR1);
    int iR3 = adjacent_ccw(iR2);
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, CRGB(255, 0, 0));
        }
        else if (i == iL1) {
            setPixel(i, CRGB(100, 0, 0));
        }
        else if (i == iL2) {
            setPixel(i, CRGB(50, 0, 0));
        }
        else if (i == iL3) {
            setPixel(i, CRGB(10, 0, 0));
        }
        else if (i == iR1) {
            setPixel(i, CRGB(100, 0, 0));
        }
        else if (i == iR2) {
            setPixel(i, CRGB(50, 0, 0));
        }
        else if (i == iR3) {
            setPixel(i, CRGB(10, 0, 0));
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}

void flicker(int thishue, int thissat) {
    int random_bright = random(0,255);
    int random_bool = random(0,random_bright);
    CRGB thisColor;
    
    if (random_bool < 10) {
        HSVtoRGB(thishue, thissat, random_bright, thisColor);
        
        for(int i = 0 ; i < longestTube; i++ ) {
            setPixel(i, thisColor);
        }
    }
}


void pulse_one_color_all(int ahue, int idelay) { //-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        ibright++;
        if (ibright >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        ibright = ibright - 1;
        if (ibright <= 1) {bouncedirection = 0;}
    }
    
    CRGB acolor;
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    for(int i = 0 ; i < longestTube; i++ ) {
        setPixel(i, acolor);
    }
}


void pulse_one_color_all_rev(int ahue, int idelay) { //-PULSE SATURATION ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        isat++;
        if (isat >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        isat = isat - 1;
        if (isat <= 1) {bouncedirection = 0;}
    }
    
    CRGB acolor;
    HSVtoRGB(ahue, isat, 255, acolor);
    
    for(int i = 0 ; i < longestTube; i++ ) {
        setPixel(i, acolor);
    }
}

void random_march(int idelay) { //RANDOM MARCH CCW
    copy_led_array();
    int iCCW;
    
    CRGB acolor;
    HSVtoRGB(random(0,360), 255, 255, acolor);
    setPixel(0, acolor);
    
    for(int i = 1; i < longestTube;i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW]);
    }
}


void rwb_march(int idelay) { //R,W,B MARCH CCW
    copy_led_array();
    int iCCW;
    
    idex++;
    if (idex > 2) {idex = 0;}
    
    switch (idex) {
        case 0:
            setPixel(0, CRGB::Red);
            break;
        case 1:
            setPixel(0, CRGB::White);
            break;
        case 2:
            setPixel(0, CRGB::Blue);
            break;
    }
    
    for(int i = 1; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
}


void white_temps() {
    int N9 = int(longestTube/9);
    for (int i = 0; i < longestTube; i++ ) {
        if (i >= 0 && i < N9)
            {setPixel(i, 255,147,41);} //-CANDLE - 1900
        if (i >= N9 && i < N9*2)
                {setPixel(i, 255,197,143);} //-40W TUNG - 2600
        if (i >= N9*2 && i < N9*3)
                {setPixel(i, 255,214,170);} //-100W TUNG - 2850
        if (i >= N9*3 && i < N9*4)
                {setPixel(i, 255,241,224);} //-HALOGEN - 3200
        if (i >= N9*4 && i < N9*5)
                {setPixel(i, 255,250,244);} //-CARBON ARC - 5200
        if (i >= N9*5 && i < N9*6)
                {setPixel(i, 255,255,251);} //-HIGH NOON SUN - 5400
        if (i >= N9*6 && i < N9*7)
                {setPixel(i, 255,255,255);} //-DIRECT SUN - 6000
        if (i >= N9*7 && i < N9*8)
                {setPixel(i, 201,226,255);} //-OVERCAST SKY - 7000
        if (i >= N9*8 && i < longestTube)
                {setPixel(i, 64,156,255);} //-CLEAR BLUE SKY - 20000
    }
}


void color_loop_vardelay() { //-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
    idex++;
    if (idex > longestTube) {idex = 0;}
    
    CRGB acolor;
    HSVtoRGB(0, 255, 255, acolor);
    
    //int di = abs(TOP_INDEX - idex); //-DISTANCE TO CENTER
    //int t = constrain((10/di)*10, 10, 500); //-DELAY INCREASE AS INDEX APPROACHES CENTER (WITHIN LIMITS)
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == idex) {
            setPixel(i, acolor);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void strip_march_cw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCCW;
    for(int i = 0; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW]);
    }
}


void strip_march_ccw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCW;
    for(int i = 0; i < longestTube; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCW = adjacent_cw(i);
        setPixel(i, ledsX[iCW]);
    }
}


void pop_horizontal(int ahue, int idelay) {  //-POP FROM LEFT TO RIGHT UP THE RING
    CRGB acolor;
    HSVtoRGB(ahue, 255, 255, acolor);
    
    int ix =0;
    
    if (bouncedirection == 0) {
        bouncedirection = 1;
        ix = idex;
    }
    else if (bouncedirection == 1) {
        bouncedirection = 0;
        ix = horizontal_index(idex);
        idex++;
        if (idex > TOP_INDEX) {
            idex = 0;
        }
    }
    
    for(int i = 0; i < longestTube; i++ ) {
        if (i == ix) {
            setPixel(i, acolor);
        }
        else {
            setPixel(i, CRGB::Black);
        }
    }
}


void quad_bright_curve(int ahue, int idelay) {  //-QUADRATIC BRIGHTNESS CURVER
    CRGB acolor;
    int ax;
    
    for(int x = 0; x < longestTube; x++ ) {
        if (x <= TOP_INDEX) {ax = x;}
        else if (x > TOP_INDEX) {ax = longestTube-x;}
        
        int a = 1; int b = 1; int c = 0;
        
        int iquad = -(ax*ax*a)+(ax*b)+c; //-ax2+bx+c
        int hquad = -(TOP_INDEX*TOP_INDEX*a)+(TOP_INDEX*b)+c; //HIGHEST BRIGHTNESS
        
        ibright = int((float(iquad)/float(hquad))*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(x,acolor);
    }
}


void flame() {
    CRGB acolor;
//    int idelay = random(0,35);
    
    float hmin = 0.1; float hmax = 45.0;
    float hdif = hmax-hmin;
    int randtemp = random(0,3);
    float hinc = (hdif/float(TOP_INDEX))+randtemp;
    
    int ahue = hmin;
    for(int i = 0; i < TOP_INDEX; i++ ) {
        
        ahue = ahue + hinc;
        
        HSVtoRGB(ahue, 255, 255, acolor);
        setPixel(i,acolor);
        int ih = horizontal_index(i);
        setPixel(ih,acolor);
        setPixel(TOP_INDEX,CRGB::White);
    }
}


void radiation(int ahue, int idelay) { //-SORT OF RADIATION SYMBOLISH-
    //int N2 = int(longestTube/2);
    int N3 = int(longestTube/3);
    int N6 = int(longestTube/6);
    int N12 = int(longestTube/12);
    CRGB acolor;
    
    for(int i = 0; i < N6; i++ ) { //-HACKY, I KNOW...
        tcount = tcount + .02;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        int j0 = (i + longestTube - N12) % longestTube;
        int j1 = (j0+N3) % longestTube;
        int j2 = (j1+N3) % longestTube;
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(j0,acolor);
        setPixel(j1,acolor);
        setPixel(j2,acolor);
        
    }

}

void sin_bright_wave(int ahue, int idelay) {
    CRGB acolor;
    
    for(int i = 0; i < longestTube; i++ ) {
        tcount = tcount + .1;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(i, acolor);
    }
}


void fade_vertical(int ahue, int idelay) { //-FADE 'UP' THE LOOP
    CRGB acolor;
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    ibright = ibright + 10;
    
    if (ibright > 255) {
        ibright = 0;
    }
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    setPixel(idexA, acolor);
    setPixel(idexB, acolor);
}


void rainbow_vertical(int istep, int idelay) { //-RAINBOW 'UP' THE LOOP
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    ihue = ihue + istep;
    if (ihue > 255)
    {
        ihue = 0;
    }
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    CRGB acolor;
    HSVtoRGB(ihue, 255, 255, acolor);
    
    setPixel(idexA, acolor);
    setPixel(idexB, acolor);

}

boolean checkButton(byte whichButton)
{
    static unsigned long lastTime = millis();
    if (millis() > lastTime + 500)
    {
        if (0 == whichButton)
        {
            return buttons->isIntensifierOn();
        }
    }
    
    return false;
}

void loop()
{
    // first update all of my buttons to check if any are down
    buttons->refresh();

    if(buttons->state(0))
    {
        currentPalette = HeatColors_p;
        Fire2012WithPalette();
        Serial.print("button state 0:");Serial.println(buttons->state(0));
    } 
    else if(buttons->state(1))
    {
        currentPalette =  CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
        Fire2012WithPalette();
        Serial.print("button state 1:");Serial.println(buttons->state(1));
    } 
    else if (buttons->state(2))
    {
        rotatingRainbow(); 
        Serial.print("button state 2:");Serial.println(buttons->state(2));
    }
    else if (buttons->state(3))
    {
        spiralRainbow();
        Serial.print("button state 3:");Serial.println(buttons->state(3));
    }
    else if (buttons->state(4))
    {
        sinelon();
        Serial.print("button state 4:");Serial.println(buttons->state(4));
    }
    else if (buttons->state(5))
    {
        juggle();
        Serial.print("button state 5:");Serial.println(buttons->state(5));
    }
    else if (buttons->state(6))
    {
        currentPalette = HeatColors_p;
        // generate noise data
        fillnoise8();
        
        // convert the noise data to colors in the LED array
        // using the current palette
        mapNoiseToLEDsUsingPalette();
        //mirror();
        Serial.print("button state 6:");Serial.println(buttons->state(6));
    }
    else if (buttons->state(7))
    {
        currentPalette =  CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

        // generate noise data
        fillnoise8();
        
        // convert the noise data to colors in the LED array
        // using the current palette
        mapNoiseToLEDsUsingPalette();
        mirror();
        Serial.print("button state 7:");Serial.println(buttons->state(7));

    }
    else if (buttons->state(8))
    {
        EVERY_N_MILLISECONDS(50) 
        {   // FastLED based non-blocking delay to update/display the sequence.
            plasma();
        }

        EVERY_N_MILLISECONDS(100) 
        {
            uint8_t maxChanges = 24; 
            nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
        }

        EVERY_N_SECONDS(5) 
        {   // Change the target palette to a random one every 5 seconds.
            uint8_t baseC = random8();                         // You can use this as a baseline colour if you want similar hues in the next line.
            targetPalette = CRGBPalette16(CHSV(baseC+random8(32), 192, random8(128,255)), CHSV(baseC+random8(32), 255, random8(128,255)), CHSV(baseC+random8(32), 192, random8(128,255)), CHSV(baseC+random8(32), 255, random8(128,255)));
        }
    } 
    else if (buttons->state(9))
    {
        blendwave();
    } 
    else if (buttons->state(10))
    {
        const int thisdelay = 60;                                          // Standard delay value.

        EVERY_N_MILLISECONDS(100) 
        {
            uint8_t maxChanges = 24; 
            nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
        }

        EVERY_N_SECONDS(3) 
        {
            targetPalette = CRGBPalette16(CHSV(random8(), 255, 32), CHSV(random8(), random8(64)+192, 255), CHSV(random8(), 255, 32), CHSV(random8(), 255, 255)); 
        }

        EVERY_N_MILLISECONDS(thisdelay) 
        {                                   // FastLED based non-blocking delay to update/display the sequence.
            ripple();
        }
    } 
    else if (buttons->state(11))
    {
        police_lightsALL();
    } 
    else if (buttons->state(12))
    {
        fourthOfJuly();
    } 
    else if (buttons->state(14))
    {
        EVERY_N_MILLISECONDS(50)
        {
            uint32_t ms = millis();
            //int32_t yHueDelta32 = ((int32_t)cos16( ms * 27 ) * (350 / kMatrixWidth));
            //int32_t xHueDelta32 = ((int32_t)cos16( ms * 39 ) * (310 / kMatrixHeight));
            int32_t yHueDelta32 = ((int32_t)cos16( ms * 27 ) * (400 / kMatrixWidth));
            int32_t xHueDelta32 = ((int32_t)cos16( ms * 39 ) * (200 / kMatrixHeight));
            DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
        }
    }
    else
    {
      Serial.println("other button state");
      //rotatingRainbow();
      //mirror();

        simpleColors();
//        explosion();
    }
    
    //send the 'leds' array out to the actual LED strip
    pulseJets();

    debugColors(); // temporarily let me see which hoop is which
    showLeds();  

    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 

    // do some periodic updates
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rotatingRainbow();
  addGlitter(80);
}

void debugColors()
{

  // numStrips refers to the number of physical strips 
  // each strip is one pin on the teensy
  for(int currentStrip = 0;currentStrip< numStrips;currentStrip++)
  {

    // purple for every 10 lights
    for(int currentLed = 1;currentLed<longestTube;currentLed++)
    {
      if (currentLed > tubeLength[currentStrip] -1)
        break;
      if (currentLed % 10 == 0)
            leds[currentStrip][currentLed] = CRGB::Purple;
    }

    // make the middle one white
    leds[currentStrip][tubeLength[currentStrip]/2] = CRGB::White;

    // make the last one blue
    leds[currentStrip][tubeLength[currentStrip]-1] = CRGB::Blue;

    //set the first n lights green where n = the index of the current strip
    for (int currentLed = 0; currentLed < currentStrip+1; currentLed++)    
      leds[currentStrip][currentLed] = CRGB::Green;
  }
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).

void Fire2012WithPalette()
{
    fillSolid(CRGB::Black);
    int height = longestTube / 2;

    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 55, suggested range 20-100 

    byte COOLING = 100;


    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    const byte SPARKING = 200;

    // Array of temperature readings at each simulation cell
    static byte heat[longestTube];


  // Step 1.  Cool down every cell a little
    for (int i = 0; i < (height); i++) 
    {
      heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / (height) + 2)));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for(int k = (height) - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < (height); j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);

      for(byte iStrip = 0;iStrip<numTubes;iStrip++)
          leds[iStrip][j] = ColorFromPalette( currentPalette, colorindex);
    }
    
    mirror();
}

// mirrors the array of leds (nice for the fire since I want it to be symmetrical
void mirror()
{
  for(byte tube = 0;tube < numTubes;tube++)
  {
    for(int row = 0;row < tubeLength[tube]/2;row++)
      leds[tube][tubeLength[tube] - row -1] = leds[tube][row];  
  }
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fillnoise8() {
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed".
  uint8_t dataSmoothing = 0;
  if( speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  
  for(int i = 0; i < numTubes; i++) {
    int ioffset = scale * i;
    for(int j = 0; j < longestTube; j++) {
      int joffset = scale * j;
      
      uint8_t data = inoise8(noiseX + ioffset,noiseY + joffset,noiseZ);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data,16);
      data = qadd8(data,scale8(data,39));

      if( dataSmoothing ) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
        data = newdata;
      }
      
      noise[i][j] = data;
    }
  }
  
  noiseZ += speed;
  
  // apply slow drift to X and Y, just for visual variation.
  noiseX += speed / 8;
  noiseY -= speed / 16;
}

void mapNoiseToLEDsUsingPalette()
{
  static uint8_t ihue=0;
  
  for(int i = 0; i < numTubes; i++) {
    for(int j = 0; j < longestTube; j++) {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[i][j];
      uint8_t bri =   noise[numTubes - i - 1][longestTube - j - 1];

      // if this palette is a 'loop', add a slowly-changing base value
      if( colorLoop) { 
        index += ihue;
      }

      // brighten up, as the color palette itself often contains the 
      // light/dark dynamic range desired
      if( bri > 127 ) {
        bri = 255;
      } else {
        bri = dim8_raw( bri * 2);
      }

      CRGB color = ColorFromPalette( currentPalette, index, bri);

      leds[i][j] = color;
    }
  }
  
  ihue+=1;
}
void ChangePaletteAndSettingsPeriodically()
{
    static byte iPalette = 0;
    iPalette++;
    if (iPalette > 8)
        iPalette = 0;

    switch(iPalette)
    {
        case 0: 
            currentPalette = RainbowColors_p;         
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 1:
            SetupPurpleAndGreenPalette();             
            speed = 10; 
            scale = 50; 
            colorLoop = 1;
            break;
        case 2:
            SetupBlackAndWhiteStripedPalette();       
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 3: 
            currentPalette = ForestColors_p;         
            speed = 8; 
            scale = 120; 
            colorLoop = 0;
            break;
        case 4: 
            currentPalette = CloudColors_p;         
            speed = 4; 
            scale = 30; 
            colorLoop = 0;
            break;
        case 5: 
            currentPalette = LavaColors_p;         
            speed = 8; 
            scale = 50; 
            colorLoop = 0;
            break;
        case 6: 
            currentPalette = OceanColors_p;         
            speed = 20; 
            scale = 90; 
            colorLoop = 0;
            break;
        case 7: 
            currentPalette = PartyColors_p;         
            speed = 20; 
            scale = 30; 
            colorLoop = 1;
            break;
        case 8: 
            currentPalette = RainbowStripeColors_p;         
            speed = 20; 
            scale = 20; 
            colorLoop = 1;
            break;

    }
}

CRGB getFlameColor()
{
    //tail lights are GRB, other lights arte RGB
    byte offset = 48;

  //  Serial.println(offset);
  byte seed = random(0+offset,50+offset);
  if (seed <30+offset)
    ;
  else if (seed < 45+offset)
    seed = seed - 30+offset;
  else 
    seed = 160 + seed + offset;

    return CHSV(seed, 255, 255);
}

CRGB getFlameColorFromPalette()
{

    const CRGBPalette16 flames =
    {
        0xFF00FF,
        0xFF00FF,
        0xFF00FF,
        0xFF00FF,

        0xFF0088,
        0xFF0088,
        0xFF0088,
        0xFF0088,

        0x0000AA,
        0x0000AA,
        0x0000AA,
        0x0000AA,

        0x880000,
        0x880000,
        0x880000,
        0x880000
    };

    const CRGBPalette16 reverseFlames =
    {
        0x00FF00,
        0x00FF00,
        0x00FF00,
        0x00FF00,

        0x88FF00,
        0x88FF00,
        0x88FF00,
        0x88FF00,

        0xAAFF00,
        0xAAFF00,
        0xAAFF00,
        0xAAFF00,

        0x008800,
        0x008800,
        0x008800,
        0x008800
    };

    const CRGBPalette16 coldFlames =
    {
        0x00FF,
        0x00FF,
        0x00FF,
        0x00FF,

        0x8800FF,
        0x8800FF,
        0x8800FF,
        0x8800FF,

        0xAA00FF,
        0xAA00FF,
        0xAA00FF,
        0xAA00FF,

        0x000088,
        0x000088,
        0x000088,
        0x000088
    };


    if (!buttons->isLifeTestOn())
        return ColorFromPalette(coldFlames, random8(), 128, LINEARBLEND);
    else
        return ColorFromPalette(reverseFlames, random8(), 128, LINEARBLEND);

}


void pulseJets()
{
        
  for (int iLed = 0;iLed<50;iLed++)
  {
    CRGB color = ColorFromPalette(LavaColors_p, random8(), 128, LINEARBLEND);
    // swap red and green because the tail lights are a different strip that swap r&g.
    byte r = color.r;
    color.r = color.g;
    color.g = r;
    leds[tailLights][iLed] = color;
    leds[tailLights][99-iLed] = color;
  }

}

// GETS CALLED BY SERIALCOMMAND WHEN NO MATCHING COMMAND
//void unrecognized(const char *command) {
//    Serial.println("nothin fo ya...");
//}
