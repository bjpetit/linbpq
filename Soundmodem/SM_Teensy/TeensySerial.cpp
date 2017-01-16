// ARDOP TNC Serial Interface for Teensy Board
//

#include <Arduino.h>

#define TEENSY

#include "TeensyConfig.h"

// This seems to be the way to change the serial port buffer size 
// without editing the IDE core file. We set the equates then include the
// core library source

// We need bigger buffers if we want to use a hardware serial port for the host
// interface (to avoid buffering delays)

// We need to be able to queue a full KISS frame to the host without 
// waiting

#define SERIAL1_TX_BUFFER_SIZE	512 // number of outgoing bytes to buffer
#define SERIAL1_RX_BUFFER_SIZE	512

#include "serial1.c"

// i2c support


#include <i2c_t3.h>

// Function prototypes
void receiveEvent(size_t count);
void requestEvent(void);

#define MEM_LEN 512
uint8_t databuf[MEM_LEN];

volatile int i2cputptr = 0, i2cgetptr = 0;

void i2csetup()
{
  // Setup for Slave mode, address 0x66, pins 18/19, external pullups, 400kHz
  Wire.begin(I2C_SLAVE, 0x66, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);

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

unsigned char i2creply[] = {192, 8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 3, 192};
unsigned char i2cidle[2] = {15};

volatile int i2creplyptr = 0;
volatile int i2creplylen = 14;


void i2cloop()
{
  // print received data - this is done in main loop to keep time spent in I2C ISR to minimum
  while (i2cgetptr != i2cputptr)
  {
    unsigned char c;
    c = databuf[i2cgetptr++];
    i2cgetptr &= 0x1ff;				// 512 cyclic

    if (c == FEND)
    {
      // Start or end of message

      int i;

      if (i2cMsgPtr == 0)
        continue;							// Start of message

      // New message

      if (i2cMessage[0] == 15 && i2cMessage[1] == 1)
      {
        // Get Params Command

        i2creplyptr = 0;
        i2creplylen = 14;
      }
      else
      {

        MONPORT.printf("Slave received: ");
        for (i = 0; i < i2cMsgPtr; i++)
          MONPORT.printf("%x ", i2cMessage[i]);

        MONPORT.printf("\r\n");
      }

      i2cMsgPtr = 0;
    }
    else
    {
      // normal char

      i2cMessage[i2cMsgPtr++] = c;
    }
  }
  //  if (i2ctxflag)
  //  {
  //   i2ctxflag = 0;
  //   MONPORT.printf("i2c read request\r\n");
  // }
}

//
// handle Rx Event (incoming I2C data)
//
void receiveEvent(size_t count)
{
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
  // PI interface works a byte at a time

  if (i2creplylen == 0)			// nothing to send
    Wire.write(i2cidle, 1); // return idle
  else
  {
    // return the next byte of the message

    Wire.write(&i2creply[i2creplyptr++], 1);
    i2creplylen--;
  }
}







// SCS Port is on USB. Debug on Serial1

#define TRUE 1
#define FALSE 0

int HostInit()
{
  return TRUE;
}

void PutString(const char * Msg)
{
  //	SerialSendData(Msg, );
  HOSTPORT.write((const uint8_t *)Msg, strlen(Msg));
}

int PutChar(unsigned char c)
{
  // SerialSendData(&c, 1);
  HOSTPORT.write(&c, 1);
  return 0;
}

#include <stdarg.h>
extern "C"
{
  void SerialSendData(const uint8_t * Msg, int Len)
  {
    HOSTPORT.write(Msg, Len);
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
  void Statsprintf(const char * format, ...)
  {
  }
  void CloseDebugLog()
  {
  }


  void CloseStatsLog()
  {
  }
}




