# MassHackersBadge2015

## Programming

The stock firmware is written in the Arduino IDE. Before making modifications, you should make sure you can properly flash the stock code before going wild. You will need:

* Arduino IDE (Recommend 1.6.5)
* <a href="https://www.pjrc.com/teensy/td_download.html">Teensyduino</a> plugin for Arduino

During the Teensyduino plugin, it will ask you any additional libraries you wish to install. **You must minimally install the SoftPWM library**, which is a pulse width modulation library which allows the badge to fade the LEDs in and out instead of just turn them on and off.

When flashing the Teensy module, use the following settings found under the Arduino menu *Tools*

* Board: "Teensy 2.0"
* CPU Speed: "8 MHz"

It is **VERY IMPORTANT** to set the speed to *8 MHz* or lower. Your teensy board is modified to operate at 3.3 volts instead of the default 5 volts. **Running at the default 16MHz is OVERCLOCKING** the processor, which will probably work fine for a while, but I can't guarantee how long it will last in the desert heat. Don't freak out if you do it by accident and the LEDs flash extra fast, just reflash again with the correct clock speed.

The Arduino IDE will then prompt you to press the reset button on the Teensy module, after which it will flash the board. You must press the reset button, since access to the bootloader through USB is disabled in the badge firmware to save power.

The Teensy module can be removed from the badge, and is populated with headers so you can pop it into a breadboard and use it for any other project you desire. Remember it has been modified to run at 3.3 volts, if you need to switch it back to 5 volts for other uses, <a href="https://www.pjrc.com/teensy/3volt.html">follow the instructions here at the part "If you decide to switch back to 5 volts..."</a>


## Firmware

### Adding a new color into the rotation
* Insert color into the `LED_COLOR` enum of the header file, ideally somewhere before `LED_COLOR_WHITE`
* Insert the RGB color values into the main file, `LEDColorMap` in the order it was inserted into `LED_COLOR` You will also need to increase the array size to accomodate this

That's it! The color automatically gets inserted into the rotation upon flashing. There is a trick to the SoftPWM library. Let's say for example you create a color that is RED 127, GREEN 255, BLUE 0. And your fade in time is 1000ms. The red and green fade in at the same rate, so red reaches its full brightness after 500ms, but green continues to fade in for an additional 500ms. You can overcome this by individually setting the fade in/out component of the red portion of the LED to be double that of the green portion of the LED. I started some work on this in the function `offLEDCorrected()` but it is up to you to finish it, and write a complementary function for turning on the LEDs!

### Adding a new mode into the rotation
* Insert the mode into the `LED_MODE` enum of the header file, which *must* be before `LED_MODE_END`
* Set the LED fade in and fade out times for your mode in `LEDModeFadeTimes` in the main file. You will also need to change the size of the array to accomodate this
* Create a handler function that does whatever you want to the LEDs - See other modes for examples
* Add a call to your handler function in the main `loop()` case statement: `switch(currentMode)`

The button presses are not interrupt based, you should plan to call the function `checkButton()` at least after every 50ms during your mode. (This is not truely a 50ms delay due to running at 8MHz, it is closer to 100ms)


## Hardware

Schematic and PCB was designed in Eagle 6.6.0. Eagle does not pre-load the ground pours when loading the PCB design. After opening the PCB you should run the "rats nest" tool to populate the ground pours.

You can remove the Teensy 2.0 module from the badge and put it into a breadboard for other fun hardware projects. The Teensy board is modified to operate at 3.3 volts, if you need to switch it back to 5 volts for other uses, <a href="https://www.pjrc.com/teensy/3volt.html">follow the instructions here at the part "If you decide to switch back to 5 volts..."</a>

### Errata

* Resistors for RED LEDs are R13 through R18 and should be 150 ohms
* Resistors for GREEN and BLUE LEDs are R1 through R12 and should be 15 ohms
* For the RGB LEDs in the BOM, the LED should be soldered on the board with the pin 1 marker on the bottom left, not the top right as on the silk screen

### BOM
| Qty | Description | MPN | Digikey PN |
|-----|-------------|-----|------------|
| 1   | Teensy 2.0  | -   |  - |
| 6   | RGB LED     | CLVBA-FKA-CAEDH8BBB7A363 | CLVBA-FKA-CAEDH8BBB7A363CT-ND |
| 6   | 150 ohm 0603 resistor | RMCF0603FT150R | RMCF0603FT150RCT-ND |
| 12  | 15 ohm 0603 resistor  | RMCF0603FT15R0 | RMCF0603FT15R0CT-ND | 
| 1 | Voltage regulator for Teensy | MCP1825S-3302E/DB | MCP1825S-3302E/DB-ND |
| 2 | Header for Teensy board | PREC012SAAN-RC | S1012EC-12-ND |
| 2 | Socket for badge PCB | 929974-01-12-RK | 929974E-01-12-ND |
| 1 | CR123A Battery Holder | 1019 | 36-1019-ND |
| 1 | Momentary Switch | KMR231NG LFS | CKN10246CT-ND |

### Files

Eagle Project: **Hardware/MassHackersBadge2015.pro**<br>
Eagle Schematic: **Hardware/MassHackersBadge2015.sch**<br>
Eagle PCB: **Hardware/MassHackersBadge2015.brd**

Gerbers for manufacturing: **Hardware/Gerbers**

Design rule check and CAM file for Gerber generation for DirtyPCBs: **Hardware/DirtyPCBs**

### Available Hardware Pins

There are <a href="https://www.pjrc.com/teensy/pinout.html">3 pins</a> available on the Teensy 2.0 to add additional hardware:
* Pin 9 - Digital I/O
* Pin 10 - Digital I/O
* Pin 11 - Analog or Digital I/O

You will need to remove pins you use from the array `UnusedPins` so they do not get disabled during the power conservation routine
