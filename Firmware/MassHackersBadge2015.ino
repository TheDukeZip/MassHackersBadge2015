#include <SoftPWM.h>
#include <SoftPWM_timer.h>
#include <EEPROM.h>

#include "MassHackersBadge.h"


/////////////////////////////////////////////////////
//
// DEFINITIONS
//
/////////////////////////////////////////////////////

#define LED_BRIGHTNESS 1                        //Turn down the brightness - 0.5 = 50%   0.25 = 25% etc.

#define NUMBER_OF_LEDS 6                        //Number of LEDs on the badge to use

#define EEPROM_COLOR_LOCATION 0                 //Location in EEPROM to save color setting
#define EEPROM_MODE_LOCATION  1                 //Location in EEPROM to save mode setting
#define MAIN_LOOP_SAVE_ITERATIONS 60            //How many iterations through the main loop to check if you've changed the settings, and save them

#define BUTTON_PIN 12                           //Which pin is connected to the button?

#define CELLULAR_AUTOMATON_WIDTH 66             //Size of the array to store the cellular automaton state
#define CELLULAR_AUTOMATON_REPEAT_THRESHOLD 3   //If the LEDs for cellular automaton stabilize for this many iterations, reset the array
#define CELLULAR_AUTOMATON_OFFSET 30            //Offset in cellular automaton array to display

#define POWER_CONSERVATION_ENABLE true          //Turns off unneeded peripherals on MCU
#define NUM_UNUSED_PINS 3                       //Define the unused pins we can set to output mode to save some power
uint8_t UnusedPins[NUM_UNUSED_PINS] = { 9, 10, 11 };

//Two-dimensional array specifying LED pins
// Each row represents an LED, array within represent pin for R, G, B in that order
// Do not change these unless you use the code with hardware other than the MassHackers 2015 badge
// See this page for pinouts: https://www.pjrc.com/teensy/pinout.html
uint8_t LEDS[6][3] = {
                      {2, 1, 0},  //D1 { R, G, B }
                      {5, 4, 3},  //D2 { R, G, B }
                      {8, 7, 6},  // etc.
                      {19, 20, 21},
                      {16, 17, 18},
                      {13, 14, 15}
};

//Map values of R, G, B, for each LED_COLOR defined in header file
uint8_t LEDColorMap[10][3] = {
                              {  0, 255,   0}, //LED_COLOR_GREEN
                              {  0,   0, 255}, //LED_COLOR_BLUE
                              {255,   0,   0}, //LED_COLOR_RED
                              {  0, 255, 255}, //LED_COLOR_CYAN
                              {255, 255,   0}, //LED_COLOR_YELLOW
                              {255,   0, 255}, //LED_COLOR_PURPLE
                              {255, 255, 255}, //LED_COLOR_WHITE
                              {255, 255, 255}, //LED_COLOR_SEQUENTIAL Just a placeholder
                              {255, 255, 255}  //LED_MODE_MULTI       Just a placeholder
};

//Map of fade in , fade out times for each LED_MODE defined in header file
uint16_t LEDModeFadeTimes[9][2] = {
                                {  50, 400 }, //LED_MODE_SCAN
                                {  50, 400 }, //LED_MODE_SCAN_CONSTANT
                                {2000,2000 }, //LED_MODE_PULSE
                                {  50, 200 }, //LED_MODE_HEARTBEAT
                                { 600, 400 }, //LED_MODE_CELLULAR_AUTOMATON
                                {   0, 500 }, //LED_MODE_TWINKLE
                                {   0, 100 }, //LED_MODE_BOOTS_AND_PANTS
                                {   0,  50 }, //LED_MODE_VU_METER
                                {   0, 500 }  //LED_MODE_PARTY_SHUFFLE
};


uint8_t cells[CELLULAR_AUTOMATON_WIDTH];          //Array to store the cellular automaton status

uint8_t ruleset[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };  //Cellular automaton ruleset - default is RULE 30 (Chaotic)
                                                  //See http://mathworld.wolfram.com/ElementaryCellularAutomaton.html for diagrams of all rules

