/*
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*/  

/*
  PSX V2 : connect a PSX-Guitae or  DualShock controller ot C64.
  By Benson TRSI in 2024. Based on the original HW Design by Toni Westbrook.
  For schematics see the doc folder or find it on Easys EDA / JLPCB: https://oshwlab.com/project


*/

#include <Arduino.h>
#include <PS2X_lib.h>
#include <mcp41xx.h>

/******************************************************************
 * set pins connected to PS2 controller:
 * replace pin numbers by the ones you use
 ******************************************************************/
#define PS2_DAT 2 
#define PS2_CMD 3 
#define PS2_SEL 4 
#define PS2_CLK 5 
#define PS2_AKN 6 // Aknowlegde from controller, not used so far

/******************************************************************
 * set pins connected to c64 JoyStickport:
 */

#define JOY_LEFT 15
#define JOY_RIGHT 14
#define JOY_UP 17
#define JOY_DOWN 16
#define JOY_BUTTON 13

#define MODE_SELECT 12    // Jumper to select behaviour

/*  Emulate Open Collector output for C64 Joystick ports */

#define OC_SETUP(pin)         \
  pinMode(pin, INPUT_PULLUP); \
  digitalWrite(pin, 1);
#define OC_WRITE_HI(pin)      \
  pinMode(pin, INPUT_PULLUP); \
  digitalWrite(pin, 1);
#define OC_WRITE_LOW(pin) \
  digitalWrite(pin, 0);   \
  pinMode(pin, OUTPUT);

/* Shredz64 has some decent values for the potis , they might differ on various SIDs
   There is no analog read, more like coding additional buttons with resistor values */

#define SHREDZ_NONE 0x00 // 100k

#define SHREDZ_STRUM_DOWN 0xd0 // 20k
#define SHREDZ_STRUM_UP 0xa0   // 50k

#define SHREDZ_WHAMMY 0xa0     // 20k
#define SHREDZ_STAR_POWER 0xd0 // 50k  aka as "neck up"

int shredz_mode = 1; //  Set to off, will give true analog values on the potis

/******************************************************************
 * select modes of PS2 controller:
 *   - pressures = analog reading of push-butttons
 *   - rumble    = motor rumbling
 * uncomment 1 of the lines for each mode selection
 ******************************************************************/
// #define pressures   true
#define pressures false
// #define rumble      true
#define rumble false

#define CHECK_PRESSED 0
#define CHECK_RELEASED 1

PS2X ps2x; // create PS2 Controller Class

int error = 0;
byte type = 0;
byte vibrate = 0;

void initController()
{

  // CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************

  OC_SETUP(PS2_AKN); // Switch to input with Pullup . Not used by now
  // setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  asm("WDR"); // Watchdog Reset will not happen

  Serial.println(F("\n\n\nTRSI in 2024 presents : PSX64+ by Benson"));
  Serial.println(F("based on PSX-lib by www.billporter.info\n"));
  Serial.println(F("supports PSX-Guitar :"));
  Serial.println(F("   Open Jumper : Shredz64 mode."));
  Serial.println(F("   Closed Jumper : Whammy and Strum as analog values on the POT"));
  Serial.println(F("and DualShock Controller"));
  Serial.println(F("   Left digital as Joystick, square for fire"));
  Serial.println(F("   Right analog as paddles"));

 
  

  if (error == 0)
  {
        
    Serial.println(F("Found Controller, configured successful "));

  }
  else if (error == 1)
    Serial.println(F("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips"));

  else if (error == 2)
    Serial.println(F("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips"));

  else if (error == 3)
    Serial.println(F("Controller refusing to enter Pressures mode, may not support it. "));

  //  Serial.print(ps2x.Analog(1), HEX);

  delay (100);
  
 
  type = ps2x.readType();
  switch (type)
  {
  case 0:
    Serial.print("Unknown Controller type found ");
    break;
  case 1:
    Serial.print("DualShock Controller found ");
    break;
  case 2:
    Serial.print("GuitarHero Controller found ");
    break;
  case 3:
    Serial.print("Wireless Sony DualShock Controller found ");
    break;
  }
}

