#define  FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include "Mux.h"

#define NUM_LEDS_PER_STRIP 185
#define ledCount NUM_LEDS_PER_STRIP
#define NUM_STRIPS 16 

CMux mux;

int BOTTOM_INDEX = 0;
int TOP_INDEX = int(ledCount/2);
int FIRST_THIRD = int(ledCount/3);
int SECOND_THIRD = FIRST_THIRD * 2;
int EVENODD = ledCount%2;

CRGB leds[NUM_STRIPS][ledCount];
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

//------------------SETUP------------------
void setup()
{
    delay(1000);
            
  LEDS.addLeds<WS2811_PORTDC,16>(*leds, ledCount);
    // For safety (to prevent too high of a power draw), the test case defaults to
    // setting brightness to 25% brightness
  LEDS.setBrightness(255);
        
    Serial.begin(57600);
    fillSolid(0,0,0); //-BLANK STRIP
    
    LEDS.show();
}


//------------------------------------- UTILITY FXNS --------------------------------------

//-SET THE COLOR OF A SINGLE RGB LED
void setPixel(int adex, int cred, int cgrn, int cblu) {
    if (adex < 0 || adex > ledCount-1)
        return;

    for(int i = 0;i<NUM_STRIPS;i++)
    {
        leds[i][adex] = CRGB(cred, cgrn, cblu);
    }
}