uint8_t cellular_automaton_repeat_count = 0;      //Store the number of times the cellular automaton LEDs have repeated


/////////////////////////////////////////////////////
//
// SETUP AND MAIN LOOP
//
/////////////////////////////////////////////////////
LED_COLOR currentColor = LED_COLOR_GREEN;
LED_MODE currentMode = LED_MODE_SCAN;

LED_COLOR currentSequentialColor = LED_COLOR_GREEN;

int saveCounter = 0;
  
void setup() 
{
  //Start saving power
  if(POWER_CONSERVATION_ENABLE) startPowerSave();
   
  //Set up SoftPWM
  SoftPWMBegin(SOFTPWM_INVERTED); //LEDs have a common anode, pull LOW to turn them ON (inverted)
  for (int i = 0; i < NUMBER_OF_LEDS; i++)
  {
    for (int j = 0; j < 3; j++)
      SoftPWMSet(LEDS[i][j], 0);
  }
  SoftPWMSetFadeTime(ALL, 50, 400);

  //Set up button
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH); 

  //Restore settings
  restoreSettings();

  //Initialize cellular automaton array
  initializeCellularAutomaton();
}

void loop() 
{
  
  switch(currentMode) {
    case LED_MODE_SCAN:
      scan();
      break;

     case LED_MODE_SCAN_CONSTANT:
      scan_constant();
      break;

     case LED_MODE_PULSE:
      pulse();
      break;

     case LED_MODE_HEARTBEAT:
      heartbeat();
      break;
      
     case LED_MODE_CELLULAR_AUTOMATON:
      cellular_automaton();
      break;
      
     case LED_MODE_TWINKLE:
      twinkle();
      break;

     case LED_MODE_BOOTS_AND_PANTS:
      boots_and_pants();
      break;

     case LED_MODE_VU_METER:
      vu_meter();
      break;

     case LED_MODE_PARTY_SHUFFLE:
      party_shuffle();
      break;

     case LED_MODE_END: //Should never get here, but disables the warning in the compiler
      break;
  }

  //Save settings occasionally
  if(++saveCounter >= MAIN_LOOP_SAVE_ITERATIONS)
  {
    saveSettings();
    saveCounter = 0;
  }

  //If you're in sequential color mode, change the color on every iteration of the loop
  if(currentColor == LED_COLOR_SEQUENTIAL)
    changeSequentialColor();
}


/////////////////////////////////////////////////////
//
// LIGHTING MODE PROGRAMMING
//
//
// One handler function per mode
// Functions are called by the main loop() above
//
/////////////////////////////////////////////////////

//OCCASIONAL CYLON / KITT STYLE SCANNER
void scan() 
{
  int i;
  
  setLED(0, currentColor);

  delay(50);
  for (i = 0; i < NUMBER_OF_LEDS - 1; i++)
  {
    setLED(i+1, currentColor);
    offLED(i);
    delay(50);
    if(checkButton()) return;
  }

  for (int t = 0; t < 4; t++)
  {
    delay(50);
    if(checkButton()) return;
  }
  
  for (i = NUMBER_OF_LEDS - 1; i > 0; i--)
  {
    setLED(i-1, currentColor);
    offLED(i);
    delay(50);
    if(checkButton()) return;
  }

  offLED(0);
  
  for (int t = 0; t < 20; t++)
  {
    delay(50);
    if (checkButton()) return;
  }
}

//CONSTANT CYLON / KITT STYLE SCANNER
void scan_constant() 
{
  int i;
  
  setLED(0, currentColor);

  delay(50);
  for (i = 0; i < NUMBER_OF_LEDS - 1; i++)
  {
    setLED(i+1, currentColor);
    offLED(i);
    delay(50);
    if (checkButton()) return;
  }

  for (int t = 0; t < 4; t++)
  {
    delay(50);
    if (checkButton()) return;;
  }
  
  for (i = NUMBER_OF_LEDS - 1; i > 0; i--)
  {
    setLED(i-1, currentColor);
    offLED(i);
    delay(50);
    if(checkButton()) return;
  }

  for (int t = 0; t < 4; t++)
  {
    delay(50);
    if (checkButton()) return;
  }
}

