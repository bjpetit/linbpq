// Code to drive a Adafruit/PRJC TFT display or Pi_TFT 3.5 Dislay

#include "TeensyConfig.h"

#include "SPI.h"

extern "C" void CAT4016(int value);

extern "C" void SetLED(int Pin, int State);

#ifdef PI35TFT

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

// For Teensy

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 9
#define _rst 8

// Color definitions
#define	ILI9341_BLACK   0x0000
#define	ILI9341_BLUE    0x001F
#define	ILI9341_RED     0xF800
#define	ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_WHITE   0xFFFF


Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

#else

// PRJC/Adafruit Display


#include "ILI9341_t3.h"

#define TFT_DC  9
#define TFT_CS 10
#define _RST 255		// Not Used
#define _MOSI 11
#define _SCLK 14		// Clock moved to ALT pin as LED is on A13
#define _MISO 12

// Use hardware SPI

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, _RST, _MOSI, _SCLK, _MISO);
#endif



#define LED0 24
#define LED1 25
#define LED2 26
#define LED3 31

#define DCDLED LED0

#define SW1 27
#define SW2 28
#define SW3 29
#define SW4 30

const int barcolours[10] = {
  ILI9341_YELLOW, ILI9341_YELLOW, ILI9341_GREEN,
  ILI9341_GREEN, ILI9341_GREEN, ILI9341_GREEN, ILI9341_GREEN,
  ILI9341_RED, ILI9341_RED, ILI9341_RED
};

// Signal levels for each bar

const int barlevels[10] = {
  1000, 2000, 5000, 8000, 11000,
  16000, 24000, 28000, 30000, 32000
};

const int CAT4016Levels[11] = {
  0, 1, 0b11, 0b111, 0b1111, 0b11111,
  0b111111, 0b1111111, 0b11111111, 0b111111111, 0b1111111111
};

extern "C"
{
  void displayLevel(int level)
  {
#ifdef WDTBOARD

    int i;

    for (i = 0; i < 10; i++)
    {
      if (level > barlevels[i])
        tft.fillRect(15 * i, 100, 14, 16, barcolours[i]);
      else
        tft.fillRect(15 * i, 100, 14, 16, ILI9341_BLACK);
    }

    for (i = 0; i < 10; i++)
    {
      if (level < barlevels[i])
        break;
    }
    CAT4016(CAT4016Levels[i]);
#endif
  }
  
  void displayPTT(int State)
  {
#ifdef WDTBOARD
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.setCursor(0, 72);
    if (State)
      tft.print("PTT");
    else
      tft.print("   ");
#endif
  }

  void displayDCD(int State)
  {
#ifdef WDTBOARD
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.setCursor(0, 48);
    if (State)
      tft.print("DCD");
    else
      tft.print("   ");
#endif
    SetLED(DCDLED, State);
  }
}
Print * setupTFT()
{
  tft.begin();
#ifdef PI35TFT
  tft.setRotation(3);
#else
  tft.setRotation(1);
#endif
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);

  return (Print *)&tft;
}