void setPixel(int adex, CRGB c) {
    if (adex < 0 || adex > ledCount-1)
        return;

    for(int i = 0;i<NUM_STRIPS;i++)
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

void fillSolid(byte strand, const CRGB& color)
{
    // fill_solid -   fill a range of LEDs with a solid color
 fill_solid( leds[strand], ledCount, color);
}

void fillSolid(CRGB color)
{
    for(int i = 0;i<7;i++)
        fillSolid(i, color);
}

void fillSolid(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    fillSolid(0, CRGB(cred, cgrn, cblu));    
}

void rotatingRainbow()
{
    static byte hue = 0;
    for(int i = 0;i < 7;i++)
    {
        fill_rainbow(leds[i], ledCount, hue++, 10);
    }
}



void rainbow_fade(int idelay) { //-FADE ALL LEDS THROUGH HSV RAINBOW
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


void police_lightsONE(int idelay) { //-POLICE LIGHTS (TWO COLOR SINGLE LED)
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


void police_lightsALL(int idelay) { //-POLICE LIGHTS (TWO COLOR SOLID)
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
    setPixel(idexR, 255, 0, 0);
    setPixel(idexW, 255, 255, 255);
    setPixel(idexB, 0, 0, 255);
}

void yxyySeizure() {
  
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
    int random_delay = random(10,100);
    int random_bool = random(0,random_bright);
    CRGB thisColor;
    
    //if (random_bool < 10) {
        HSVtoRGB(thishue, thissat, random_bright, thisColor);
        
        for(int i = 0 ; i < ledCount; i++ ) {
            setPixel(i, thisColor);
        }
   // }
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


void random_red() { //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
    int temprand;
    for(int i = 0; i < ledCount; i++ ) {
        for(int strip = 0;strip < NUM_STRIPS;strip++)
        {
            temprand = random(0,100);
            if (temprand > 50) {leds[strip][i].r = 255;}
            if (temprand <= 50) {leds[strip][i].r = 0;}
            leds[strip][i].b = 0; leds[strip][i].g = 0;
        }
    }
}

// todo: make this work with multiple arrays
void rule30(int idelay) { //1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
    copy_led_array();
    int iCW;
    int iCCW;
    int y = 100;
    for(int i = 0; i < ledCount; i++ ) {
        iCW = adjacent_cw(i);
        iCCW = adjacent_ccw(i);
        if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {leds[0][i].r = 0;}
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
            setPixel(0, 255, 255, 255);
            break;
        case 2:
            setPixel(0, 0, 0, 255);
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
    
    int di = abs(TOP_INDEX - idex); //-DISTANCE TO CENTER
    int t = constrain((10/di)*10, 10, 500); //-DELAY INCREASE AS INDEX APPROACHES CENTER (WITHIN LIMITS)
    
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
    
    int ix;
    
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
    int idelay = random(0,35);
    
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


void pacman(int idelay) { //-MARCH STRIP C-W
    int s = int(ledCount/4);
    lcount++;
    if (lcount > 5) {lcount = 0;}
    if (lcount == 0) {
        fillSolid(255,255,0);
    }
    if (lcount == 1 || lcount == 5) {
        fillSolid(255,255,0);
        setPixel(s,CRGB(0,0,0));
    }
    if (lcount == 2 || lcount == 4) {
        fillSolid(255,255,0);
        setPixel(s-1,CRGB(0,0,0));
        setPixel(s,CRGB(0,0,0));
        setPixel(s+1,CRGB(0,0,0));
    }
    if (lcount == 3) {
        fillSolid(255,255,0);
        setPixel(s-2,CRGB(0,0,0));
        setPixel(s-1,CRGB(0,0,0));
        setPixel(s,CRGB(0,0,0));
        setPixel(s+1,CRGB(0,0,0));
        setPixel(s+2,CRGB(0,0,0));
    }
}

int getModeFromSerial() {
    // if there's any serial available, read it:
    while (Serial.available() > 0) {
        // look for the next valid integer in the incoming serial stream:
        int iMode = Serial.parseInt();
        Serial.print("switching to:");Serial.println(iMode);
        // look for the newline. That's the end of your
        // sentence:
        return iMode;
    }
}

boolean checkButton(byte whichButton)
{
    static unsigned long lastTime = millis();
    if (millis() > lastTime + 500)
    {
        if (0 == whichButton)
        {
            return mux.getEmission();
        }
    }
    else
    {
        return false;
    }
}

//------------------MAIN LOOP------------------
void loop() {

    //
    if (checkButton(0))
    {
      ledMode++;
      if (ledMode > 27)
        ledMode = 0;
    }

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
    
    if (ledMode == 2) {rainbow_fade(20);}                //---STRIP RAINBOW FADE
    if (ledMode == 3) {rainbow_loop(10, 20);}            //---RAINBOW LOOP
    if (ledMode == 4) {random_burst(20);}                //---RANDOM
    if (ledMode == 5) {color_bounce(20);}                //---CYLON v1
    if (ledMode == 6) {color_bounceFADE(20);}            //---CYLON v2
    if (ledMode == 7) {police_lightsONE(40);}            //---POLICE SINGLE
    if (ledMode == 8) {police_lightsALL(40);}            //---POLICE SOLID
    if (ledMode == 9) {flicker(200,255);}                //---STRIP FLICKER
    if (ledMode == 10) {pulse_one_color_all(0, 10);}     //--- PULSE COLOR BRIGHTNESS
    if (ledMode == 11) {pulse_one_color_all_rev(0, 10);} //--- PULSE COLOR SATURATION
    if (ledMode == 12) {fade_vertical(240, 60);}         //--- VERTICAL SOMETHING
    if (ledMode == 13) {rule30(100);}                    //--- CELL AUTO - RULE 30 (RED)
    if (ledMode == 14) {random_march(30);}               //--- MARCH RANDOM COLORS
    if (ledMode == 15) {rwb_march(50);}                  //--- MARCH RWB COLORS
    if (ledMode == 16) {radiation(120, 60);}             //--- RADIATION SYMBOL (OR SOME APPROXIMATION)
    if (ledMode == 17) {color_loop_vardelay();}          //--- VARIABLE DELAY LOOP
    if (ledMode == 18) {white_temps();}                  //--- WHITE TEMPERATURES
    if (ledMode == 19) {sin_bright_wave(240, 35);}       //--- SIN WAVE BRIGHTNESS
    if (ledMode == 20) {pop_horizontal(300, 100);}       //--- POP LEFT/RIGHT
    if (ledMode == 21) {quad_bright_curve(240, 100);}    //--- QUADRATIC BRIGHTNESS CURVE  
    if (ledMode == 22) {flame();}                        //--- FLAME-ISH EFFECT
    if (ledMode == 23) {rainbow_vertical(7, 20);}       //--- VERITCAL RAINBOW
    if (ledMode == 24) {pacman(100);}                     //--- PACMAN
    //if (ledMode == 25) {musicReactiveFade(mux.getCutOff());}
    if (ledMode == 26) {fourthOfJuly();}
    if (ledMode == 27) {rotatingRainbow();}
    if (ledMode == 28) {yxyy();}
    
    
    if (ledMode == 101) {fillSolid(255,0,0);}    //---101- STRIP SOLID RED
    if (ledMode == 102) {fillSolid(0,255,0);}    //---102- STRIP SOLID GREEN
    if (ledMode == 103) {fillSolid(0,0,255);}    //---103- STRIP SOLID BLUE
    if (ledMode == 104) {fillSolid(255,255,0);}  //---104- STRIP SOLID YELLOW
    if (ledMode == 105) {fillSolid(0,255,255);}  //---105- STRIP SOLID TEAL?
    if (ledMode == 106) {fillSolid(255,0,255);}  //---106- STRIP SOLID VIOLET?
    
    LEDS.show();
    delay(5);
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