//PULSE
void pulse()
{
  int i;
  
  for(i = 0; i < NUMBER_OF_LEDS; i++)
  {
    setLED(i, currentColor);
  }

  for (int t = 0; t < 16; t++)
  {
    delay(50);
    if (checkButton()) return;
  }
  
  for(i = 0; i < NUMBER_OF_LEDS; i++)
  {
    offLED(i);
  }

  for (int t = 0; t < 30; t++)
  {
    delay(50);
    if (checkButton()) return;
  }
}

//HEARTBEAT
void heartbeat()
{
  int i;
  
  for(i = 3; i < NUMBER_OF_LEDS; i++)
  {
    setLED(i, currentColor);
    setLED(5 - i, currentColor);
  }

  delay(50);
  if (checkButton()) return;  

  for(i = 0; i < NUMBER_OF_LEDS / 2; i++)
  {
    offLED(i);
    offLED(5 - i);
  }

  if (checkButton()) return; 
  
  for(i = 3; i < NUMBER_OF_LEDS; i++)
  {
    setLED(i, currentColor);
    setLED(5 - i, currentColor);
  }

  delay(50);
  if (checkButton()) return;  

  for(i = 0; i < NUMBER_OF_LEDS / 2; i++)
  {
    offLED(i);
    offLED(5 - i);
  }
  
  for (int t = 0; t < 30; t++)
  {
    delay(50);
    if (checkButton()) return;
  }
}

//CELLULAR AUTOMATON
void cellular_automaton() 
{
  int i;
  bool LEDs_repeated = true;
  uint8_t newcells[CELLULAR_AUTOMATON_WIDTH];  

  //Turning off the LEDs, then on, seems to be more visually appealing than doing it all in one loop
  for(i = 0; i < NUMBER_OF_LEDS; i++)
  {
    if(!cells[i + CELLULAR_AUTOMATON_OFFSET])
      offLED(i);
  }
  for(i = 0; i < NUMBER_OF_LEDS; i++)
  {
    if(cells[i + CELLULAR_AUTOMATON_OFFSET])
      setLED(i, currentColor);
  }
      
  for(i = 1; i < CELLULAR_AUTOMATON_WIDTH - 2; i++)
  {
    uint8_t left = cells[i-1];
    uint8_t middle = cells[i];
    uint8_t right = cells[i+1];

    newcells[i] = cellular_automaton_rules(left, middle, right);
  }

  //Far left cell
  newcells[0] = cellular_automaton_rules(0, cells[0], cells[1]);

  //Far right cell
  newcells[CELLULAR_AUTOMATON_WIDTH - 1] = cellular_automaton_rules(cells[CELLULAR_AUTOMATON_WIDTH - 2], cells[CELLULAR_AUTOMATON_WIDTH - 1], 0);

  //Check and see if the LEDs repeated
  for(i = 0; i < NUMBER_OF_LEDS; i++)
  {
    if(newcells[i + CELLULAR_AUTOMATON_OFFSET] != cells[i + CELLULAR_AUTOMATON_OFFSET])
      LEDs_repeated = false;
  }
  if(LEDs_repeated) 
    cellular_automaton_repeat_count++;

  memcpy(cells, newcells, CELLULAR_AUTOMATON_WIDTH * sizeof(char)); // I know you just did a double take. I put that there just to make you smile  ;)

  if (checkButton()) return;
  for (int t = 0; t < 20; t++)
  {
    delay(50);
    if (checkButton()) return;
  }

  //If the LEDs repeated too many times, reset the array
  if(cellular_automaton_repeat_count >= CELLULAR_AUTOMATON_REPEAT_THRESHOLD)
    initializeCellularAutomaton();
}
uint8_t cellular_automaton_rules(uint8_t a, uint8_t b, uint8_t c)
{
    if      (a == 1 && b == 1 && c == 1) return ruleset[0];
    else if (a == 1 && b == 1 && c == 0) return ruleset[1];
    else if (a == 1 && b == 0 && c == 1) return ruleset[2];
    else if (a == 1 && b == 0 && c == 0) return ruleset[3];
    else if (a == 0 && b == 1 && c == 1) return ruleset[4];
    else if (a == 0 && b == 1 && c == 0) return ruleset[5];
    else if (a == 0 && b == 0 && c == 1) return ruleset[6];
    else if (a == 0 && b == 0 && c == 0) return ruleset[7];
    return 0;    //Should never get here!
}

