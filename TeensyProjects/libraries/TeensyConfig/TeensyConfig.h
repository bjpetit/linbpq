// Board specific configuration for Teensy

// There are currently two boards, one designed by the WDT and one by me (G8BPQ). Mine is 
// a Raspberry PI form factor board. The WDT board is intended to be a standalone unit.

// Currently there are two applications using these boards, ARDOP and a port of Thomas Sailer's
// Packet Soundmodem. The port supports 1200 and 9600 modems. 

// WDTBOARD is the WDT version, with 4 LEDS and Switches, an Adafruit TFT display.
// a 10 segment LED bar driven by a CAT4016 and two digital pots using i2c. The standard
// setup is with the Host Port on USB and a monitor/debug port on Serial1, though a 
// Bluetooth version is planned.

// PIBOARD is the Raspberry PI i2c Board. It has 4 LEDS and the Host port on 
// Serial1 Serial3 or i2c. There is no TFT,  CAT4016 Display or switches.
// Level setting pots are on SPI

//#define PACKET
#define ARDOP

// Standard definitions

#define TEENSY

#define Statsprintf MONprintf
#define WriteExceptionLog MONprintf

#ifdef ARDOP
#include "TeensyConfigARDOP.h"
#endif

#ifdef PACKET
#include "TeensyConfigPacket.h"
#endif
