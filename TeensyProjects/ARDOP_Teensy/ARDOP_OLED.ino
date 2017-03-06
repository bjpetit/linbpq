

#include <Adafruit_SSD1306.h>

/*// If using software SPI (the default case):
  #define OLED_MOSI   9
  #define OLED_CLK   10
  #define OLED_DC    11
  #define OLED_CS    12
  #define OLED_RESET 13
  Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
*/
// Uncomment this block to use hardware SPI
#define OLED_DC     8
#define OLED_CS     9
#define OLED_RESET  7

#define _MOSI 11
#define _SCLK 13		// Clock moved to ALT pin as LED is on A13
#define _MISO 12
// for spi Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);		// i2c

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

uint8_t * buffer;		// OLED Image buffer

void setupOLED()
{
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.

  display.display();
  delay(1000);

  // I can't get the drawline or pixel routines to work from C,
  // so I get the display buffer and update it myself.

  // The displays i have are 128 x 64. As this is primarily for
  // constellation display, I'll use in potrait mode, with the top
  // 64 x 64 used for the constellation and the lower area for short
  // status messages

  // Pixels from a byte seem to be plotted in the 64 direction, and if we
  // mount the display so this is left to right, it works bottom to top.
  // or for conventional (L>R, Top>Bottom) we must reverse one. I'll start
  // from 127 and work down.

  // So byte origin is 127 - y + 128 * (x/8)

  buffer = display.getDisplay();

  clearDisplay();
  DrawAxes(99, "");
  updateDisplay();
}





extern "C"
{
  void DrawAxes(int Qual, char * Mode)
  {
    int y;
    // Draw x and y axes in centre of constallation area

    int yCenter = (ConstellationHeight - 2) / 2;
    int xCenter = (ConstellationWidth - 2) / 2;

    Set8Pixels(0, xCenter, 0x55, WHITE);
    Set8Pixels(8, xCenter, 0x55, WHITE);
    Set8Pixels(16, xCenter, 0x55, WHITE);
    Set8Pixels(24, xCenter, 0x55, WHITE);
    Set8Pixels(32, xCenter, 0x55, WHITE);
    Set8Pixels(40, xCenter, 0x55, WHITE);
    Set8Pixels(48, xCenter, 0x55, WHITE);
    Set8Pixels(56, xCenter, 0x55, WHITE);

    // y is 64 pixels from 31, 0

    for (y = 0; y < 64; y += 2)
      SetPixel(xCenter, y, WHITE);

    display.setRotation(1);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 70);
    display.printf("QUAL %d  ", Qual);
    display.setCursor(0, 82);
    display.print(Mode);
  }

  void SetPixel(int16_t x, int16_t y, int16_t Colour)
  {
    int offset;
    uint8_t val;

    offset = (127 - y) + 128 * (x / 8);

    // top bit is first pixel

    val = (1 << (x & 7));
    if (Colour)
      buffer[offset] |= val;
    else
      buffer[offset] &= ~val;

  }

  void Set8Pixels(int16_t x, int16_t y, int pixels, int16_t Colour)
  {
    int offset;
    offset = (127 - y) + 128 * (x / 8);
    buffer[offset] = pixels;
  }

  void clearDisplay()
  {
    display.clearDisplay();
  }

  void updateDisplay()
  {
    display.display();
  }
}