//TWINKLE RANDOM COLORS ON RANDOM LEDS
//Party modes should return true if checkButton() returns true to assist with breaking out of party shuffle
bool twinkle() 
{
  int lastLED = -1; //Store last LED lit so you never twinkle the same LED twice in a row
  int led = -1;

  randomSeed(millis());

  //Party modes should last for approx 3.2s or 8 'beats' to assist with "Party Shuffle"
  for (int i = 0; i < 21; i++)
  {
    //Select random LED
    while(led == lastLED) led = random(0, NUMBER_OF_LEDS);
    lastLED = led;
  
    //Select random color
    long color = random(0, (uint8_t)LED_COLOR_WHITE + 1);   //Don't randomly select sequential / multi
  
    setLED(led, (LED_COLOR)color);
    delay(25);
    if (checkButton()) return true;
    offLED(led);
  
    delay(25);
    if (checkButton()) return true;
  }

  return false;
}

//RANDOM COLOR ALL LED FLASH AT ~120BPM
//Party modes should return true if checkButton() returns true to assist with breaking out of party shuffle
bool boots_and_pants()
{
  int i;

  randomSeed(millis());

  //Party modes should last for 8 'beats' to assist with "Party Shuffle"
  for(int j = 0; j < 8; j++)
  {
    //Select random color
    long color = random(0, (uint8_t)LED_COLOR_END);   //Randomly selecting sequential or multi is ok and gives an occasional treat
    
    for(i = 0; i < NUMBER_OF_LEDS; i++)
    {
      setLED(i, (LED_COLOR)color);
    }
  
    for(i = 0; i < NUMBER_OF_LEDS; i++)
    {
      offLED(i);
    }
  
    if (checkButton()) return true;
  }

  return false;
}

//FAKE VU METER PEAKING ~150BPM
//Party modes should return true if checkButton() returns true to assist with breaking out of party shuffle
bool vu_meter()
{
  //Party modes should last for 8 'beats' to assist with "Party Shuffle"
  for(int i = 0; i < 8; i++)
  {
    setLED(0, LED_COLOR_GREEN);
    setLED(1, LED_COLOR_GREEN);
    setLED(2, LED_COLOR_GREEN);
    setLED(3, LED_COLOR_YELLOW);
    setLED(4, LED_COLOR_YELLOW);
    setLED(5, LED_COLOR_RED);
    if (checkButton()) return true;
    
    offLED(5);
    offLED(4);
    offLED(3);
    offLED(2);
    offLED(1);
    offLED(0);
    if (checkButton()) return true;
  }

  return false;
}

//CYCLE THROUGH ALL PARTY MODES
void party_shuffle()
{
  SoftPWMSetFadeTime(ALL, LEDModeFadeTimes[(uint8_t)LED_MODE_TWINKLE][0] / LED_BRIGHTNESS, LEDModeFadeTimes[(uint8_t)LED_MODE_TWINKLE][1] / LED_BRIGHTNESS);
  if(twinkle()) return;
  SoftPWMSetFadeTime(ALL, LEDModeFadeTimes[(uint8_t)LED_MODE_BOOTS_AND_PANTS][0] / LED_BRIGHTNESS, LEDModeFadeTimes[(uint8_t)LED_MODE_BOOTS_AND_PANTS][1] / LED_BRIGHTNESS);
  if(boots_and_pants()) return;
  SoftPWMSetFadeTime(ALL, LEDModeFadeTimes[(uint8_t)LED_MODE_VU_METER][0] / LED_BRIGHTNESS, LEDModeFadeTimes[(uint8_t)LED_MODE_VU_METER][1] / LED_BRIGHTNESS);
  if(vu_meter()) return;
}

