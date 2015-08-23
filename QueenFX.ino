#define  FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include "Mux.h"

const byte realLedCount = 192;
const byte ledCount = 139;
const byte realNumStrips = 12;
const byte numStrips = 14;
const byte UPDATES_PER_SECOND =200;

// the front of the car has shorter lights
const byte stripLengths[14] = {36, 70, 84, 102, 115, 126, 139, 139, 139, 139, 139, 139, 139, 139} ;

CMux mux;

// globals for FX animations
int BOTTOM_INDEX = 0;
int TOP_INDEX = int(ledCount/2);
int FIRST_THIRD = int(ledCount/3);
int SECOND_THIRD = FIRST_THIRD * 2;
int EVENODD = ledCount%2;

CRGB realLeds[numStrips][realLedCount];
CRGB leds[numStrips][ledCount];

CRGB ledsX[ledCount]; //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, ETC)
const byte ledCounts[] = { };

int ledMode = 23;      //-START IN DEMO MODE
//int ledMode = 5;

//-PERISTENT VARS
byte idex = 0;        //-LED INDEX (0 to ledCount-1
byte ihue = 0;        //-HUE (0-360)
int ibright = 0;     //-BRIGHTNESS (0-255)
int isat = 0;        //-SATURATION (0-255)
bool bounceForward = true;  //-SWITCH FOR COLOR BOUNCE (0-1)
int bouncedirection = 0;
float tcount = 0.0;      //-INC VAR FOR SIN LOOPS
int lcount = 0;      //-ANOTHER COUNTING VAR
//byte eq[7] ={};


// color palette related stuff
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


