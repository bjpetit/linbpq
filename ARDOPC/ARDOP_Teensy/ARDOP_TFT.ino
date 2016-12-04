// Code to drive a Adafruit/PRJC TFT display

#include "SPI.h"
#include "ILI9341_t3.h"		// This is the PRJC optimised version of driver

#define TFT_DC  9
#define TFT_CS 10
#define _RST 255		// Not Used
#define _MOSI 11
#define _SCLK 14		// Clock moved to ALT pin as LED is on A13
#define _MISO 12

// Use hardware SPI
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, _RST, _MOSI, _SCLK, _MISO);

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
  }
}
void setupTFT()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
}


