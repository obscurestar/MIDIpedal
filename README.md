ATTRIBUTES:
    Original MIDI CC and Note send pulled from code by Gustavo Silveira and Dolce Wang.
    NeoPixel by Adafruit
    ObscureStar for the rest of it.

OVERVIEW:
    A simple MIDI guitar controller which can send CC and PC commands.
    The sensors use exponential smoothing to reduce messaging and can be tuned further with both threshold and clamp values.
    Uses 2 WS2813 LEDs as a status display with the intensity of green and blue representing the forward values of the two pedals and a 1-second blink with the colors black, red, yellow, green, cyan, blue, magenta, and white on the first LED to identify bank 1-8 and black, red, green, blue to identify patch 1-4 in that bank. Which LED is flashing depends on whether patch or bank is presently being controlled by the rotary encoder.  Encoder values loop eg: 1 2 3 4 1 2 3 4 ..
    The rotary encoder selects the patch/bank.  Clicking the rotary encoder button switches between patch and bank selection.  

IMPORTANT NOTES:
. Button trigger events are sent on button release.
. IR Range sensors are treated as pots with a clamped range
. All clamping/tuning values in the initialization of MidiPots objects are calibrated to my hardware.  Yours may vary.

FUNCTIONS:

Each button can send 3 distinct CCs by either tapping the button or holding it for 1 or 3 seconds
eg:
    CC80 is sent when first button is tapped  (pressed for less than 1 second) 
    CC81 is sent when first button is held for 1-2.999 seconds
    CC82 is sent when first button is held for 3 seconds or more

    The pedals and potentiometers also have overloaded functionality, sending CCs for the value read,
    the inverted value and on/off messages with increasing speed/decreasing speed for a total of 4ccs per.
    This lets you do stuff like decrease output as you increase gain or use a volume pedal as a 
    pulsed kill switch. It messes up 'learn' mode though so the values are by 10s.

eg:  
    CC1 is a 0-127 value from the first pedal or pot.
    CC11 is a 127-0 value from the first pedal or pot.
    CC21 toggles between 0 and 127 once every 1sec - 1ms 
    CC31 toggles between 127 and 0 once every 1sec - 1ms 

    CC2 is a 0-127 value from the second pedal or pot.
    etc.

DEPENDENCIES:
   https://github.com/obscurestar/arduino_widgets 
