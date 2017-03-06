// Program to allow an ESP01 to be programmed via a Teensy 3.2/3.6 on a PI Teensy board

// Data is passed between the The USB port and the ESP01 serial port connected to Serial3,
// and a simple command handler run on Serial1 to toggle the
// ESP Reset and Program pins

// To program the Arduino IDE would be attached to the USB port and a
// terminal program to Serial1

#define ESPReset 21
#define ESPProg 22
#define ESP2 23

#define LED0 2
#define LED1 3
#define LED2 4
#define LED3 5

int Progmode = 0;
int ProgTime = 0;


void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial3.begin(115200);
  pinMode(ESPReset, OUTPUT);
  pinMode(ESPProg, OUTPUT);
  pinMode(ESP2, OUTPUT);
  digitalWriteFast(ESPReset, HIGH);
  digitalWriteFast(ESPProg, HIGH);
  digitalWriteFast(ESP2, HIGH);

  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Flash Leds to show starting

  SetLED(LED0, 1);
  SetLED(LED1, 1);
  SetLED(LED2, 1);
  SetLED(LED3, 1);
  delay(200);
  SetLED(LED3, 0);
  delay(200);
  SetLED(LED2, 0);
  delay(200);
  SetLED(LED1, 0);
  delay(200);
  SetLED(LED0, 0);

  Serial1.println("ESP Programmer Ready");
}

void SetLED(int LED, int state)
{
  state = !state;			// LEDS inverted on PI Board
  digitalWriteFast(LED, state);
}

unsigned char RXBUFFER[512];
void loop()
{
  int Avail = Serial.available();
  int Count;

  Avail = Serial.available();

  // As soon as we see something on Serial, switch to Program Mode

  if (Avail)
  {
    if (Progmode == 0)
    {
      // Toggle reset while Prog is low

      pinMode(ESPProg, OUTPUT);				// driven at boot, so leave as input till needed
      digitalWriteFast(ESPProg, LOW);
      SetLED(LED0, 1);
      delay(100);
      digitalWriteFast(ESPReset, LOW);
      delay(100);
      digitalWriteFast(ESPReset, HIGH);
      Serial1.println("In Program Mode");
      Progmode = 1;
      ProgTime = millis();
    }

    if (Avail > 499)
      Avail = 499;

    Count = Serial.readBytes((char *)RXBUFFER, Avail);
    Serial3.write(RXBUFFER, Count);
    Serial1.write(RXBUFFER, Count);

    ProgTime = millis();
  }

  Avail = Serial3.available();

  if (Avail)
  {
    if (Avail > 499)
      Avail = 499;

    Count = Serial3.readBytes((char *)RXBUFFER, Avail);
    Serial.write(RXBUFFER, Count);
    Serial1.write(RXBUFFER, Count);
  }

  Avail = Serial1.available();

  if (Avail)
  {
    if (Avail > 499)
      Avail = 499;

    Count = Serial1.readBytes((char *)RXBUFFER, Avail);

    if (RXBUFFER[0] == 'r')
    {
      digitalWriteFast(ESPProg, HIGH);
      SetLED(LED0, 0);
      digitalWriteFast(ESPReset, LOW);
      delay(100);
      digitalWriteFast(ESPReset, HIGH);
    }

    if (RXBUFFER[0] == 'p')
    {
      // Toggle reset while Prog is low

      pinMode(ESPProg, OUTPUT);				// driven at boot, so leave as input till needed
      digitalWriteFast(ESPProg, LOW);
      SetLED(LED0, 1);
      delay(100);
      digitalWriteFast(ESPReset, LOW);
      delay(100);
      digitalWriteFast(ESPReset, HIGH);
      //    digitalWriteFast(ESPProg, HIGH);
      //    pinMode(ESPProg, INPUT);
    }
  }

	// Leave program mode if nothing heard for 3 seconds

  if (Progmode && ((millis() - ProgTime) > 3000))
  {
  	  digitalWriteFast(ESPProg, HIGH);
      SetLED(LED0, 0);
      digitalWriteFast(ESPReset, LOW);
      delay(100);
      digitalWriteFast(ESPReset, HIGH);
      Progmode = 0;
      Serial1.println("Leave Program Mode");	
  }
}
