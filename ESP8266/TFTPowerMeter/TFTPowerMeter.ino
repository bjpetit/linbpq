#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

#include <WiFiUdp.h>
#include <Wire.h>

#include <ADS7846.h>

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress IPAddr(192, 168, 42, 12);

char packetBuffer[256]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP

WiFiUDP udp;

#include <SPI.h>

#define PI3.5TFT

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9340.h>

#include <Wire.h>
#include <Adafruit_INA219.h>

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

// Left Solar, Right Solar, Dougen, Domestic, Engine, Thruster

Adafruit_INA219 ina219[6] = {0x40, 0x41, 0x44, 0x4C , 0x4d, 0x4e};

// For ESP8266

#define _sclk 14
#define _miso 12
#define _mosi 13
#define _cs 15
#define _dc 2
#define _rst 0

// clk = 14 NodeMCU D5
// mosi = 13        D7
// miso = 12        D6
// DC = 2           D4
// CS = 15          D8

// i2c 4/5          D1/2

// Use 0 for Touch Pad CS = D3
// Need to disable RST in code (connected to ESP RST

ADS7846 ads(0);         // Use pin 0 for CS

// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

int doGraph = 0;

float lastI[6][400] = {0};
float lastV[6][400] = {0};

short lastY[6][400] = {0};

int graphIndex = 0;

void setup()
{
  int count = 0, address, error;
  uint8_t MAC_array[6];
  char MAC_char[18] = "";

  Serial.begin(115200);
  while (!Serial);

  tft.begin();
  yield();

  tft.setRotation(1);
  tft.fillScreen(ILI9340_BLACK);
  yield();

  tft.setCursor(0, 0);
  tft.setTextColor(ILI9340_WHITE, ILI9340_BLACK);  tft.setTextSize(3);

  Serial.println(F("\nTFT Power Meter"));

  httpUpdater.setup(&httpServer);

  httpServer.on("/", handleRoot);
//  httpServer.onNotFound(handleNotFound);

  httpServer.begin();

  

  Serial.println(F("\nWeb Updater ready"));
  tft.println(F("Updater ready"));

  Serial.println(F("\nScanning i2c bus"));
  tft.println(F("Scanning i2c bus"));

  Wire.begin();

  for (address = 1; address <= 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print(F("i2c device at Address 0x"));
      Serial.println(address, HEX);
      tft.print(F("i2c device at Address 0x"));
      tft.println(address, HEX);
      yield();
    }
    else if (error == 4)
    {
      Serial.print(F("i2c Error at Address 0x"));
      Serial.println(address, HEX);
      tft.print(F("i2c Error at Address 0x"));
      tft.println(address, HEX);
      yield();
    }
  }

  ina219[0].begin();
  ina219[1].begin();
  ina219[2].begin();
  ina219[3].begin();
  ina219[4].begin();
  ina219[5].begin();

  //  ina219[0].ina219SetCalibration_16V_400mA();
  //  ina219[1].ina219SetCalibration_16V_400mA();
  //  ina219[2].ina219SetCalibration_16V_400mA();
  //  ina219[3].ina219SetCalibration_16V_400mA();

  tft.println(F("Wifi connect"));

  // WiFi.mode(WIFI_OFF);
  // WiFi.forceSleepBegin();
  // delay(1); //Needed, at least in my tests WiFi doesn't power off without this for some reason

  //  WiFi.forceSleepWake();

  WiFi.mode(WIFI_STA);

  Serial.println(F("Connecting to wifi Popo"));

  WiFi.begin("Popo", "");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    tft.print(".");
    if (count++ > 10)
      break;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(F("Connecting to wifi Hilton"));
    WiFi.begin("Hilton", "gb7bpqgb7bpq");
    tft.println(F("Wifi connect Hilton"));
    IPAddr.fromString("192.168.1.64");
    count = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      tft.print(".");
      if (count++ > 10)
        break;
    }
  }

  Serial.println("");
  tft.println("");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println(F("WIFI : Connected!"));
  }
  else
    Serial.println(F("WIFI : Connected Failed"));

  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i)
  {
    sprintf(MAC_char, "%s%02x:", MAC_char, MAC_array[i]);
  }

  Serial.println(MAC_char);

  tft.println(WiFi.localIP());
  delay(1000);
  tft.fillScreen(ILI9340_BLACK);
  yield();

  udp.begin(localPort);
  ads.begin();
  ads.setRotation(1);
}

int n = 90, i;
double V = 12.00;


double shuntR[6] = {0.1, 0.1, 0.1, 0.001, .001, .00075};
double busV[6], shuntV[6];
double I[6];

char Line[256];

