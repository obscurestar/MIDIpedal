//Uses 'Arduino Micro' board designation.


// 7 digital 1 analog  encoder = 3 pot a1 3 buttons 1 LED strip
//2 14 15 16

//A0 pot
//A1 A2 pedals  (A1 LEFT 130-310 safe range) (A2 RIGHT 140-320)
//D2 WS2813
//D9-111 buttons
//D14-16 rotary encoder


const short PIN_LEDS = 10;   //Pin WS2813 strip attached to.
const short NUM_LEDS = 2;   //Length of WS2813 strip.
const short NUM_BUTTONS = 3;
const short NUM_POTS = 4;

#include "MIDIUSB.h"
byte MIDI_CHANNEL = 1;
byte LOWEST_NOTE = 36; //Lowest note to be used; 36 = C2; 60 = Middle C
byte POT_CC_START = 1;     // First MIDI CC to use.
byte POT_REVERSE_CC_START = 11;  //Output inverse  
byte POT_PULSE_CC_START = 21;
byte POT_REVERSE_PULSE_CC_START = 31;
byte BUTTON_CC_START = 80; 

//MIDI controllers seem to have 
const short gBanks = 8;
const short gPatches = 4;
short gBank = 0;
short gPatch = 0;
short gPatchBank = 0;
byte gEncoding = 0;  //bank if 0, patch if 1

#include <Adafruit_NeoPixel.h>    //Thanks Adafruit!
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "color.h"
#include "midi_pot.h"
#include "fastbutton.h"
#include "encoder.h"

MidiPot gPots[NUM_POTS];
Button gButtons[NUM_BUTTONS];

Encoder gEncoder(7,8);
Button gEncoderButton(6, true); //Encoder button is special.

Adafruit_NeoPixel H_LEDS = Adafruit_NeoPixel(NUM_LEDS, PIN_LEDS, NEO_BGR + NEO_KHZ800);

const byte MAX_BRITE=64;  //The brightest we want our display to get. 
unsigned long gNextLEDUpdate=0;
unsigned long gNextEncoderRead=0;
unsigned long gEncoderFlashTimer=0;
boolean gEncoderFlashHigh=true;
float gEncoded=0.0f;

COLOR gColor;
COLOR gColors[8];

void set_color(int color, byte red, byte green, byte blue)
{
  gColors[color].c[0] = red;
  gColors[color].c[1] = green;
  gColors[color].c[2] = blue;
}

void setup_colors(){
  gColor.l = 0;
  set_color(0,         0,         0,         0 );   //Black
  set_color(1, MAX_BRITE,         0,         0 );   //Red
  set_color(2, MAX_BRITE, MAX_BRITE,         0 );   //Yellow
  set_color(3,         0, MAX_BRITE,         0 );   //Green
  set_color(4,         0, MAX_BRITE, MAX_BRITE );   //Cyan
  set_color(5,         0,         0, MAX_BRITE );   //Blue
  set_color(6, MAX_BRITE,         0, MAX_BRITE );   //Magenta
  set_color(7, MAX_BRITE, MAX_BRITE, MAX_BRITE );   //White
}

void setup() {
  setup_colors();
  gEncoderFlashTimer=millis();

  H_LEDS.setBrightness(MAX_BRITE);  //TODO experiment with this feature.
  H_LEDS.begin(); //Initialize communication with WS281* chain.
  H_LEDS.show(); //No values are set so this should be black.

  gPots[0].init(A2, 0.05, 190, 300, 5, 100); //Left pedal 180 310 
  gPots[1].init(A3, 0.05, 170,310, 5, 100); //Right pedal 160 320
  gPots[2].init(A1, 0.1, 0, 1024, 8, 100); //Pot
  gPots[3].init(A0, 0.1, 0, 1024, 8, 100); //Pot

  //Buttons left to right and encoder button. (PIN, pullup ON)
  gButtons[0].init( 3, true);
  gButtons[1].init( 4, true);
  gButtons[2].init( 5, true);
}