//FLASHLIGHT MODE
// Not part of mode cycle, enabled by holding button for > 5s
void flashLight()
{
  int i;

  for(i = 0; i < NUMBER_OF_LEDS; i++)
    offLED(i);

  setLED(0, LED_COLOR_WHITE);
  setLED(1, LED_COLOR_WHITE);
  setLED(2, LED_COLOR_WHITE);
  setLED(3, LED_COLOR_WHITE);
  setLED(4, LED_COLOR_WHITE);
  setLED(5, LED_COLOR_WHITE);

  delay(1000);

  //Timeout after about 30 seconds
  for(i = 0; i < 500; i++)
  {
    delay(25);
    //Or exit if user pushes button
    if(checkButtonFlashlight())
      return;
  }

  for(i = 0; i < NUMBER_OF_LEDS; i++)
  offLED(i);
}


/////////////////////////////////////////////////////
//
// HELPER FUNCTIONS
//
/////////////////////////////////////////////////////
void startPowerSave()
{
  //Disable USB - saves ~4mA at 3.3V & 8MHz
  // Reprogramming the teensy will require a press of the teensy's reboot button
  Serial.end();
  
  //Disable ADC - saves ~0.8mA
  ADCSRA = 0;

  //Set unused pins to output mode - saves ~0.4mA
  for(int i = 0; i < NUM_UNUSED_PINS; i++)
    pinMode(UnusedPins[i], OUTPUT);
}

void changeColor() 
{  
  currentColor = (LED_COLOR)((uint8_t)currentColor + 1);  
  if (currentColor == LED_COLOR_END)
    currentColor = (LED_COLOR)0;

  //Flash currently selected color to user
  if(currentColor == LED_COLOR_SEQUENTIAL || currentColor == LED_COLOR_MULTI)
  {
    //Sequential/multi sees many-colors
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      setLED(i, (LED_COLOR)i);
    delay(25);
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      offLED(i);    
  }
  else
  {
    //Otherwise just show color selected
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      setLED(i, currentColor);
    delay(25);
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      offLED(i);
  }

  //Reset the save counter so you have to leave the color on for a bit before it saves
  saveCounter = 0;
}

void changeSequentialColor() 
{  
  currentSequentialColor = (LED_COLOR)((uint8_t)currentSequentialColor + 1);  
  if (currentSequentialColor == LED_COLOR_SEQUENTIAL)
    currentSequentialColor = (LED_COLOR)0;
}

void changeMode() 
{
  currentMode = (LED_MODE)((uint8_t)currentMode + 1);
  if (currentMode == LED_MODE_END)
    currentMode = (LED_MODE)0;

  SoftPWMSetFadeTime(ALL, LEDModeFadeTimes[(uint8_t)currentMode][0] / LED_BRIGHTNESS, LEDModeFadeTimes[(uint8_t)currentMode][1] / LED_BRIGHTNESS);
    
  //Reset the save counter so you have to leave the mode on for a bit before it saves
  saveCounter = 0;
}

//Check the button state and take appropriate action
// Elite points if you modify this to use an interrupt
// but this is A) Easy for non-embedded people to understand -and-
//             B) Gives the benefit of built in debounce
// In hindsight interrupt based code probably would be easier to understand than this mess
bool checkButton() 
{
  int holdCount = 0;
  
  if (!digitalRead(BUTTON_PIN))
  {
    //User pressed the button, shut all LEDs off in preparation
    // to show feedback to user
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      offLED(i);

    //See if they let go < 1s - change color
    for(int i = 0; i < 4; i++)
    {
      delay(50);
      if (digitalRead(BUTTON_PIN))
      {
        changeColor();
        delay(50);
        return false;
      } 
    }

    //User held down for > 1s, flash for feedback to user
    //Flash currently selected color to user
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      setLED(i, currentColor);
    delay(25);
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      offLED(i);
    
    while (!digitalRead(BUTTON_PIN))
    {
      delay(50);
      holdCount++;
      //If user held down for >5s, enable flashlight
      if(holdCount > 25)
        flashLight();
    }

    //Only change mode if user didn't enable the flashlight
    if(holdCount <= 25)
      changeMode();
    return true;
  }

  return false;
}