void loop(void)
{
  int rawBus[8], rawShunt[8];

  int n;
  unsigned long start = micros();

  httpServer.handleClient();

  for (i = 0; i < 6; i++)
  {
    rawBus[i] =  ina219[i].getBusVoltage_raw();
    rawShunt[i] = ina219[i].getShuntVoltage_raw();

    // low bit is .01 mV. with 1 ohm, low bit = .01 mA, so FSD = 320 mA
    // low bit is .01 mV. with .1 ohm, low bit = .1 mA, so FSD = 3.2 A
    // low bit is .01 mV. with .001 ohm, low bit = 10 mA, so FSD = 320 A

    //rawBus is in millivolts

    // Convert raw here to save messing with chip calibration for different shunt resistors

    busV[i] = rawBus[i] / 1000.0;
    shuntV[i] = rawShunt[i] / 100000.0;
    I[i] = shuntV[i] / shuntR[i];

    lastV[i][graphIndex] = busV[i];
    lastI[i][graphIndex] = I[i];

    if (graphIndex == 400)
      graphIndex = 0;

    Serial.print(i);
    Serial.print(" Bus: "); Serial.print(busV[i]); Serial.print(" V  ");
    Serial.print("Shunt: "); Serial.print(shuntV[i]); Serial.print(" V ");
    Serial.print("Current: "); Serial.print(I[i]); Serial.println(" A");
  }
  graphIndex++;
  //  Serial.println("");

  if (I[0] < 0) I[0] = 0;
  if (I[1] < 0) I[1] = 0;


  // Serial.print("read and print took ");
  // Serial.println(micros() - start);
  //  delay(1000);

  start = micros();

  if (doGraph)
  {
    int x, y, index = graphIndex;

    for (x = 0; x < 400; x++)
    {
      tft.drawPixel(x + 40 , 300 - lastY[3][x], ILI9340_BLACK);		// remove old pixel
      y = (lastV[3][index] - 12.0) * 100.0;									// 100 pixels per volt - gives 12 - 15
      tft.drawPixel(x + 40, 300 - y, ILI9340_GREEN);
      lastY[3][x] = y;
      if (index++ > 400)
        index = 0;
    }
    tft.drawFastHLine(40, 100, 400, ILI9340_BLUE),
    tft.drawFastHLine(40, 200, 400, ILI9340_BLUE),
    tft.drawFastHLine(40, 300, 400, ILI9340_BLUE),
    delay(1000);
  }
  else
  {
    tft.setCursor(0, 0);
    tft.setTextColor(ILI9340_WHITE, ILI9340_BLACK);
    tft.setTextSize(3.5);
    tft.println("Solar Panels");
    tft.setTextSize(4);
    tft.print(busV[0]);
    tft.print("V ");
    tft.print(I[0]);
    tft.print("A ");
    tft.print(I[1]);
    tft.println("A ");

    yield();

    tft.setTextSize(3);
    tft.println("Domestic");
    tft.setTextSize(4);
    tft.print(busV[3]);
    tft.print("V ");
    tft.print(I[3]);
    tft.println("A   ");

    yield();

    tft.setTextSize(3);

    tft.println("Engine");
    tft.setTextSize(4);
    tft.print(busV[4]);
    tft.print("V ");
    tft.print(I[4]);
    tft.println("A   ");

    yield();

    tft.setTextSize(3);
    tft.println("Thruster");
    tft.setTextSize(4);
    tft.print(busV[5]);
    tft.print("V ");
    tft.print(I[5]);
    tft.println("A   ");

    tft.setCursor(300, 290);
    tft.setTextSize(3);
    tft.print(ads.getXraw() - 555);
    tft.print(" ");
    tft.print(ads.getYraw() - 63);
    tft.print("     ");

    Serial.print("Refresh took ");
    Serial.println(micros() - start);
  }
  //  delay(1000);

  n = sprintf(Line, "$IIESP,A,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
              rawBus[0], rawShunt[0], rawBus[1], rawShunt[1], rawBus[2], rawShunt[2],
              rawBus[3], rawShunt[3], rawBus[4], rawShunt[4], rawBus[5], rawShunt[5]);

  SendUDP(IPAddr, Line, n);

  start = micros();

  ads.service();

  uint16_t x = ads.getXraw() - 555;
  uint16_t y = ads.getYraw() - 63;
  uint16_t z = ads.getPressure();
  if (z)
  {
    Serial.print("X = ");
    Serial.println(x, DEC);
    Serial.println(y, DEC);
    Serial.println(z, DEC);

    if (x < 240 && doGraph)
    {
      tft.fillScreen(ILI9340_BLACK);
      doGraph = 0;
    }
    else
    {
      if (x > 240 && x < 1000 && doGraph == 0)
      {
        tft.fillScreen(ILI9340_BLACK);
        doGraph = 1;
      }
    }
  }
  // Serial.print("Touch took ");
  //  Serial.println(micros() - start);
  //  delay(1000);

}

void SendUDP(IPAddress & address, char * packetBuffer, int len)
{
  udp.beginPacket(address, 8873);
  udp.write(packetBuffer, len);
  Serial.print(udp.endPacket());
  Serial.print(packetBuffer);
}

void handleRoot()
{
  httpServer.send(200, "text/plain", Line);
}
/*
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += httpServer.uri();
  message += "\nMethod: ";
  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += httpServer.args();
  message += "\n";
  for (uint8_t i=0; i<httpServer.args(); i++){
    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
  }
  httpServer.send(404, "text/plain", message);
}
*/

