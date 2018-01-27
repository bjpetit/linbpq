//
//  Code for Packet using ARDOP like frames.
//	A sort of poor man's Robust Packet
//
// This uses Special Variable Length frames

// Packet has header of 6 bytes  sent in 4PSK.500.100. 
// Header is 4 bits Type 12 Bits Len 2 bytes CRC 2 bytes RS
// Once we have that we receive the rest of the packet in the 
// mode defined in the header.
// Uses Frsame Type 0xC0, symbolic name PktFrameHeader


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <winioctl.h>
#else
#define HANDLE int
#endif

#include "ARDOPC.h"


extern UCHAR KISSBUFFER[500]; // Long enough for stuffed KISS frame
extern int KISSLength;


VOID EncodePacket(UCHAR * Data, int Len);
VOID AddTxByteDirect(UCHAR Byte);
VOID AddTxByteStuffed(UCHAR Byte);
unsigned short int compute_crc(unsigned char *buf,int len);
void PacketStartTX();
BOOL GetNextKISSFrame();
VOID SendAckModeAck();

extern unsigned char bytEncodedBytes[1800];		// I think the biggest is 600 bd 768 + overhead
extern int EncLen;

extern UCHAR PacketMon[360];
extern int PacketMonMore;
extern int PacketMonLength;


int pktNumCar = 4;
int pktMaxCar = 8;
int pktMaxFrame = 4;
int pktPacLen = 80;

int pktMode = 2; // QAM
int pktRXMode;		// Corrently receiving mode

int pktDataLen;
int pktRSLen;
const char pktMod[4][8] = {"4PSK", "8PSK", "16QAM"};
const char pktBW[9][8] = {"", "200", "500", "", "1000", "", "", "", "2000"};


int pktModeLen = 3;


VOID PktARDOPEncode(UCHAR * Data, int Len)
{
	unsigned char DataToSend[4];

	// Create Packet Header.  3 bits Mod Type 3 bits Carriers 10 Bits Len
	// Same is sent on each carrier of 4PSK.500.100 frame for robustness
	// Now just use one carrier (200 Hz) with more FEC

	if (Len > 1023)
		return;

	DataToSend[0] = (pktMode << 5) | ((pktNumCar - 1) << 2) | (Len >> 8);
	DataToSend[1] = Len & 0xff;
	
//	DataToSend[2] = DataToSend[0];
//	DataToSend[3] = DataToSend[1];

	// Calc Data and RS Length

	pktDataLen = (Len + (pktNumCar - 1))/pktNumCar; // Round up

	pktRSLen = pktDataLen >> 2;			// Try 25% for now

	if (pktRSLen & 1)
		pktRSLen++;						// Odd RS bytes no use

	if (pktRSLen < 4)
		pktRSLen = 4;					// At least 4

	// Encode Header
	
	EncLen = EncodePSKData(PktFrameHeader, DataToSend, 2, bytEncodedBytes);
	
	// Encode Data

	EncodePSKData(PktFrameData, Data, Len, &bytEncodedBytes[EncLen]);
	ModPSKDataAndPlay(PktFrameHeader, bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 

}

// Called when link idle to see if any packet frames to send

void PktARDOPStartTX()
{
	if (GetNextKISSFrame() == FALSE)
		return;			// nothing to send
	
	while (TRUE)				// loop till we run out of packets
	{
		switch(KISSBUFFER[0])
		{
		case 0:			// Normal Data

			WriteDebugLog(LOGALERT, "Sending Packet Frame Len %d", KISSLength - 1); 

			PktARDOPEncode(KISSBUFFER + 1, KISSLength - 1);

			// Trace it

			if (PacketMonLength == 0)	// Ingore if one queued
			{
				PacketMon[0] = 0x80;		// TX Flag
				memcpy(&PacketMon[1], &KISSBUFFER[1], KISSLength);
	
				PacketMonLength = KISSLength;
			}

			break;

		case 6:			// HW Paramters. Set Mode and Bandwidth

			pktNumCar = KISSBUFFER[1] >> 4;
			pktMode = KISSBUFFER[1] & 0xF; 
			break;
		
		case 12:
		
		// Ackmode frame. Return ACK Bytes (first 2) to host when TX complete
		
			WriteDebugLog(LOGALERT, "Sending Packet Frame Len %d", KISSLength - 3); 
			PktARDOPEncode(KISSBUFFER + 3, KISSLength - 3);

			// Returns when Complete so can send ACK

			SendAckModeAck();
			break;
		}

		// See if any more

		if (GetNextKISSFrame() == FALSE)
			break;			// no more to send
	}
}

