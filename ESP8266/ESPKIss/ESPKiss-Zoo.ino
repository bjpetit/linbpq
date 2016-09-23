/*
 *
 * ESP8266 KISS TNC.
 *
 * Will probably use TCP, but could be UDP
 *
 * Some concepts inspired by the TNC-X software by
 *
 * Tone generation is via a 4 bit resistive DAC. This uses a 16 level approxmation
 * to a sine wave
 *
 */

#define TRUE 1
#define FALSE 0

volatile int ticks = 0;

#include <ESP8266WiFi.h>

//how many clients should be able to telnet to this ESP8266

const char* ssid = "Hilton";
const char* password = "gb7bpqgb7bpq";

WiFiServer server(8105);
WiFiClient serverClient;

int TXDELAY = 25;		// Number of flags

unsigned char txBuffer[360] = "0x82aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

unsigned char KISSMsg[360];
unsigned char * kissptr = KISSMsg;
bool ESCFLAG = false;
bool MSGREADY = false;

#define FEND 0xC0	// KISS CONTROL CODES 
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define	RXSIZE 256

// Tones are generated with a 16 step pseudo-sinewave synced to instruction clock (80 MHz)

#define	LOTONE 80000000 / (1200 * 16)
#define HITONE 80000000 / (2200 * 16)

// 80 MHz ticks per bit

#define BIT_DELAY 80000000/1200

static unsigned char rxbytes[RXSIZE];	// Incoming character array
static unsigned char msg_start;			// Index of message start after header
static unsigned char msg_end;			// Index of message ending character
static unsigned char command;			// Used just for toggling
static unsigned char transmitting;		// Keeps track of TX/RX state
static unsigned char rxtoggled;			// Signals frequency just toggled

static unsigned short dcd;				// Carrier detect of sorts
volatile char busy;

unsigned int txtone = LOTONE;
volatile unsigned int nexttonetick;

unsigned int lastCycleCount;


void setup()
{
	Serial.begin(115200);
	Serial.println("Init");

	pinMode(4, OUTPUT);			// DAC3
	pinMode(5, OUTPUT);			// DCD
	pinMode(12, OUTPUT);		// DAC0
	pinMode(13, OUTPUT);		// DAC1
	pinMode(14, OUTPUT);		// DAC2
	pinMode(15, OUTPUT);		// PTT
 
	WiFi.begin(ssid, password);
	Serial.print("\nConnecting to "); Serial.println(ssid);
	uint8_t i = 0;
	
	while (WiFi.status() != WL_CONNECTED && i++ < 20)
		delay(500);
	
	if(!WL_CONNECTED)
		printf("Could not connect to %s\n", ssid);
	else
		printf("Connected to %s\n", ssid);

	//start tcp server
	
	server.begin();
	server.setNoDelay(true);
	
	Serial.print("Ready! Use 'telnet ");
	Serial.print(WiFi.localIP());
	Serial.println(" 8105' to connect");

	// initialise timer0 to generate interrupts at 16 x tone frequescy

	noInterrupts();
	timer0_isr_init();
	timer0_attachInterrupt(timer0_handler);
	nexttonetick = ESP.getCycleCount() + LOTONE;
	timer0_write(nexttonetick);
	interrupts();

	Serial.println("Sending calibrate tones");
	transmitting = TRUE;
	delay(10000);
	txtone = HITONE;
	delay(10000);
	transmitting = FALSE;
	Serial.println("Init Complete");
}

int lastticks;

void loop()
{
	if (MSGREADY && transmitting == 0)
	{
		int Len = kissptr - &KISSMsg[1];

		if (Len < 4)
		{
			printf("Control Frame %d %d\n", KISSMsg[0], KISSMsg[1]);
			kissptr = KISSMsg;
			MSGREADY = FALSE;
			return;
		}			
		printf("Sending new frame Len %d\n", Len);
		memcpy(txBuffer, &KISSMsg[1], Len);
		kissptr = KISSMsg;
		MSGREADY = FALSE;
		sendFrame(txBuffer, Len);
		return;				// We only have one input buffer, so don't read from client
	}

	delay(10);
 //   printf("Ticks %d %d\n", ticks, lastticks - ticks);
	lastticks = ticks;
	PollSerial();
	PollWifi();
}

