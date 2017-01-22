// ARDOP TNC Serial Interface for Teensy Board
//

#include <Arduino.h>
#include <EEPROM.h>

#include "TeensyConfig.h"

// This seems to be the only way to change the serial port buffer size
// without editing the IDE core file. We set the equates then include the
// core library source

// We need bigger buffers if we want to use a hardware serial port for the host
// interface (to avoid buffering delays)

// We need to be able to queue a full KISS or Hostmode frame to the host without
// waiting

#define SERIAL1_TX_BUFFER_SIZE	512 // number of outgoing bytes to buffer
#define SERIAL1_RX_BUFFER_SIZE	512

#include "serial1.c"

extern "C" void SaveEEPROM(int Reg, unsigned char Val);
extern "C" void SetPot(int address, int value);
extern "C" unsigned int GetPot(int i);

// i2c support

#if defined I2CHOST || defined I2CKISS || defined I2CMONITOR

#include <i2c_t3.h>

void receiveEvent(size_t count);
void requestEvent(void);

#define MEM_LEN 512
uint8_t databuf[MEM_LEN];

volatile int i2cputptr = 0, i2cgetptr = 0, target = 0;

void i2csetup()
{
  // Setup for Slave mode, address I2SLAVEADDR, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, I2CSLAVEADDR, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

  // Data init

  memset(databuf, 0, sizeof(databuf));

  // register events

  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

unsigned char i2cMessage[512];
int i2cMsgPtr = 0;

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#ifdef I2CHOST

unsigned char i2creply[512] = {0xaa, 0};

volatile int i2creplyptr = 3;
volatile int i2creplylen = 0;

#else

unsigned char i2creply[512] = {192, 8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 3, 192};
unsigned char i2cidle[2] = {15};

volatile int i2creplyptr = 0;
volatile int i2creplylen = 14;

// This is only called if I2CKISS is enabled
void i2cloop()
{
  // print received data - this is done in main loop to keep time spent in I2C ISR to minimum

  while (i2cgetptr != i2cputptr)
  {
    unsigned char c;
    c = databuf[i2cgetptr++];
    i2cgetptr &= 0x1ff;				// 512 cyclic

    MONPORT.printf("%x %c ", c, c);

    if (c == FEND)
    {
      // Start or end of message

      int i, Len;

      if (i2cMsgPtr == 0)
        continue;							// Start of message

      Len = i2cMsgPtr;
      i2cMsgPtr = 0;

      // New message

      MONPORT.printf("Slave received: ");
      for (i = 0; i < Len; i++)
        MONPORT.printf("%x ", i2cMessage[i]);
      MONPORT.printf("\r\n");

      if (i2cMessage[0] == 15)
      {
        // Immediate Command

        if (i2cMessage[1] == 1)
        {
          // Get Params Command

          int Sum = 8, val;

          for (i = 0; i < 9; i++)
          {
            val = EEPROM.read(i);
            i2creply[i + 2] = val;
            Sum ^= val;
          }

          // Reg 9 and 10 are Digital Pots

          val = GetPot(0);
          i2creply[11] = val;
          Sum ^= val;

          val = GetPot(1);
          i2creply[12] = val;
          Sum ^= val;

          i2creply[13] = Sum;
          i2creply[14] = FEND;

          i2creplyptr = 0;
          i2creplylen = 15;
          return;
        }
        return;			// other immediate command
      }
      if (i2cMessage[0] && i2cMessage[0] != 12)
      {
        // Not Normal or Ackmode Data, so set param

        int reg = i2cMessage[0];
        int val = i2cMessage[1];

        if (reg == 9) // SPI Pots
          SetPot(0, val);
        else if (reg == 10)
          SetPot(1, val);
        else
          SaveEEPROM(reg, val);
        return;
      }

      // Data Frame

      return;
    }
    else
    {
      // normal char

      i2cMessage[i2cMsgPtr++] = c;
    }
  }
}

#endif

//
// handle Rx Event (incoming I2C data)
//
void receiveEvent(size_t count)
{
  target = Wire.getRxAddr(); 	// Get Slave address

  while (count--)
  {
    databuf[i2cputptr++] = Wire.readByte();
    i2cputptr &= 0x1ff;		// 512 cyclic buffer
  }
}

//
// handle Tx Event (outgoing I2C data)
//

void requestEvent(void)
{  
#ifdef I2CHOST
  i2creply[1] = i2creplylen;
  i2creply[2] = i2creplylen >> 8;		// First two bytes are length
  Wire.write(i2creply, i2creplylen + 3);
  i2creplylen = 0;
  i2creplyptr = 3;
#else
// TNC-PI interface works a byte at a time

  if (i2creplylen == 0)			// nothing to send
    Wire.write(i2cidle, 0); // return idle
  else
  {
    // return the next byte of the message
    Wire.write(&i2creply[i2creplyptr++], 1);
    i2creplylen--;
  }
#endif
}

#endif

#include <stdarg.h>
extern "C"
{

  void Statsprintf(const char * format, ...)
  {
    return;
  }

  int HostInit()
  {
    return true;
  }


  void PutString(const char * Msg)
  {
#ifdef I2CHOST
    memcpy(&i2creply[3], Msg, strlen(Msg));
    i2creplyptr = 3;
    i2creplylen = strlen(Msg);
#else
    HOSTPORT.write((const uint8_t *)Msg, strlen(Msg));
#endif
  }

  int PutChar(unsigned char c)
  {
#ifdef I2CHOST
    i2creply[i2creplyptr++] = c;
    i2creplylen++;
#else
    HOSTPORT.write(&c, 1);
#endif
    return 0;
  }

  void SerialSendData(const uint8_t * Msg, int Len)
  {
#ifdef I2CHOST
    memcpy(&i2creply[3], Msg, Len);
    i2creplyptr = 3;
    i2creplylen = Len;
#else
    HOSTPORT.write(Msg, Len);
#endif
  }

  void WriteDebugLog(int Level, const char * format, ...)
  {
    char Mess[10000];
    va_list(arglist);

    va_start(arglist, format);
    vsprintf(Mess, format, arglist);
    //	Serial1Print(Mess);
    MONPORT.println(Mess);
    return;
  }
  void WriteExceptionLog(const char * format, ...)
  {
    char Mess[10000];
    va_list(arglist);

    va_start(arglist, format);
    vsprintf(Mess, format, arglist);
    //	Serial1Print(Mess);
    MONPORT.println(Mess);
    return;
  }

  void CloseDebugLog()
  {
  }


  void CloseStatsLog()
  {
  }
}




