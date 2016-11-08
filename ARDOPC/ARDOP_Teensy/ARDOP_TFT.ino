// Code to drive a Adafruit/PRJC TFT display

#include "SPI.h"
#include "ILI9341_t3.h"		// This is the PRJC optimised version of driver

extern "C" {
  void displayCall(int dirn, char * Call);
}
extern "C" {
  void displayState(char * Call);
}

#define TFT_DC  9
#define TFT_CS 10
#define _RST 255		// Not Used
#define _MOSI 11
#define _SCLK 14		// Clock moved to ALT pin as LED is on A13
#define _MISO 12

// Use hardware SPI
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, _RST, _MOSI, _SCLK, _MISO);

void setupTFT()
{
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.fillRect(100, 100, 60, 16, ILI9341_RED);

}

void displayCall(int dirn, char * Call)
{
  char paddedcall[12] = "           ";

  paddedcall[0] = dirn;
  memcpy(paddedcall + 1, Call, strlen(Call));

  tft.setCursor(0, 72);
  tft.print(paddedcall);
}
void displayState(char * State)
{
  tft.setCursor(0, 48);
  tft.print("          ");
  tft.setCursor(0, 48);
  tft.print(State);
}