void PollWifi()
{
	uint8_t i;
	
	//check if there are any new clients

	if (server.hasClient())
	{
		if (!serverClient || !serverClient.connected())
		{
			if (serverClient)
				serverClient.stop();
		
			serverClient = server.available();
		
			Serial.print("New client\n");
		}
		else
		{
			//no free/disconnected spot so reject
	
			WiFiClient serverClient = server.available();
			serverClient.stop();
	
		}
	}
		
	//check client for data
	
	if (MSGREADY)
		return;				// We only have one input buffer, so don't read from client

	if (serverClient && serverClient.connected())
	{
		if (serverClient.available())
		{
			//get data from the telnet client and push it to the UART
		
			while(serverClient.available())
			{
				if (ProcessKISSChar(serverClient.read()))
					return;	 // Got a Message
			}
		}
	}
 }

void bitDelay()
{
	// run background until delay has elapsed (uses instructin counter)

	PollSerial();
	PollWifi();

	while ((ESP.getCycleCount() - lastCycleCount) < BIT_DELAY)
	{
		yield(); 
	}
	lastCycleCount += BIT_DELAY;
	return;
}

unsigned int sine_index;	// Index for the D-to-A converter

// We use pins 12-14 and 4 as a 4 bit DAC. Cant use 15 as that has to be held low to boot

int  sine[16] = {
	0x0010,	 //8
	0x3010,	 // 11
	0x5010,	 // 13
	0x7010,	 // 15
	0x7010,	 // 15
	0x7010,	 // 15
	0x5010,	 // 13
	0x3010,	 // 11
	0x0010,	 // 8
	0x5000,
	0x2000,
	0,
	0,
	0,
	0x2000,
	0x5000};

// Timer 0 interrupt handler

//#define ESP8266_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))
//#define GPO	ESP8266_REG(0x300) //GPIO_OUT R/W (Output Level)
//#define GPI	ESP8266_REG(0x318) //GPIO_IN RO (Read Input Level)

unsigned int * GPIORptr = ((uint32_t*) &GPI); //(unsigned int *)0x60000000+0x318; //GPI
unsigned int * GPIOWptr = ((uint32_t*) &GPO); //(unsigned int *)0x60000000+0x300; //GPO

void ICACHE_RAM_ATTR timer0_handler ()
{
	// This interrupt occurs at 16 * tone frequency and generates a reasonable 
	// approximation to a sine wave using a 4 bit resistive DAC attached to 
	// GPIO pins  12 - 14 and 4

//	timer0_write(ESP.getCycleCount() + txtone);
//	timer0_write(ESP.getCycleCount() + 80000000);

	// reprime timer
	
	nexttonetick += txtone;
	timer0_write(nexttonetick);
	ticks++;

	if (transmitting)
	{
		unsigned int pins = *GPIORptr;

		pins &= 0x8fef;				// Mask bits 4, 12-14
		pins |= sine[sine_index++]; // or in next DAV value
		*GPIOWptr = pins;
		sine_index &= 15;			// mask to 4 bit
	}
}

int ones;				// consecutive 1 bits sent (for bit stuffing test)

void sendFrame(unsigned char * frame, int len)
{
	unsigned int loop_delay;
	unsigned char sequential_ones = 0;
	unsigned short crc;
	int i;

	crc = compute_crc(frame, len);
	crc ^= 0xffff;

	if (!transmitting)
	{
		lastCycleCount = ESP.getCycleCount();		// prime bit timer
		transmitting = TRUE;
		digitalWrite(15, HIGH);			 // Set PTT

		// Send flags for TXDelay period

		Serial.println("TXdelay");
		
		for (loop_delay = 0; loop_delay < TXDELAY; loop_delay++)
		{
			sendFlag();
		}
	}

	sendFlag();				// Always send at least one
	ones = 0;
	
	for (i = 0; i < len; i++)
		sendByte(*(frame++));

	//	Send CRC. 

	sendByte(crc & 0xFF);		// Send the low byte of the crc
	sendByte(crc >> 8);			// Send the high byte of the crc
	
	//	Send closing FLAG
	
	sendFlag();					// Send a flag to end the packet

//	See if another frame

	if (MSGREADY)
	{
		int Len = kissptr - &KISSMsg[1];
		printf("Sending another Len %d\n", Len);
		memcpy(txBuffer, &KISSMsg[1], Len);
		kissptr = KISSMsg;
		MSGREADY = FALSE;
		sendFrame(txBuffer, Len);
		return;				// We only have one input buffer, so don't read from client
	}
	transmitting = FALSE;
	digitalWrite(15, LOW);
}