unsigned char whammy_val;
unsigned char whammy_val_old = 0xff;

// This is for DualShock controller
#define ANALOG_STICK_VALS 4

unsigned char pad_stick_val_old[ANALOG_STICK_VALS];
int check_sticks[ANALOG_STICK_VALS] = {PSS_LX, PSS_LY, PSS_RX, PSS_RY};
// We have two axis on two sticks, but only two analog outputs , so define, which one should be used
int map_sticks[ANALOG_STICK_VALS] = {0, 0, POTA_CS, POTB_CS};
const char *caption_sticks[ANALOG_STICK_VALS] = {"Left horiz:", "Left vert:", "Right horiz:", "Right vert:"};

unsigned char strum_val = 0x3f;

/* Check if Button is pressed or released and switch the joy-stick-lines on or off*/
int checkPsxButton(unsigned int psx_button, int joy_button, const char *caption)
{
  int ret = 0;

  if (ps2x.ButtonPressed(psx_button))
  {
    Serial.print(caption);
    Serial.println(F(" Pressed"));
    OC_WRITE_LOW(joy_button);
    ret = 1;
  }
  if (ps2x.ButtonReleased(psx_button))
  {
    Serial.print(caption);
    Serial.println(F(" Released"));
    OC_WRITE_HI(joy_button);
    ret = 2;
  }
  return ret;
}

