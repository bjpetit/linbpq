// Board specific configuraton for Teensy

// There are currently two boards, one designed by the WDT and one by me (G8BPQ). Mine is 
// a Raspberry PI form factor board. The WDT board is indended to be a standalone unit.

// Currently there are two applications using these boards, ARDOP and a port of Thomas Sailer's
// Packet Soundmodem. The port supports 1200 and 9600 modems. 

// WDTBOARD is the WDT version, with 4 LEDS and Switches, an Adafruit TFT display.
// a 10 segment LED bar driven by a CAT4016 and two digital pots using i2c. The standard
// setup is with the Host Port on USB and a monitor/debug port on Serial1, though a Bluetooth 
// version is planned.

// PIBOARD is the Raspberry PI i2c Board. It has 4 LEDS and the Host port on Serial1. There is no TFT
// CAT4016 Display or switches. Level setting pots are on SPI

#define TEENSY


#define PIBOARD

#ifdef PIBOARD

// Can use Seraal or I2C for Host Interface

// If we define I2CHOST we shoudlnt define HOSTPORT

//#define HOSTPORT Serial1
#define MONPORT Serial1

#define i2cSlaveSupport
#define I2CSLAVEADDR 0x1F
#define I2CHOST

#define HASPOTS
#define SPIPOTS
#define SPIPOTCS 10

#define pttPin 6

#define LED0 2
#define LED1 3
#define LED2 4
#define LED3 5

#else

#define WDTBOARD

#define HOSTPORT Serial
#define MONPORT Serial1

#define TFT
#define BARLEDS


//#define HASPOTS
//#define SPIPOTS
#define I2CPOTADDR

#define pttPin 6

#define LED0 24
#define LED1 25
#define LED2 26
#define LED3 31

#define SW1 27
#define SW2 28
#define SW3 29
#define SW4 30

// CAT4016 10 LED display

#define CLK 2
#define BLANK 3
#define LATCH 4
#define SIN 5

#endif