void sendFlag()
{
	// Send zero 
	
	txtone = (txtone == LOTONE) ? HITONE : LOTONE; // Toggle transmit tone
	bitDelay();			// Wait till bit sent

	// send 6 ones (no change)

	bitDelay();			// Wait till bit sent
	bitDelay();			// Wait till bit sent
	bitDelay();			// Wait till bit sent
	bitDelay();			// Wait till bit sent
	bitDelay();			// Wait till bit sent
	bitDelay();			// Wait till bit sent

	// Send zero 

	txtone = (txtone == LOTONE) ? HITONE : LOTONE; // Toggle transmit tone
	bitDelay();			// Wait till bit sent
}

void sendByte(unsigned char txbyte)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (txbyte & 1)			// Sending 1?
		{
			// no change, but need to stuff a zero after 5 ones

			if (++ones == 5) // Is this the 5th "1" in a row?
			{
				bitDelay();			// Wait till bit sent
				txtone = (txtone == LOTONE) ? HITONE : LOTONE; // Toggle transmit tone
				ones = 0;
			}
		}
		else
		{
			// Send zero, so change tone

			ones = 0;
			txtone = (txtone == LOTONE) ? HITONE : LOTONE; // Toggle transmit tone
		}
		txbyte = txbyte >> 1;
		bitDelay();					// wait till bit is sent
	}
}


//	KISS over Serial. This is promarily for testing, as live system is intended to
//	operate over wifi


void PollSerial()
{
	while (Serial.available() > 0)
	{
		if (ProcessKISSChar(Serial.read()))
		return;	 // got a message
	}
}

bool ProcessKISSChar(unsigned char c)
{
	if (ESCFLAG)
	{
		//
		//	FESC received - next should be TFESC or TFEND

		ESCFLAG = FALSE;

		if (c == TFESC)
		c = FESC;

		if (c == TFEND)
		c = FEND;

		// any other leave unchanged

	}
	else
	{
		switch (c)
		{
		case FEND:

			//
			//	Either start of message or message complete
			//

			if (kissptr == KISSMsg)
				return FALSE;			 // start of frame

//			TCPSEND(KISSMsg, kissptr - KISSMsg);

			MSGREADY = TRUE;
			return TRUE;

		case FESC:

			ESCFLAG = TRUE;
			return FALSE;
		}
	}

	//
	//	Ok, a normal char
	//
	
	*(kissptr++) = c;

	if (kissptr == &KISSMsg[359])
		kissptr--;
	
	return FALSE;
}

void TCPSEND(unsigned char * Msg, int Len)
{
	// I dont think there is a function to send an array of bytes

	if (serverClient && serverClient.connected())
	{
		int i;
		unsigned char c;
		unsigned char encoded[1024];
		unsigned char * out = &encoded[0];

		*(out++) = FEND;
	
		for (i=0; i < Len; i++)
		{
			c = *(Msg++);

			switch (c)
			{
			case FEND:
	
				*(out++) = FESC;
				*(out++) = TFEND;
				break;

			case FESC:
			
				*(out++) = FESC;
				*(out++) = TFESC;
				break;

			default:

				*(out++) = c;
			}
		}

		*(out++) = FEND;
		serverClient.write(&encoded[0], encoded - out);
	}
}

// would this be better (faster) in RAM ?? 

const unsigned short CRCTAB[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 
}; 

unsigned short int compute_crc(unsigned char *buf,int len)
{
	unsigned short fcs = 0xffff; 
	int i;

	for(i = 0; i < len; i++) 
		fcs = (fcs >>8 ) ^ CRCTAB[(fcs ^ buf[i]) & 0xff]; 

	return fcs;
}