void readContoller()
{
  /* You must Read Gamepad to get new values and set vibration values
     ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
     if you don't enable the rumble, use ps2x.read_gamepad(); with no values
     You should call this at least once a second
   */
  int star_pressed =false;

  if (error == 1) // skip loop if no controller found
    return;

  if (type == 2)
  {                      // Guitar Hero Controller
    ps2x.read_gamepad(); // read controller

    checkPsxButton(GREEN_FRET, JOY_UP, "Green Fret");
    checkPsxButton(RED_FRET, JOY_DOWN, "Red Fret");
    checkPsxButton(YELLOW_FRET, JOY_LEFT, "Yellow Fret");
    checkPsxButton(BLUE_FRET, JOY_RIGHT, "BLue Fret");
    checkPsxButton(ORANGE_FRET, JOY_BUTTON, "Orange Fret");

    if (ps2x.ButtonPressed(STAR_POWER))
    {
      Serial.println(F("Star Power Command on"));
      mcp4151_writeValue(POTB_CS, SHREDZ_STAR_POWER);
      star_pressed=true;
    }

    if (ps2x.ButtonReleased(STAR_POWER))
    {
      Serial.println(F("Star Power Command off"));
      mcp4151_writeValue(POTB_CS, SHREDZ_NONE);
      star_pressed=false;
    }

    if (shredz_mode){
      if(ps2x.ButtonPressed(UP_STRUM)){
        Serial.println(F("Strum up pressed"));
        mcp4151_writeValue(POTA_CS,SHREDZ_STRUM_UP);
      }

      if (ps2x.ButtonReleased(UP_STRUM))
      {
        Serial.println(F("Strum up released"));
        mcp4151_writeValue(POTA_CS, SHREDZ_NONE);
      }

      if(ps2x.ButtonPressed(DOWN_STRUM)){
        Serial.println(F("Strum down pressed"));
        mcp4151_writeValue(POTA_CS,SHREDZ_STRUM_DOWN);
      }

      if(ps2x.ButtonReleased(DOWN_STRUM)){
        Serial.println(F("Strum down released"));
        mcp4151_writeValue(POTA_CS,SHREDZ_NONE);
      }
    }
    else
    {
      if (ps2x.Button(UP_STRUM))
      { // will be TRUE as long as button is pressed
        if (strum_val < 0xfc)
        {
          strum_val+=3;
        }
        Serial.print(F("Up Strum :0x"));
        Serial.println(strum_val, HEX);
        mcp4151_writeValue(POTA_CS, strum_val);
      }
      if (ps2x.Button(DOWN_STRUM))
      {
        if (strum_val > 3)
        {
          strum_val-=3;
        }
        Serial.print(F("Down Strum :0x"));
        Serial.println(strum_val, HEX);
        mcp4151_writeValue(POTA_CS, strum_val);
      }
    }
    if (ps2x.Button(PSB_START)) // will be TRUE as long as button is pressed
      Serial.println(F("Start is being held"));
    if (ps2x.Button(PSB_SELECT))
      Serial.println(F("Select is being held"));

    whammy_val = ps2x.Analog(WHAMMY_BAR);
    if (whammy_val != whammy_val_old)
    {
      Serial.print(F("Wammy Bar Position:"));
      Serial.println(whammy_val, DEC);
      whammy_val_old = whammy_val;
    }
    if (shredz_mode)
    {
      if (whammy_val < 0x3f)
      {
        mcp4151_writeValue(POTB_CS, SHREDZ_WHAMMY);
      }else{
        if (!star_pressed){ // Don't overwrite the star-command
          mcp4151_writeValue(POTB_CS, SHREDZ_NONE);
        }
      }

    }
    else
    {
      mcp4151_writeValue(POTB_CS, whammy_val*2);
    }
  }
  else
  {                              // DualShock Controller
    ps2x.read_gamepad(false, 0); // read controller
    if (ps2x.NewButtonState())
    { // will be TRUE if any button changes state (on to off, or off to on)
#ifdef _DEBUG
      Serial.print("Got data:");
      Serial.println(ps2x.ButtonDataByte());
#endif
      checkPsxButton(PSB_PAD_LEFT, JOY_LEFT, "PAD left");
      checkPsxButton(PSB_PAD_RIGHT, JOY_RIGHT, "PAD right");
      checkPsxButton(PSB_PAD_UP, JOY_UP, "PAD up");
      checkPsxButton(PSB_PAD_DOWN, JOY_DOWN, "PAD down");
      checkPsxButton(PSB_SQUARE, JOY_BUTTON, "PAD square");
    }
    for (int i = 0; i < ANALOG_STICK_VALS; i++)
    {
      byte stick_val = ps2x.Analog(check_sticks[i]);
      if (stick_val != pad_stick_val_old[i])
      {
        pad_stick_val_old[i] = stick_val;
        Serial.print(caption_sticks[i]);
        Serial.println(stick_val);
        if (map_sticks[i])
        {
          mcp4151_writeValue(map_sticks[i], stick_val);
        }
      }
    }
  }
  delay(1);
}
void initJoyport()
{

  OC_SETUP(JOY_UP);
  OC_SETUP(JOY_DOWN);
  OC_SETUP(JOY_LEFT);
  OC_SETUP(JOY_RIGHT);
  OC_SETUP(JOY_BUTTON);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MODE_SELECT, INPUT_PULLUP);
  initController();
  initJoyport();
  init_mcp4151();
  for (int i = 0; i < ANALOG_STICK_VALS; i++)
  {
    pad_stick_val_old[i] = 0xff;
  }
  mcp4151_writeValue(POTA_CS, SHREDZ_NONE); // turn potis to non-function
  mcp4151_writeValue(POTB_CS, SHREDZ_NONE);
}

void loop()
{
  int mode;
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, 0);
  readContoller();
  digitalWrite(LED_BUILTIN, 1);
  mode = digitalRead(MODE_SELECT);
  if (mode != shredz_mode){
    Serial.print (F("Mode change: ShredzMode is "));
    if (!mode){
      Serial.println("off");
    }else{
      Serial.println("on");
    }
    shredz_mode=mode;
  }
}
