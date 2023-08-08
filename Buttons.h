#include <FastLED.h>
#include <Adafruit_TLC59711.h>
#include "Mux.h"

class CButtons
{
    private:

    // How many boards do you have chained??
    const byte NUM_TLC59711 = 4;
    const byte data = 4;
    const byte clk = 3;

    CRGB buttonLights[16];

    CMux mux;
    Adafruit_TLC59711 tlc = Adafruit_TLC59711(NUM_TLC59711, clk, data);

    // this took me way longer than I expected, but my buttons are in 
    // fairly random order, so this maps the actual button IDs from the
    // mux to the order the buttons are installed in the dashboard

    const int full = 60000;
    const int half = 30000;
    // const byte __buttonOrder[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
    const byte buttonOrder[16] = {13,12,3,2,14,15,0,1,4,5, 6, 7, 8, 9,10,11};
    const byte ledOrder[16] =    {11,8,13,14,10,9,12,15,0,1,2,3,4,5,6,7};

    bool buttonCache[16] = {false};
    byte lastButtonOn = 255;
    CRGB buttonUpColor[16] = {CRGB::Orange, CRGB::Blue, CRGB::Green, CRGB::Purple, CRGB::White, CRGB::Brown, CRGB::Aqua, CRGB::Violet,CRGB::Red, CRGB::Pink, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Indigo, CRGB::Violet};
    CRGB buttonColor[16] = {CRGB::Red, CRGB::Pink, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Indigo, CRGB::Violet,CRGB::Red, CRGB::Pink, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Indigo, CRGB::Violet};

    public:


    CButtons()
    {
    }

    bool isIntensifierOn()
    {
        return buttonCache[3];
    }

    bool isLifeTestOn()
    {
        return buttonCache[7];
    }

  void print()
  {
    Serial.println("og: ");
    for(byte i = 0;i<16;i++)
    {
        Serial.print(buttonCache[i]);
        if (0==(i+1)%4)
        {
            Serial.println();
        }    
    }
    Serial.println();


    Serial.println("bo: ");
    for(byte i = 0;i<16;i++)
    {
        Serial.print(buttonCache[buttonOrder[i]]);
        printGroup(i);
    }
    Serial.println();
  }

    void printGroup(byte i)
    {
        if (0==(i+1)%4)
        {
            Serial.println();
        }    
    }

    bool state (int index)
    {
        //print();
        //Serial.print("Buttons->state(");Serial.print(index);Serial.print(") ");Serial.print(buttonCache[index]);Serial.println();
        return buttonCache[index];
    }
    
    void setup()
    {
        pinMode(data, OUTPUT);
        pinMode(clk, OUTPUT);
        tlc.begin();
        tlc.write();
    }


    void clear()
    {
        mux.clear();
    }

    void chaseColorButtons()
    {
        for (byte led = 0;led<16;led++)
        {
            clear();
            if (led == 0)
                setButtonColor(led, 0, full, 0); 
            else
                setButtonColor(led, full, 0, 0); 
            
            FastLED.delay(100);  
        }
    }

    // periodically check the state of the buttons and determine if they
    // should be up or down, cache that and color them appropriately
    void refresh()
    {
        // I only want to refresh the buttons every 200 milliseconds
        static unsigned long lastTime = millis();
        if (millis() < lastTime + 50)
        {
            return;
        }
        lastTime = millis();

        //mux.printBare();

        // loop over all of the buttons and check their 
        for (byte i = 0;i<16;i++)
        {
            byte led = ledOrder[i];
            byte buttonIndex = buttonOrder[i];
            bool isButtonOn = mux.ButtonOn(buttonIndex);

            if (isButtonOn)
            {
                clear();
                mux.setButtonState(buttonOrder[i], false);


                CRGB c = buttonUpColor[i];
                //Serial.print("setButtonDownColor ");Serial.print(i);Serial.print(".");Serial.print(led);Serial.print(" ");Serial.print(c.r);Serial.println();
                setButtonColor(led, c);
                mux.setButtonState(buttonOrder[i], true);  
                buttonCache[i] = true;
            }
            else
            {
                CRGB c = buttonUpColor[i];
                c /= 4;
                //Serial.print("setButtonUpColor ");Serial.print(i);Serial.print(".");Serial.print(led);Serial.print(" ");Serial.print(c.r);Serial.println();
                setButtonColor(led, c); 
                buttonCache[i] = false;
            }
        }
        //Serial.println();
    }

    void setButtonColor(byte button, CRGB color)
    {
        setButtonColor(button, color.red, color.green, color.blue);
    }

    void setButtonColor(byte button, int r, int g, int b)
    { 
        // set the button cache
        CRGB c = CRGB(r, g, b);
        buttonColor[button] = c;

        // these are wired BRG
        byte brightness = 128;
        tlc.setLED(button, b*brightness, g*brightness, r*brightness);
        tlc.write();  
    }

    CRGB getButtonColor(byte button)
    {
        return buttonColor[button];
    }


    // Fill the dots one after the other with a color
    void colorWipe(uint16_t r, uint16_t g, uint16_t b, uint8_t wait) {
    for(uint16_t i=0; i<8*NUM_TLC59711; i++) {
        tlc.setLED(i, r, g, b);
        tlc.write();
        delay(wait);
    }
    }

    // Slightly different, this makes the rainbow equally distributed throughout
    void rainbowCycle(uint8_t wait) {
    uint32_t i, j;

    for(j=0; j<65535; j+=10) { // 1 cycle of all colors on wheel
        for(i=0; i < 4*NUM_TLC59711; i++) {
        Wheel(i, ((i * 65535 / (4*NUM_TLC59711)) + j) & 65535);
        }
        tlc.write();
        delay(wait);
    }
    }

    // Input a value 0 to 4095 to get a color value.
    // The colours are a transition r - g - b - back to r.
    void Wheel(uint8_t ledn, uint16_t WheelPos) {
    if(WheelPos < 21845) {
        tlc.setLED(ledn, 3*WheelPos, 65535 - 3*WheelPos, 0);
    } else if(WheelPos < 43690) {
        WheelPos -= 21845;
        tlc.setLED(ledn, 65535 - 3*WheelPos, 0, 3*WheelPos);
    } else {
        WheelPos -= 43690;
        tlc.setLED(ledn, 0, 3*WheelPos, 65535 - 3*WheelPos);
    }
    }

    //All RGB Channels on full colour
    //Cycles trough all brightness settings from 0 up to 127
    void increaseBrightness()
    {
        for(uint16_t i=0; i<8*NUM_TLC59711; i++) {
            tlc.setLED(i, 65535, 65535, 65535);
        }
        for(int i = 0; i < 128; i++){
            tlc.simpleSetBrightness(i);
            tlc.write();
            delay(20);
        }
    }

};