//Check button status during flashlight mode
// If user presses button, turn off white LEDs and exit
bool checkButtonFlashlight()
{
  if (!digitalRead(BUTTON_PIN))
  {
    for(int i = 0; i < NUMBER_OF_LEDS; i++)
      offLED(i);
    return true;
  }

   return false;
}

void saveSettings()
{
    //The update function only writes to EEPROM if the value has changed
    EEPROM.update(EEPROM_COLOR_LOCATION, (uint8_t)currentColor);
    EEPROM.update(EEPROM_MODE_LOCATION, (uint8_t)currentMode);
}

void restoreSettings()
{
  if((uint8_t)EEPROM.read(EEPROM_COLOR_LOCATION) < (uint8_t)LED_COLOR_END)
    currentColor = (LED_COLOR)EEPROM.read(EEPROM_COLOR_LOCATION);

  if((uint8_t)EEPROM.read(EEPROM_MODE_LOCATION) < (uint8_t)LED_MODE_END)
  {
    currentMode = (LED_MODE)EEPROM.read(EEPROM_MODE_LOCATION);
    SoftPWMSetFadeTime(ALL, LEDModeFadeTimes[(uint8_t)currentMode][0] / LED_BRIGHTNESS, LEDModeFadeTimes[(uint8_t)currentMode][1] / LED_BRIGHTNESS);
  }

}

void setLED(uint8_t ledNumber, LED_COLOR color)
{
  //If the user is in sequential color mode, we need a little extra trickery
  if(color == LED_COLOR_SEQUENTIAL)
    color = currentSequentialColor;

  //If the user is in multi color mode, there is bonus trickery
  else if(color == LED_COLOR_MULTI)
  {
    color = currentSequentialColor;
    changeSequentialColor();
  }
  
  SoftPWMSet(LEDS[ledNumber][0], LEDColorMap[(uint8_t)color][0] * LED_BRIGHTNESS);
  SoftPWMSet(LEDS[ledNumber][1], LEDColorMap[(uint8_t)color][1] * LED_BRIGHTNESS);
  SoftPWMSet(LEDS[ledNumber][2], LEDColorMap[(uint8_t)color][2] * LED_BRIGHTNESS);
}

void offLED(uint8_t ledNumber)
{
  SoftPWMSet(LEDS[ledNumber][0], 0);
  SoftPWMSet(LEDS[ledNumber][1], 0);
  SoftPWMSet(LEDS[ledNumber][2], 0);
}

void offLEDCorrected(uint8_t ledNumber, uint8_t currentIntensity, uint8_t fadeTime)
{
  SoftPWMSetFadeTime(LEDS[ledNumber][0], 0, (uint8_t)((float)255 / (float)currentIntensity * (float)fadeTime));
  SoftPWMSet(LEDS[ledNumber][0], 0);
  SoftPWMSetFadeTime(LEDS[ledNumber][1], 0, (uint8_t)((float)255 / (float)currentIntensity * (float)fadeTime));
  SoftPWMSet(LEDS[ledNumber][1], 0);
  SoftPWMSetFadeTime(LEDS[ledNumber][2], 0, (uint8_t)((float)255 / (float)currentIntensity * (float)fadeTime));
  SoftPWMSet(LEDS[ledNumber][2], 0);
}

void initializeCellularAutomaton()
{
  memset(cells, 0, CELLULAR_AUTOMATON_WIDTH * sizeof(char)); //I know you love to stare at any function that starts with 'mem'
  cells[CELLULAR_AUTOMATON_WIDTH / 2] = 1;
  cellular_automaton_repeat_count = 0;
}