//------------------SETUP------------------
void setup()
{
  delay(1000);
            
  LEDS.addLeds<WS2811_PORTDC,16>(*realLeds, realLedCount);
  LEDS.setBrightness(255);
        
  currentPalette = HeatColors_p;
  currentBlending = LINEARBLEND;

  Serial.begin(57600);
  Serial.println(F("https://github.com/zekekoch/QueenFX"));
  fillSolid(0,0,0); //-BLANK STRIP
    
    showLeds();
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
// the front of the car has shorter lights
//const byte stripLengths[14] = {36, 70, 84, 102, 115, 126, 139, 139, 139, 139, 139, 139, 139, 139} ;

void showLeds()
{
    // there are 13 phyical strips, but the first three are all on one logical strip
    for (byte iStrip = 0;iStrip < numStrips;iStrip++)
    {
        switch(iStrip)
        {
            case 0:
                for(byte iLed = 0;iLed < stripLengths[iStrip];iLed++)
                {
                    realLeds[0][iLed+155] = leds[iStrip][mapLed(iLed, ledCount, stripLengths[iStrip])];
                }                
                break;
            case 1: 
                // this one is backwards
                for(byte iLed = 0;iLed < stripLengths[iStrip];iLed++)
                {
                    realLeds[0][154-iLed] = leds[iStrip][mapLed(iLed, ledCount, stripLengths[iStrip])];
                }                
                break;
            case 2:
                for(byte iLed = 0;iLed < 84;iLed++)
                {
                    realLeds[0][iLed] = leds[iStrip][mapLed(iLed, ledCount, stripLengths[iStrip])];
                }                
                break;
            default:
                for(byte iLed = 0;iLed < ledCount;iLed++)
                {
                    realLeds[iStrip - 2][iLed] = leds[iStrip][mapLed(iLed, ledCount, stripLengths[iStrip])];
                }
            break;
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
    
    for(int iStrip = 0;iStrip < numStrips;iStrip++)
    {
        for( int iLed = 0; iLed < ledCount;iLed++) 
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

//-SET THE COLOR OF A SINGLE RGB LED
void setPixel(int adex, int cred, int cgrn, int cblu) {
    if (adex < 0 || adex > ledCount-1)
        return;

    for(int i = 0;i<numStrips;i++)
    {
        leds[i][adex] = CRGB(cred, cgrn, cblu);
    }
}

void setPixel(int adex, CRGB c) {
    if (adex < 0 || adex > ledCount-1)
        return;

    for(int i = 0;i<numStrips;i++)
    {
        leds[i][adex] = c;
    }
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
    return ledCount - i;
}


//-FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
    //int N2 = int(ledCount/2);
    int iN = i + TOP_INDEX;
    if (i >= TOP_INDEX) {
        iN = ( i + TOP_INDEX ) % ledCount; 
    }
    return iN;
}

int nextThird(int i) {
    int iN = i + (int)(ledCount / 3);
    if (iN >= ledCount) {
        iN = iN % ledCount;
    }
    return iN;
}

int adjacent_cw(int i) {
    int r;  
    if (i < 0) {
        r = 0;
    }
    else if (i >= ledCount - 1) {
        r = ledCount;
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
    else if (i > ledCount) {
        r = ledCount - 1;
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
    for(int i = 0; i < ledCount; i++ ) {
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
    leds[random8(14)][ random16(ledCount) ] += CRGB::White;
  }
}


void fillSolid(byte strand, const CRGB& color)
{
    // fill_solid -   fill a range of LEDs with a solid color
 fill_solid( leds[strand], ledCount, color);
}

void fillSolid(CRGB color)
{
    for(int i = 0;i<numStrips;i++)
        fillSolid(i, color);
}

void fillSolid(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    fillSolid(0, CRGB(cred, cgrn, cblu));    
}

void rotatingRainbow()
{
    static byte hue = 0;
    for(int i = 0;i < numStrips;i++)
    {
        fill_rainbow(leds[i], ledCount, hue++, 10);
        if(mux.isIntensifierOn())  
            addGlitter(255);
    }
}

void spiralRainbow()
{
    static byte hue = 0;
    static CHSV hsv = CHSV(0,255,255);
    static unsigned long lastTime = millis();
    static byte iStrip = 0;

    if (millis() < lastTime + 1)
    {
        return;
    }

    iStrip++;
    if (iStrip == numStrips)
        iStrip = 0;

    Serial.println(iStrip);

    hsv.hue += 1;

    for(int i = 0; i < numStrips;i++)
    {
        fadeToBlackBy(leds[i], ledCount, 30);
    }

    for( int iLed = 0; iLed < ledCount; iLed++) 
    {
        leds[iStrip][iLed] = hsv;
    }



    lastTime = millis();
}


void juggle() 
{
  // eight colored dots, weaving in and out of sync with each other
    for(byte iStrip =0;iStrip < numStrips;iStrip++)
    {
        fadeToBlackBy( leds[iStrip], ledCount/2, 20);
    }
    
    byte dothue = 0;
    for( int i = 0; i < 16; i++) {
        leds[beatsin16(i*2,0,numStrips)][beatsin16(i*2,0,ledCount)] |= CHSV(dothue, 200, 255);
    dothue += 32;
    }
    mirror();

}



void sinelon()
{
    static byte gHue;
  // a colored dot sweeping back and forth, with fading trails
    for(byte i = 0;i<numStrips;i++)
    {

        fadeToBlackBy( leds[i], ledCount, 20);
        int ypos = beatsin16(13,0,ledCount);
        leds[i][ypos] += CHSV( gHue, 255, 192);
    }
}

void rainbow_fade() { //-FADE ALL LEDS THROUGH HSV RAINBOW
    ihue++;
    CRGB thisColor;
    HSVtoRGB(ihue, 255, 255, thisColor);
    for(int idex = 0 ; idex < ledCount; idex++ ) {
        setPixel(idex,thisColor);
    }
}


void rainbow_loop(int istep, int idelay) { //-LOOP HSV RAINBOW
    static byte idex = 0;
    static byte offSet = 0;
    idex++;
    ihue = ihue + istep;
    
    if (idex >= ledCount) {
        idex = ++offSet;
    }
    
    CRGB icolor;
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void random_burst(int idelay) { //-RANDOM INDEX/COLOR
    CRGB icolor;
    
    idex = random(0,ledCount);
    ihue = random(0,255);
    
    HSVtoRGB(ihue, 255, 255, icolor);
    setPixel(idex, icolor);
}


void color_bounce(int idelay) { //-BOUNCE COLOR (SINGLE LED)
    if (bounceForward)
    {
        idex = idex + 1;
        if (idex == ledCount)
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

    for(int i = 0; i < ledCount; i++ ) {
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
    if (idex >= ledCount) {
        idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    for(int i = 0; i < ledCount; i++ ) {
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
    if (idex >= ledCount) {idex = 0;}
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    setPixel(idexR, 255, 0, 0);
    setPixel(idexB, 0, 0, 255);
}

void fourthOfJuly() { //-red, white and blue
    idex++;
    if (idex >= ledCount) {idex = 0;}
    int idexR = idex;
    int idexW = nextThird(idexR);
    int idexB = nextThird(idexW);
    setPixel(idexR, CRGB::Red);
    setPixel(idexW, CRGB::White);
    setPixel(idexB, CRGB::Blue);
}

void yxyy() { //-red, white and blue
    idex++;
    if (idex >= ledCount) 
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

        if (idex < ledCount) {
            idex++;
        } else if (idex == ledCount) {
            bounceForward = !bounceForward;
        }

        for(int i = 0;i<trailLength;i++)
        {
            //Serial.print(" to ");Serial.println(adjacent_cw(idex+i));
            setPixel(adjacent_cw(idex-i), HSVtoRGB(hue, 255, 255 - trailDecay*i));
        }
    } else {
        if (idex > ledCount) {
            idex = ledCount;
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
        if (idex == ledCount) {
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
    
    for(int i = 0; i < ledCount; i++ ) {
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
        
        for(int i = 0 ; i < ledCount; i++ ) {
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
    
    for(int i = 0 ; i < ledCount; i++ ) {
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
    
    for(int i = 0 ; i < ledCount; i++ ) {
        setPixel(i, acolor);
    }
}

void random_march(int idelay) { //RANDOM MARCH CCW
    copy_led_array();
    int iCCW;
    
    CRGB acolor;
    HSVtoRGB(random(0,360), 255, 255, acolor);
    setPixel(0, acolor);
    
    for(int i = 1; i < ledCount;i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
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
    
    for(int i = 1; i < ledCount; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
}


void white_temps() {
    int N9 = int(ledCount/9);
    for (int i = 0; i < ledCount; i++ ) {
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
        if (i >= N9*8 && i < ledCount)
                {setPixel(i, 64,156,255);} //-CLEAR BLUE SKY - 20000
    }
}


void color_loop_vardelay() { //-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
    idex++;
    if (idex > ledCount) {idex = 0;}
    
    CRGB acolor;
    HSVtoRGB(0, 255, 255, acolor);
    
    //int di = abs(TOP_INDEX - idex); //-DISTANCE TO CENTER
    //int t = constrain((10/di)*10, 10, 500); //-DELAY INCREASE AS INDEX APPROACHES CENTER (WITHIN LIMITS)
    
    for(int i = 0; i < ledCount; i++ ) {
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
    for(int i = 0; i < ledCount; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        setPixel(i, ledsX[iCCW]);
    }
}


void strip_march_ccw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCW;
    for(int i = 0; i < ledCount; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
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
    
    for(int i = 0; i < ledCount; i++ ) {
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
    
    for(int x = 0; x < ledCount; x++ ) {
        if (x <= TOP_INDEX) {ax = x;}
        else if (x > TOP_INDEX) {ax = ledCount-x;}
        
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
    //int N2 = int(ledCount/2);
    int N3 = int(ledCount/3);
    int N6 = int(ledCount/6);
    int N12 = int(ledCount/12);
    CRGB acolor;
    
    for(int i = 0; i < N6; i++ ) { //-HACKY, I KNOW...
        tcount = tcount + .02;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        int j0 = (i + ledCount - N12) % ledCount;
        int j1 = (j0+N3) % ledCount;
        int j2 = (j1+N3) % ledCount;
        HSVtoRGB(ahue, 255, ibright, acolor);
        setPixel(j0,acolor);
        setPixel(j1,acolor);
        setPixel(j2,acolor);
        
    }

}


void sin_bright_wave(int ahue, int idelay) {
    CRGB acolor;
    
    for(int i = 0; i < ledCount; i++ ) {
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
            return mux.getDynamicIntensifier();
        }
    }
    
    return false;
}

void onSelectorChange(byte dialState)
{
    switch(dialState)
    {
        case 0:
            //---STRIP RAINBOW FADE
            rainbow_fade();
            break;
        case 1:
            //---RAINBOW LOOP
            rainbow_loop(10, 20);         
        break;
        case 2:
            //--- VERITCAL RAINBOW
            rainbow_vertical(7, 20);
            break;
        case 3:
            rotatingRainbow();
            break;
        case 4:
            if (mux.getEmission())
                //---POLICE SINGLE
                police_lightsONE();
            else
                //---POLICE SOLID
                police_lightsALL();
        break;
        case 5:
        break;

    }

}

//------------------MAIN LOOP------------------
void loop() {
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());

  static int iDelay = 0;
  if (iDelay++ > 10)
  {    
    iDelay = 0;
    mux.getCutOff();
    mux.print();
  }

  switch(mux.getDial())
  {
    case CMux::dialCont:
      rotatingRainbow();
      break;
    case CMux::dialEmission:
      juggle();
      break;
    case CMux::dialLow:
      Fire2012WithPalette();
      break;
    case CMux::dialMedium:
      spiralRainbow();
      break;
    case CMux::dialHigh:
      sin_bright_wave(240, 35);
      break;
  }

  // test light order
  // Set the first n leds on each strip to show which strip it is
  //for(int i = 0; i < numStrips; i++) {
  //  for(int j = 0; j <= i+1; j++) {
  //    leds[i][j] = CRGB::Green;
  /// }
  //}
/*
  for(int i = 0;i< numStrips;i++)
  {
    // make the last one blue
    leds[i][ledCount-1] = CRGB::Blue;
    leds[i][ledCount/2] = CRGB::White;
    for(int l = 0;l<ledCount;l++)
    {
      if (l % 10 == 0)
            leds[i][l] = CRGB::Purple;
    }
  }

  for(byte s = 0;s < numStrips;s++)
    leds[s][0] = CRGB::Yellow;
*/


    //police_lightsALL();

    showLeds();
    FastLED.delay(1000/UPDATES_PER_SECOND);
    return;

    onSelectorChange(mux.getDial());

    if (mux.getCutOff() < 25)
        strip_march_cw(100);
    else if (mux.getCutOff() > 1000)
        strip_march_ccw(100);     
    
    switch(ledMode)
    {
        case 0:
            //soundMachine(0xCC0066, mux.getCutOff());
            sin_bright_wave(240, 35);       //--- SIN WAVE BRIGHTNESS
            break;
        case 1:
            fillSolid(CRGB::White);
            break;
        default:
            break;
    }
    

    if (ledMode == 4) {random_burst(20);}                //---RANDOM
    if (ledMode == 5) {color_bounce(20);}                //---CYLON v1
    if (ledMode == 6) {color_bounceFADE(20);}            //---CYLON v2

    if (ledMode == 9) {flicker(200,255);}                //---STRIP FLICKER
    if (ledMode == 10) {pulse_one_color_all(0, 10);}     //--- PULSE COLOR BRIGHTNESS
    if (ledMode == 11) {pulse_one_color_all_rev(0, 10);} //--- PULSE COLOR SATURATION
    if (ledMode == 12) {fade_vertical(240, 60);}         //--- VERTICAL SOMETHING
    if (ledMode == 14) {random_march(30);}               //--- MARCH RANDOM COLORS
    if (ledMode == 15) {rwb_march(50);}                  //--- MARCH RWB COLORS
    if (ledMode == 16) {radiation(120, 60);}             //--- RADIATION SYMBOL (OR SOME APPROXIMATION)
    if (ledMode == 17) {color_loop_vardelay();}          //--- VARIABLE DELAY LOOP
    if (ledMode == 18) {white_temps();}                  //--- WHITE TEMPERATURES
    if (ledMode == 19) {sin_bright_wave(240, 35);}       //--- SIN WAVE BRIGHTNESS
    if (ledMode == 20) {pop_horizontal(300, 100);}       //--- POP LEFT/RIGHT
    if (ledMode == 21) {quad_bright_curve(240, 100);}    //--- QUADRATIC BRIGHTNESS CURVE  
    if (ledMode == 22) {flame();}                        //--- FLAME-ISH EFFECT
    //if (ledMode == 25) {musicReactiveFade(mux.getCutOff());}
    if (ledMode == 26) {fourthOfJuly();}
    if (ledMode == 28) {yxyy();}
    
    
    if (ledMode == 101) {fillSolid(255,0,0);}    //---101- STRIP SOLID RED
    if (ledMode == 102) {fillSolid(0,255,0);}    //---102- STRIP SOLID GREEN
    if (ledMode == 103) {fillSolid(0,0,255);}    //---103- STRIP SOLID BLUE
    if (ledMode == 104) {fillSolid(255,255,0);}  //---104- STRIP SOLID YELLOW
    if (ledMode == 105) {fillSolid(0,255,255);}  //---105- STRIP SOLID TEAL?
    if (ledMode == 106) {fillSolid(255,0,255);}  //---106- STRIP SOLID VIOLET?
    
    showLeds();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
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
    byte height = ledCount / 2;

    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 55, suggested range 20-100 
    const byte COOLING = mux.getCutOff();


    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    const byte SPARKING = 120;

    // Array of temperature readings at each simulation cell
    static byte heat[ledCount];


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

      for(byte iStrip = 0;iStrip<numStrips;iStrip++)
          leds[iStrip][j] = ColorFromPalette( currentPalette, colorindex);
    }
    
    mirror();
}

// mirrors the array of leds (nice for the fire since I want it to be symmetrical
void mirror()
{
  for(byte col = 0;col < numStrips;col++)
  {
    for(byte row = 0;row < ledCount/2;row++)
      leds[col][ledCount - row -1] = leds[col][row];  
  }
}

/*
void soundMachine(CRGB color, byte eq[7]){
    CRGB baseColor = CRGB::LightPink;
    CRGB highlightColor = color;
    
    int bass = (eq[0] + eq[1]) / 2;
    int high = (eq[4] + eq[4]) /2;
    
    bass > 100 ? fillSolid(0, highlightColor) : fillSolid(0, baseColor);
    bass > 150 ? fillSolid(1, highlightColor) : fillSolid(1, baseColor);
    bass > 200 ? fillSolid(2, highlightColor) : fillSolid(2, baseColor);
    high > 150 ? fillSolid(3, highlightColor) : fillSolid(3, baseColor);
    high > 100 ? fillSolid(4, highlightColor) : fillSolid(4, baseColor);
    high > 200 ? fillSolid(5, highlightColor) : fillSolid(5, baseColor);
    high > 200 ? fillSolid(6, highlightColor) : fillSolid(6, baseColor);
}
*/


// GETS CALLED BY SERIALCOMMAND WHEN NO MATCHING COMMAND
void unrecognized(const char *command) {
    Serial.println("nothin fo ya...");
}