void handleEncoder()
{
  int b = gEncoderButton.getStatus();

  //On button click, toggle between bank and patch selection.
  if ( b == Button::CLICK )
  {
    gEncoding = !gEncoding;
  }
  else
  {
     //Only process rotary if the button wasn't statused.
    int encoding = gEncoder.getStatus();
    gEncoded += (long) encoding;
    
    if (millis() > gNextEncoderRead)
    {
      gNextEncoderRead = millis() + 500;
      encoding = 0;
      if (gEncoded > 0.0f)
      {
        encoding = 1;
      } 
      else if (gEncoded < 0.0f)
      {
        encoding = -1;
      }
      gEncoded = 0.0f;

      if (gEncoding)
      {
        gPatch += encoding;
        if (gPatch < 0)
        {
          gPatch = gPatches-1;
        }
        if (gPatch >= gPatches)
        {
          gPatch = 0;
        }
      }
      else
      {
        gBank += encoding;
        if (gBank < 0)
        {
          gBank = gBanks -1;
        }
        if (gBank >= gBanks)
        {
          gBank = 0;
        }
      }
      int patchbank = gBank*gPatches + gPatch;
      if ( patchbank != gPatchBank )
      {
        programChange(MIDI_CHANNEL, 
                      patchbank,
                      0);
        MidiUSB.flush();
      }
      gPatchBank = patchbank;
    }
  }
}
void handleButtons()
{
  //for (int i=0;i<NUM_BUTTONS;++i)
  int newstate;
  int state;
  int i;
  for (i=0;i<NUM_BUTTONS;++i)
  {
    newstate = -1;
    state = gButtons[i].getStatus();
    
    if (state != Button::UNDEF )
    {
      controlChange(MIDI_CHANNEL, 
                    BUTTON_CC_START + i*3 + (state - 1),
                    0);
      MidiUSB.flush();
    }
  }
}

void doPulsedPots( int pot )
{
  //Broke out from inside handlePots.
  if ( gPots[ pot ].getPulse() )
  {
      //Send the pulse.
      controlChange(MIDI_CHANNEL, 
                    POT_PULSE_CC_START + pot,
                    (byte) gPots [ pot ].getPulseOn() * 127);
      MidiUSB.flush();

      //Send the inverse pulse.
      controlChange(MIDI_CHANNEL, 
                    POT_REVERSE_PULSE_CC_START + pot,
                    (byte) (!gPots [ pot ].getPulseOn()) * 127);
                    
      MidiUSB.flush();


                    
      MidiUSB.flush();    

  }
}

void handlePots() {
  for (int i=0;i<NUM_POTS;++i)
  {
    if ( gPots[i].getStatus() )
    {
      if ( gPots[i].mPrevious != gPots[i].mCurrent)
      {
        
        //This is the 'normal' bank.  Send the value.
        controlChange(MIDI_CHANNEL, 
                      POT_CC_START + i,
                      gPots[i].mCurrent);
        
        //It appears must flush between control changes.

        MidiUSB.flush();

        //The reversed set  (127 - current value)
        controlChange( MIDI_CHANNEL, 
                       POT_REVERSE_CC_START + i,
                       (127 - gPots[i].mCurrent) );

        MidiUSB.flush();
        
        gPots[i].mPrevious = gPots[i].mCurrent;
        
        //TODO something less crude for LEDs.
        if (i < 3)
        {
          //But I want these colors!
          int j=1;
          if (i==0)
          {
            j = 2;
          }
          else if (i==2)
          {
            j = 0;
          }
          gColor.c[j] = (unsigned char)map(gPots[i].mCurrent, 0, 127, 0, MAX_BRITE);
  

        }
      }
    }
    doPulsedPots( i );  //Broke these out clarity.
  }
}

// Arduino MIDI functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void programChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0C, 0xC0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void handleLED()
{
   //Don't update LEDs that often.
   if (gNextLEDUpdate < millis())
   {
      H_LEDS.setPixelColor(0, gColor.l);
      H_LEDS.setPixelColor(1, gColor.l);
      
      //To identify the bank and patch use a color flash.
      if (millis() - gEncoderFlashTimer > 1000)
      {
        gEncoderFlashHigh=!gEncoderFlashHigh;
        gEncoderFlashTimer = millis();
      }
      if (gEncoderFlashHigh)
      {  //Flashing, override colors.
        COLOR c;
        if (gEncoding)
        { //Patch mode, 4 choices
          c.l = gColors[ gPatch*2 + 1 ].l;
          H_LEDS.setPixelColor(1, c.l);
        }
        else
        {  //Bank mode, 8 choices
          c.l = gColors[ gBank ].l;
          H_LEDS.setPixelColor(0, c.l);
        }
      }
      H_LEDS.show();
      gNextLEDUpdate = millis() + 200; //Update 5 times a second
   }
}

void loop() {
   //Todo timing conditions to reduce lag.
   handleEncoder();
   handlePots();
   handleButtons();
   handleLED();
}
