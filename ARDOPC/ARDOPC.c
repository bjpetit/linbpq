// ARDOPC.cpp : Defines the entry point for the console application.
//

#include "ARDOPC.h"

char GridSquare[7] = "IO68VL";
char Callsign[10] = "G8BPQ-2";
BOOL wantCWID = FALSE;
int LeaderLength = 500;
int TrailerLength = 0;

BOOL SoundIsPlaying = FALSE;


char TXQueue[4096] = "Hello";					// May malloc this, or change to cyclic buffer
int TXQueueLen = 5;

int MaxCorrections;

UCHAR bytSessionID;

int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 

short intTwoToneLeaderTemplate[120];  // holds just 1 symbol (10 ms) of the leader

short intPSK100bdCarTemplate[9][4][120];	// The actual templates over 9 carriers for 4 phase values and 120 samples
    //   (only positive Phase values are in the table, sign reversal is used to get the negative phase values) This reduces the table size from 7680 to 3840 integers
short intPSK200bdCarTemplate[9][4][72];		// Templates for 200 bd with cyclic prefix
short intFSK25bdCarTemplate[16][480];		// Template for 16FSK carriers spaced at 25 Hz, 25 baud
short intFSK50bdCarTemplate[4][240];		// Template for 4FSK carriers spaced at 50 Hz, 50 baud
short intFSK100bdCarTemplate[20][120];		// Template for 4FSK carriers spaced at 100 Hz, 100 baud
short intFSK600bdCarTemplate[4][20];		// Template for 4FSK carriers spaced at 600 Hz, 600 baud  (used for FM only)


const char strFrameType[256][16] = {
	"DataNAK", //  Range &H00 to &H1F includes 5 bits for quality 1 Car, 200Hz,4FSK
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","",

        //Short Control Frames 1 Car, 200Hz,4FSK  ' Reassigned May 22, 2015 for maximum "distance"
 
	"BREAK", "", "",
	"IDLE", "", "",
	"DISC", "", "",
	"END",
	"ConRejBusy",
	"ConRejBW",
	"",

	//Special frames 1 Car, 200Hz,4FSK 0x30 +

	"IDFrame",
	"ConReq200M",
	"ConReq500M",
	"ConReq1000M",
	"ConReq2000M",
	"ConReq200F",
	"ConReq500F",
	"ConReq1000F",
	"ConReq2000F",
	"ConAck200",
	"ConAck500",
	"ConAck1000",
	"ConAck2000",
	// Types &H3D to &H3F reserved
	"","","",
	// 200 Hz Bandwidth Data 
	// 1 Car PSK Data Modes 200 HzBW  100 baud 
	
	"4PSK.200.100.E",	// 0x40
	"4PSK.200.100.O",
	"4PSK.200.100S.E",
	"4PSK.200.100S.O",
	"8PSK.200.100.E",
	"8PSK.200.100.O",

	// 1 Car 4FSK Data mode 200 HzBW, 50 baud 

	"4FSK.200.50.E",
	"4FSK.200.50.O",
	"4FSK.200.50S.E", // 48
	"4FSK.200.50S.O",
	"4FSK.500.100.E",
	"4FSK.500.100.O",
	"4FSK.500.100S.E",
	"4FSK.500.100S.O",
	"8FSK.200.25.E",
	"8FSK.200.25.O",

	//' 2 Car PSK Data Modes 100 baud
	"4PSK.500.100.E", // 50
	"4PSK.500.100.O",
	"8PSK.500.100.E",
	"8PSK.500.100.O",

	// 2 Car Data modes 167 baud  
	"4PSK.500.167.E",
	"4PSK.500.167.O",
	"8PSK.500.167.E",
	"8PSK.500.167.O",

	// 1 Car 16FSK mode 25 baud

	"16FSK.500.25.E", // 58
	"16FSK.500.25.O",
	"16FSK.500.25S.E",
	"16FSK.500.25S.O", "","","","",

	//1 Khz Bandwidth Data Modes 
	//  4 Car 100 baud PSK
	"4PSK.1000.100.E", //60
	"4PSK.1000.100.O",
	"8PSK.1000.100.E",
	"8PSK.1000.100.O",
	// 4 car 167 baud PSK
	"4PSK.1000.167.E",
	"4PSK.1000.167.O",
	"8PSK.1000.167.E",
	"8PSK.1000.167.O",
	// 2 Car 4FSK 100 baud
	"4FSK.1000.100.E", //68
	"4FSK.1000.100.O","","","","","","",

	// 2Khz Bandwidth Data Modes 
	//  8 Car 100 baud PSK
	"4PSK.2000.100.E", //70 
	"4PSK.2000.100.O",
	"8PSK.2000.100.E",
	"8PSK.2000.100.O",
	//  8 Car 167 baud PSK
	"4PSK.2000.167.E",
	"4PSK.2000.167.O",
	"8PSK.2000.167.E",
	"8PSK.2000.167.O",
	// 4 Car 4FSK 100 baud
	"4FSK.2000.100.E",
	"4FSK.2000.100.O",
	// 1 Car 4FSK 600 baud (FM only)
	"4FSK.2000.600.E", // Experimental 
	"4FSK.2000.600.O", // Experimental
	"4FSK.2000.600S.E", // Experimental
	"4FSK.2000.600S.O", // Experimental //7d
	"","",	// 7e-7f
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","", //C0

	//Frame Types &HA0 to &HDF reserved for experimentation 
	"SOUND2K" //D0
	"","","","","","","","","","","","","","","","",
    //Data ACK  1 Car, 200Hz,4FSK
	"DataACK"		// Range &HE0 to &HFF includes 5 bits for quality 
};

void GetSemaphore()
{
}

void FreeSemaphore()
{
}

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}



void CompressCallsign(char * Callsign, UCHAR * Compressed);
void CompressGridSquare(char * Square, UCHAR * Compressed);
void  ASCIIto6Bit(char * Padded, UCHAR * Compressed);
void GetTwoToneLeaderWithSync(int intSymLen, short * intLeader);

void AddTagToDataAndSendToHost(char * Msg, char * Type)
{
}

void SendCWID(char * Callsign, BOOL x)
{
}

// Subroutine to generate 1 symbol of leader

void GenerateTwoToneLeaderTemplate()
{
	// to create leader alternate these template samples reversing sign on each adjacent symbol
    
	int i;
	double x, y, z;
	
	for (i = 0; i < 120; i++)
	{
		y = (sin(((1500.0 - 50) / 1500) * (i / 8.0 * 2 * M_PI)));
		z = (sin(((1500.0 + 50) / 1500) * (i / 8.0 * 2 * M_PI)));

		x = intAmp * 0.6 * (y - z);
		intTwoToneLeaderTemplate[i] = (short)x + 0.5;;
	}
}

// Subroutine to create the FSK symbol templates

void GenerateFSKTemplates()
{
	// Generate templates of 240 samples (each symbol template = 20 ms) for each of the 4 possible carriers used in 200 Hz BW FSK modulation.
	// Generate templates of 120 samples (each symbol template = 10 ms) for each of the 20 possible carriers used in 500, 1000 and 2000 Hz BW 4FSK modulation.
	//Used to speed up computation of FSK frames and reduce use of Sin functions.
	//50 baud Tone values 

	// the possible carrier frequencies in Hz ' note gaps for groups of 4 at 900, 1400, and 1900 Hz improved isolation between simultaneous carriers

	double dblCarFreq[] = {1425, 1475, 1525, 1575, 600, 700, 800, 900, 1100, 1200, 1300, 1400, 1600, 1700, 1800, 1900, 2100, 2200, 2300, 2400};

	double dblAngle;		// Angle in radians
	double dblCarPhaseInc[20]; 
	int i, k;

	// Compute the phase inc per sample

    for (i = 0; i < 4; i++) 
	{
		dblCarPhaseInc[i] = 2 * M_PI * dblCarFreq[i] / 12000;
	}
	
	// Now compute the templates: (960 32 bit values total)   
	
	for (i = 0; i < 4; i++)			// across the 4 tones for 50 baud frequencies
	{
		dblAngle = 0;
		// 50 baud template

		for (k = 0; k < 240; k++)	// for 240 samples (one 50 baud symbol)
		{
			intFSK50bdCarTemplate[i][k] = intAmp * 1.1 * sin(dblAngle);  // with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
			dblAngle += dblCarPhaseInc[i];

			if (dblAngle >= 2 * M_PI)
				dblAngle -= 2 * M_PI;
		}
	}

	// 16 FSK templates (500 Hz BW, 25 baud)

	for (i = 0; i < 16; i++)	 // across the 16 tones for 25 baud frequencies
	{
		dblAngle = 0;
		//25 baud template
		for (k = 0; k < 480; k++)			 // for 480 samples (one 25 baud symbol)
		{
			intFSK25bdCarTemplate[i][k] = intAmp * 1.1 * sin(dblAngle); // with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
			dblAngle += (2 * M_PI / 12000) * (1312.5 + i * 25);
			if (dblAngle >= 2 * M_PI)
				dblAngle -= 2 * M_PI;
		}
	}

	// 4FSK templates for 600 baud (2 Khz bandwidth) 
	for (i = 0; i < 4; i++)		 // across the 4 tones for 600 baud frequencies
	{
		dblAngle = 0;
		//600 baud template
		for (k = 0; k < 20; k++)	 // for 20 samples (one 600 baud symbol)
		{
			intFSK600bdCarTemplate[i][k] = intAmp * 1.1 * sin(dblAngle); // with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
			dblAngle += (2 * M_PI / 12000) * (600 + i * 600);
			if (dblAngle >= 2 * M_PI)
				dblAngle -= 2 * M_PI;
		}
	}

	//  100 baud Tone values for a single carrier case 
	// the 100 baud carrier frequencies in Hz

	dblCarFreq[0] = 1350;
	dblCarFreq[1] = 1450;
	dblCarFreq[2] = 1550;
	dblCarFreq[3] = 1650;

	//Values of dblCarFreq for index 4-19 as in Dim above
	// Compute the phase inc per sample
   
	for (i = 0; i < 20; i++)
	{
		dblCarPhaseInc[i] = 2 * M_PI * dblCarFreq[i] / 12000;
	}

	// Now compute the templates: (2400 32 bit values total)  

	for (i = 0; i < 20; i++)	 // across 20 tones
	{
		dblAngle = 0;
		//'100 baud template
		for (k = 0; k < 120; k++)		// for 120 samples (one 100 baud symbol)
		{
			intFSK100bdCarTemplate[i][k] = intAmp * 1.1 * sin(dblAngle); // with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
			dblAngle += dblCarPhaseInc[i];
			if (dblAngle >= 2 * M_PI)
				dblAngle -= 2 * M_PI;
		}
	}
}


// Function to look up frame info from bytFrameType

BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
			   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType)
{
	//Used to "lookup" all parameters by frame Type. 
	// Returns True if all fields updated otherwise false (improper bytFrameType)

	// 1 Carrier 4FSK control frames 
   
	if (bytFrameType >= 0 &&  bytFrameType <= 0x1F)
	{
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 40;
	}

	switch(bytFrameType)
	{
	case 0x23:
	case 0x26:
	case 0x29:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;
	
	case 0x2C:
	case 0x2D:
	case 0x2E:
		
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	case 0x30:
	case 0x38:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 14;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0x39:
	case 0x3C:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 3;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0xe0:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	// 1 Carrier Data modes
    //  100 baud PSK (200 baud not compatible with 200 Hz bandwidth)  (Note 1 carrier modes Qual Threshold reduced to 30 (was 50) for testing April 20, 2015

	case 0x40:
	case 0x41:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;	
		break;
	
	case 0x42:
	case 0x43:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 8;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
	
	case 0x44:
	case 0x45:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 50 baud 4FSK 200 Hz bandwidth

	case 0x46:
	case 0x47:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;

	case 0x48:
	case 0x49:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 4;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;
	
	// 100 baud 4FSK 500 Hz bandwidth

	case 0x4A:
	case 0x4B:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
 
 	case 0x4C:
	case 0x4D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 25 baud 8FSK 200 Hz bandwidth (in testing)
 
 	case 0x4E:
	case 0x4F:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 24;
		*intRSLen = 6;
		strcpy(strMod, "8FSK");
		*intBaud = 25;
		*bytQualThres = 30; 
		break;
		
	// 25 baud 16FSK 500 Hz bandwidth 

 	case 0x58:
	case 0x59:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "16FSK");
		*intBaud = 25;
		*bytQualThres = 30;
 		break;

	// 600 baud 4FSK 2000 Hz bandwidth 

	case 0x7a:
	case 0x7b:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 600;
		*intRSLen = 150;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	case 0x7C:
	case 0x7D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 200;
		*intRSLen = 50;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	// 2 Carrier Data Modes
	// 100 baud 

	case 0x50:
	case 0x51:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
 		break;

	case 0x52:
	case 0x53:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
						
	// 167 baud testing
    
	case 0x54:
	case 0x55:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
		
	case 0x56:
	case 0x57:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 150;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	//' 4 Carrier Data Modes
	//	100 baud
     	
	case 0x60:
	case 0x61:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
	
	case 0x62:
	case 0x63:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	//	167 Baud

	case 0x64:
	case 0x65:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	case 0x66:
	case 0x67:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
	
	// 100 baud 2 carrier 4FSK
     
	case 0x68:
	case 0x69:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 8 Carrier Data modes
	//	100 baud

	case 0x70:
	case 0x71:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	case 0x72:
	case 0x73:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 167 baud

	case 0x74:
	case 0x75:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
		
	case 0x76:
	case 0x77:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	// 100 baud 4 carrier 4FSK
 
	case 0x78:
	case 0x79:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	default:
		//'Logs.Exception("[PSKDataInfo] No data for frame type= H" & Format(bytFrameType, "x"))
        return FALSE;
	}
	
	if (bytFrameType >= 0 && bytFrameType <= 0x1F)
		strcpy(strType,strFrameType[0]);
	else
		if (bytFrameType >= 0xE0) 
			strcpy(strType,strFrameType[0xE0]);
        else
			strcpy(strType,strFrameType[bytFrameType]);

	return TRUE;
}

void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK and create the integer array of 32 bit samples suitable for playing 
	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;
	short intLeader[100000];
	short intSamples[100000];

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;
	UCHAR bytLastSym[8];	// Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 

	double dblCarScalingFactor;
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "4FSK") != 0)
		return;
	
//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If


	if (intLeaderLen == 0)
		intLeaderLenMS = LeaderLength;
	else
		intLeaderLenMS = intLeaderLen;


 //           Dim intLeader(-1) As Int32
    switch(intBaud)
	{		
	case 50:
		
		intSampPerSym = 240;
        // For FEC transmission the computed leader length = MCB.Leader length    
		intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (Len - 2) / intNumCar);

		GetTwoToneLeaderWithSync(intLeaderLenMS / 10, &intLeader[0]);
		SampleSink(intLeader, intLeaderLenMS * 12);	// (500 ms 2400

		 break;
                
	case 100:
		
		intSampPerSym = 120;
		intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (Len - 2) / intNumCar);
		GetTwoToneLeaderWithSync(intLeaderLenMS / 10, &intLeader[0]);
		SampleSink(intLeader, intLeaderLenMS * 12);	// (500 ms 2400
	}
	
	// Create the leader
	
	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below
    
	//Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
     
	intSamplePtr = 0;

	//Create the 8 symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
	// No reference needed for 4FSK

	// note revised To accomodate 1 parity symbol per byte (10 symbols total)

	for(j = 0; j < 2; j++)		 // for the 2 bytes of the frame type
	{              
		bytMask = 0xc0;
		
		for(k = 0; k < 5; k++)		 // for 5 symbols per byte (4 data + 1 parity)
		{
			if (k < 4)
				bytSymToSend = (bytMask & bytEncodedData[j]) >> (2 * (3 - k));
			else
				bytSymToSend = ComputeTypeParity(bytEncodedData[0]);

			for(n = 0; n < 240; n++)
			{
				if (((5 * j + k) & 1 ) == 0)
					intSamples[intSamplePtr + n] = intFSK50bdCarTemplate[bytSymToSend][n];
				else
					intSamples[intSamplePtr + n] = -intFSK50bdCarTemplate[bytSymToSend][n]; // -sign insures no phase discontinuity at symbol boundaries
			}
			intSamplePtr += 240;
			bytMask = bytMask >> 2;
		}
	}

	//' No reference needed for 4FSK

	intDataPtr = 2;

	switch(intNumCar)
	{
	case 1:			 // use carriers 0-4
		
		dblCarScalingFactor = 1.0; //  (scaling factors determined emperically to minimize crest factor) 

		for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data
		{
			bytMask = 0xC0;		 // Initialize mask each new data byte
			
			for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
			{
				bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (2 * (3 - k)); // Values 0-3

				for (n = 0; n < intSampPerSym; n++)	 // Sum for all the samples of a symbols 
				{
					if((k & 1) == 0)
					{
						if(intBaud == 50)
							intSamples[intSamplePtr + n] = intFSK50bdCarTemplate[bytSymToSend][n];
						else
							intSamples[intSamplePtr + n] = intFSK100bdCarTemplate[bytSymToSend][n];
					}
					else
 					{
						if(intBaud == 50)
							intSamples[intSamplePtr + n] = -intFSK50bdCarTemplate[bytSymToSend][n];
						else
							intSamples[intSamplePtr + n] = -intFSK100bdCarTemplate[bytSymToSend][n];
					}
				}

				bytMask = bytMask >> 2;
				intSamplePtr += intSampPerSym;
			}
			intDataPtr += 1;
		}

		if (intBaud == 50)
			FSXmtFilter200_1500Hz(intSamples, intSamplePtr);
		else if (intBaud == 100)
			FSXmtFilter500_1500Hz(intSamples, intSamplePtr);

		SampleSink(intSamples, intSamplePtr);	// 2400
		SoundFlush();

		break;

	case 2:			// use carriers 8-15 (100 baud only)

		dblCarScalingFactor = 0.51; //  (scaling factors determined emperically to minimize crest factor)

		for (m = 0; m < intDataBytesPerCar; m++)	  // For each byte of input data 
		{
			bytMask = 0xC0;	// Initialize mask each new data byte
                        			
			for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
			{
				for (n = 0; n < intSampPerSym; n++)	 // for all the samples of a symbol for 2 carriers
				{
					//' First carrier
                      
					bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
					intSamples[intSamplePtr + n] = intFSK100bdCarTemplate[8 + bytSymToSend][n];
					// Second carrier
                    
					bytSymToSend = (bytMask & bytEncodedData[intDataPtr + intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSamples[intSamplePtr + n] = dblCarScalingFactor * (intSamples[intSamplePtr + n] + intFSK100bdCarTemplate[12 + bytSymToSend][n]);
				}

				bytMask = bytMask >> 2;
				intSamplePtr += intSampPerSym;
			}
			intDataPtr += 1;
		}
             
//			intSamples = FSXmtFilter1000_1500Hz(intSamples)
		SampleSink(intSamples, intSamplePtr);	// 2400
		SoundFlush();

		break;

				/*
		Case 4 ' use carriers 4-19 (100 baud only)
                    dblCarScalingFactor = 0.27 '  (scaling factors determined emperically to minimize crest factor)
                    For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
                        bytMask = &HC0 ' Initialize mask each new data byte
                        For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                            For n As Integer = 0 To intSampPerSym - 1 ' for all the samples of a symbol for 4 carriers 
                                ' First carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = intFSK100bdCarTemplate(4 + bytSymToSend, n)
                                ' Second carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr + intDataBytesPerCar)) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) + intFSK100bdCarTemplate(8 + bytSymToSend, n)
                                ' Third carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr + (2 * intDataBytesPerCar))) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) + intFSK100bdCarTemplate(12 + bytSymToSend, n)
                                ' Fourth carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr + (3 * intDataBytesPerCar))) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = dblCarScalingFactor * (intSamples(intSamplePtr + n) + intFSK100bdCarTemplate(16 + bytSymToSend, n))
                            Next n
                            bytMask = bytMask >> 2
                            intSamplePtr += intSampPerSym
                        Next k
                        intDataPtr += 1
                    Next m
                    intSamples = FSXmtFilter2000_1500Hz(intSamples)
  */
	}
}

#define mm  8           /* RS code over GF(2**4) - change to suit */
#define nn  255          /* nn=2**mm -1   length of codeword */

extern int tt, kk;
extern int data[];
extern int bb[];

int RSEncode(UCHAR * bytToRS, UCHAR * bytRSEncoded, int MaxErr, int Len)
{
	int Start, i;
	int RSLen;

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	tt = MaxErr;		// number of errors that can be corrected 
    kk = nn - 2 * tt;	// the number of information characters  kk = nn-2*tt

	RSLen = 2 * tt;

	generate_gf();

	/* compute the generator polynomial for this RS code */
	
	gen_poly();

	// Copy the suplied data to end of data array. Len of array is kk

	Start = kk - Len;

	if (Len > kk)
		return 0;

	for(i = 0; i < Len; i++)
	{
		data[i + Start] = bytToRS[i];
	}
 
	encode_rs();

	// Return original + RS Code
	
	memcpy(bytRSEncoded, bytToRS, Len);

	for (i = 0; i < RSLen; i++)
	{
		bytRSEncoded[Len++] = bb[i];
	}
	return Len + RSLen;
}


// Function to encode data for all FSK frame types
  
int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedData)
{
	// Objective is to use this to use this to send all 4FSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 50, 100
	//    strMod is 4FSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, intDataToSendPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[16];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int EncLen;				// Length of encoded data
	int i;
	UCHAR bytToRS[256];
	UCHAR bytRSEncoded[256];


	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}
	
	//strFrameName = strType;

	//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)
 
	EncLen = (2 + intNumCar * (intDataLen + intRSLen + 1 + 2));
	
	//	Generate the 2 bytes for the frame type data:
	
	bytEncodedData[0] = bytFrameType;
	bytEncodedData[1] = bytFrameType ^ bytSessionID;

     //   Dim bytToRS(intDataLen + 3 - 1) As Byte ' Data + Count + 2 byte CRC

	intDataToSendPtr = 0;
	intEncodedDataPtr = 2;
	MaxCorrections = intRSLen / 2;  // RS length must be even

	if (intBaud < 600 || intDataLen < 600)
	{
		// Now compute the RS frame for each carrier in sequence and move it to bytEncodedData 
		
		for (i = 0; i < intNumCar; i++)		//  across all carriers
		{
			intCarDataCnt = Length - intDataToSendPtr;
			
			if (intCarDataCnt >= intDataLen)
			{
				// Will all fit ???

				bytToRS[0] = intDataLen;
				intStartIndex = intEncodedDataPtr;
				memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intDataLen);
				intDataToSendPtr += intDataLen;
			}
			else
			{
				bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

				if (intCarDataCnt > 0)
				{
					memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intCarDataCnt);
                    intDataToSendPtr += intCarDataCnt;
				}	
			}
		
			GenCRC16FrameType(bytToRS, 0, intDataLen, bytFrameType); // calculate the CRC on the byte count  data bytes

			EncLen = RSEncode(bytToRS, bytRSEncoded, MaxCorrections, intDataLen + 3);  // Generate the RS encoding ...now 14 bytes total
        
			memcpy(&bytEncodedData[intEncodedDataPtr], bytRSEncoded, EncLen);
 
			intEncodedDataPtr += EncLen;
		}
	}
/*
	// special case for 600 baud 4FSK which has 600 byte data field sent as three sequencial (200 byte + 50 byte RS) groups

	ReDim bytEncodedData((2 + intDataLen + intRSLen + 9) - 1) ' handles 3 groups of data with independed count and CRC
            bytEncodedData(0) = bytFrameType
            bytEncodedData(1) = bytFrameType Xor stcConnection.bytSessionID
            objRS8.MaxCorrections = intRSLen \ 6
            For i As Integer = 0 To 2 ' for three blocks of RS data
                ReDim bytToRS(intDataLen \ 3 + 3 - 1)
                intCarDataCnt = bytDataToSend.Length - intDataToSendPtr
                If intCarDataCnt >= intDataLen \ 3 Then
                    bytToRS(0) = intDataLen \ 3
                    Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intDataLen \ 3)
                    intDataToSendPtr += intDataLen \ 3
                Else
                    bytToRS(0) = bytDataToSend.Length - intDataToSendPtr
                    Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, bytDataToSend.Length - intDataToSendPtr)
                    intDataToSendPtr = intDataLen
                End If
                GenCRC16FrameType(bytToRS, 0, intDataLen \ 3, CByte(bytFrameType)) 'calculate the CRC on the byte count  data bytes
                bytToRS = objRS8.RSEncode(bytToRS)
                Array.Copy(bytToRS, 0, bytEncodedData, intEncodedDataPtr, bytToRS.Length)
                intEncodedDataPtr += bytToRS.Length
            Next i
            Return bytEncodedData
  */
	return intEncodedDataPtr;
}
	

int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytReturn)
{
	// Function to encodes ID frame 
	// Returns length of encoded message 

	UCHAR bytToRS[16];
	UCHAR bytRSEncoded[16];

	 // If (Not CheckValidCallsignSyntax(strMyCallsign)) Then
     //       Logs.Exception("[EncodeModulate.EncodeIDFrame] Illegal Callsign syntax or Gridsquare length. MyCallsign = " & strMyCallsign & ", Gridsquare = " & strGridSquare)
      //      Return Nothing

//	Dim bytReturn(2 + 6 + 6 + 2 - 1) As Byte ' 4 Frame type bytes + 6 Call sign (compressed)  + 8 Grid square bytes ( compressed)  +  2 bytes CRC 

	bytReturn[0] = 0x30;

	//	strFilename = objFrameInfo.Name(bytReturn(0)) & "_" & strMyCallsign.Trim.ToUpper & " [" & strGridSquare.Trim & "]"

	bytReturn[1] = bytReturn[0] ^ 0xFF;

	// Modified May 9, 2015 to use RS instead of 2 byte CRC.
       
	CompressCallsign(Callsign, &bytToRS[0]);

    if (Square[0])
		CompressGridSquare(Square, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	MaxCorrections = 1;	 // with two Parity bytes can correct 1 error. 

	RSEncode(bytToRS, bytRSEncoded, MaxCorrections, 12);  // Generate the RS encoding ...now 14 bytes total
        
	memcpy(&bytReturn[2], bytRSEncoded, 14);

	return 16;
}

void SendID(BOOL blnEnableCWID)
{
	unsigned char bytEncodedBytes[16];
	unsigned char bytIDSent[80];
	int Len;

	// Schedular needs to ensure this isnt called if already playing

	if (SoundIsPlaying)
		return;

    if (GridSquare[0] == 0)
	{
		Len = Encode4FSKIDFrame(Callsign, "No GS", bytEncodedBytes);
		sprintf(bytIDSent," %s:[No GS] ", Callsign);
	}
	else
	{
		Len = Encode4FSKIDFrame(Callsign, GridSquare, bytEncodedBytes);
		sprintf(bytIDSent," %s:[%s] ", Callsign, GridSquare);
	}

	AddTagToDataAndSendToHost(bytIDSent, "IDF");

	// On embedded platforms we don't have the memory to create full sound stream before playiong,
	// so this is structured differently from Rick's code

	Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent

    if (blnEnableCWID)
		sendCWID(Callsign, FALSE);

}

// Function to generate a 5 second burst of two tone (1450 and 1550 Hz) used for setting up drive level
 
void ModTwoToneTest()
{
	short intLeader[100000];
	GetTwoToneLeaderWithSync(500, &intLeader[0]);
	SampleSink(intLeader, 60000);	// 5 secs
	SoundFlush();
}


//  Subroutine to send 5 seconds of two tone

void Send5SecTwoTone()
{
	if (!SoundIsPlaying)
	{
//		CreateWaveStream(ModTwoToneTest());
	}
}

// Function to compress callsign (up to 7 characters + optional "-"SSID   (-0 to -15 or -A to -Z) 
    
void CompressCallsign(char * Callsign, UCHAR * Compressed)
{
	char * Dash = strchr(Callsign, '-');
	char Padded[16];
	int SSID;

	if (Dash == 0)		// if No SSID
	{
		strcpy(Padded, Callsign);
		strcat(Padded, "    ");
		Padded[7] = '0';			//  "0" indicates no SSID
	}
	else
	{
		*(Dash++) = 0;
		SSID = atoi(Dash);

		strcpy(Padded, Callsign);
		strcat(Padded, "    ");

		if (SSID >= 10)		// ' handles special case of -10 to -15 : ; < = > ? '
			Padded[7] = ':' + *(Dash) - 10;
		else
			Padded[7] = *(Dash);
	}

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to compress Gridsquare (up to 8 characters)

void CompressGridSquare(char * Square, UCHAR * Compressed)
{
	char Padded[17];
        
	if (strlen(Square) > 8)
		return;

	strcpy(Padded, Square);
	strcat(Padded, "        ");

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

/*
 > 8 Then Return Nothing
        Dim bytReturn() As Byte = ASCIIto6Bit(GetBytes(strGS.PadRight(8))) ' compress to 6 bit ascii (upper case)
        Return bytReturn ' 6 bytes total
    End Function  ' CompressGridSquare

    ' Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
    Public Function DeCompressCallsign(bytCallsign() As Byte) As String
        If IsNothing(bytCallsign) Then Return ""
        If bytCallsign.Length <> 6 Then Return ""
        Dim bytTest() As Byte = Bit6ToASCII(bytCallsign)
        Dim strWithSSID As String
        If bytTest(7) = 48 Then ' Value of "0" so No SSID
            ReDim Preserve bytTest(6)
            Return GetString(bytTest).Trim
        ElseIf (bytTest(7) >= 58 And bytTest(7) <= 63) Then ' handles special case for -10 to -15
            DeCompressCallsign = GetString(bytTest).Substring(0, 7).Trim & "-"
            DeCompressCallsign &= (bytTest(7) - 48).ToString
        Else
            strWithSSID = GetString(bytTest)
            Return strWithSSID.Substring(0, 7).Trim & "-" & strWithSSID.Substring(strWithSSID.Length - 1)
        End If
    End Function ' DeCompressCallsign

    ' Function to decompress 6 byte Grid square to 4, 6 or 8 characters
    Public Function DeCompressGridSquare(bytGS() As Byte) As String
        If IsNothing(bytGS) Then Return ""
        If bytGS.Length <> 6 Then Return ""
        Dim bytTest() As Byte = Bit6ToASCII(bytGS)
        Return GetString(bytTest).Trim
    End Function ' DeCompressGridSquare

*/

void  ASCIIto6Bit(char * Padded, UCHAR * Compressed)
{
	// Input must be 8 bytes which will convert to 6 bytes of packed 6 bit characters and
	// inputs must be the ASCII character set values from 32 to 95....
    
	unsigned long long intSum = 0;

	int i;

	for (i=0; i<4; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[0] = (UCHAR)(intSum >> 16) & 255;
	Compressed[1] = (UCHAR)(intSum >> 8) &  255;
	Compressed[2] = (UCHAR)intSum & 255;

	intSum = 0;

	for (i=4; i<8; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[3] = (UCHAR)(intSum >> 16) & 255;
	Compressed[4] = (UCHAR)(intSum >> 8) &  255;
	Compressed[5] = (UCHAR)intSum & 255;
}

/*


void Bit6ToASCII(char * Padded, UCHAR * Compressed)
{
; //compress to 8 6 bit characters   6 bytes total

        ' Input must be 6 bytes which represent packed 6 bit characters that well 
        ' result will be 8 ASCII character set values from 32 to 95...

        Dim intSum As Int64
        Dim intMask As Int64
        Dim bytReturn(7) As Byte

        For i As Integer = 0 To 2
            intSum = 256 * intSum + byt6Bit(i)
        Next i
        For j As Integer = 0 To 3
            intMask = CInt(63 * (64 ^ (3 - j)))
            bytReturn(j) = CByte(32 + (intSum And intMask) \ CInt((64 ^ (3 - j))))
        Next
        For i As Integer = 0 To 2
            intSum = 256 * intSum + byt6Bit(i + 3)
        Next i
        For j As Integer = 0 To 3
            intMask = CInt(63 * (64 ^ (3 - j)))
            bytReturn(j + 4) = CByte(32 + (intSum And intMask) \ CInt((64 ^ (3 - j))))
        Next
        Return bytReturn
    End Function
*/

// Function to generate the Two-tone leader and Frame Sync (used in all frame types) 

void GetTwoToneLeaderWithSync(int intSymLen, short * intLeader)
{
	// Generate a 100 baud (10 ms symbol time) 2 tone leader 
	// leader tones used are 1450 and 1550 Hz.  
  
	int intPtr = 0;
	int intSign = 1;
	int i, j;

    if ((intSymLen & 1) == 1) 
		intSign = -1;

	for (i = 0; i < intSymLen; i++)   //for the number of symbols needed (two symbols less than total leader length) 
	{
		for (j = 0; j < 120; j++)	// for 120 samples per symbol (100 baud) 
		{
           if (i != (intSymLen - 1)) 
			   intLeader[intPtr] = intSign * intTwoToneLeaderTemplate[j];
		   else
			   intLeader[intPtr] = -intSign * intTwoToneLeaderTemplate[j];
   
		   intPtr += 1;
		}
		intSign = -intSign;
	}

}

// Function to apply 200 Hz filter for transmit  

void FSXmtFilter200_1500Hz(short * intNewSamples, int Length)
{
	// Used for PSK 200 Hz modulation XMIT filter  
	// assumes sample rate of 12000
	// implements 3 100 Hz wide sections centered on 1500 Hz  (~200 Hz wide @ - 30dB centered on 1500 Hz)
	// FSF (Frequency Selective Filter) variables

	static double dblR = 0.9995;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static double dblRn;

	static double dblR2;
	static double dblCoef[19] = {0.0};			// the coefficients
	double dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	double dblZout_0[19] = {0.0};	// resonator outputs
	double dblZout_1[19] = {0.0};	// resonator outputs delayed one sample
	double dblZout_2[19] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j;

	double intFilteredSample = 0;			//  Filtered sample
	double largest = 0;

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);


	Length = AddTrailer(intNewSamples, Length);  // add the trailer before filtering

	// Initialize the coefficients

	if (dblCoef[15] == 0.0)
	{
		for (i = 14; i <= 16; i++)
		{
			dblCoef[i] = 2 * dblR * cos(2 * M_PI * i / intN); // For Frequency = bin i
		}
	}

	for (i = 0; i < Length + intFilLen - 1; i++)
	{
		intFilteredSample = 0;
		if (i < intN)
			dblZin = intNewSamples[i];
		else if (i < Length)
			dblZin = intNewSamples[i] - dblRn * intNewSamples[i - intN];
		else
			dblZin = -dblRn * intNewSamples[i - intN];
 
		//Compute the Comb

		dblZComb = dblZin - dblZin_2 * dblR2;
		dblZin_2 = dblZin_1;
		dblZin_1 = dblZin;

		// Now the resonators
		
		for (j = 14; j <= 16; j++)	   // calculate output for 3 resonators 
		{
			dblZout_0[j] = dblZComb + dblCoef[j] * dblZout_1[j] - dblR2 * dblZout_2[j];
			dblZout_2[j] = dblZout_1[j];
			dblZout_1[j] = dblZout_0[j];

			// scale each by transition coeff and + (Even) or - (Odd) 

			if (i >= intFilLen)
			{
              if (j == 14 || j == 16)
				  intFilteredSample += 0.7389 * dblZout_0[j];
			  else
				  intFilteredSample -= dblZout_0[j];
			}
		}

		if (i >= intFilLen)
		{
			intFilteredSample = intFilteredSample * 0.00833333333; //  rescales for gain of filter
			if (intFilteredSample > 32700)  // Hard clip above 32700
				intFilteredSample = 32700;
			else if (intFilteredSample < -32700)
				intFilteredSample = -32700;

			intNewSamples[i - intFilLen] = (short)intFilteredSample; // & 0xfff0;
			largest = max(largest, intFilteredSample);
		}
	}
}
	
// Function to apply 500 Hz filter for transmit 
void FSXmtFilter500_1500Hz(short * intNewSamples, int Length)
{
	// Used for FSK modulation XMIT filter  
	// assumes sample rate of 12000
	// implements 7 100 Hz wide sections centered on 1500 Hz  (~500 Hz wide @ - 30dB centered on 1500 Hz)
	// FSF (Frequency Selective Filter) variables
 
	static double dblR = 0.9995;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static double dblRn;

	static double dblR2;
	static double dblCoef[19] = {0.0};			// the coefficients
	double dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	double dblZout_0[19] = {0.0};	// resonator outputs
	double dblZout_1[19] = {0.0};	// resonator outputs delayed one sample
	double dblZout_2[19] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j;

	double intFilteredSample = 0;			//  Filtered sample

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);

	Length = AddTrailer(intNewSamples, Length);  // add the trailer before filtering

	// Initialize the coefficients

	if (dblCoef[18] == 0.0)
	{
		for (i = 12; i <= 18; i++)
		{
			dblCoef[i] = 2 * dblR * cos(2 * M_PI * i / intN); // For Frequency = bin i
		}
	}

  
	for (i = 0; i < Length + intFilLen - 1; i++)
	{
		intFilteredSample = 0;

		if (i < intN)
			dblZin = intNewSamples[i];
		else if (i < Length)
			dblZin = intNewSamples[i] - dblRn * intNewSamples[i - intN];
		else
			dblZin = -dblRn * intNewSamples[i - intN];
 
		//Compute the Comb

		dblZComb = dblZin - dblZin_2 * dblR2;
		dblZin_2 = dblZin_1;
		dblZin_1 = dblZin;	
				 
		// Now the resonators
		
		for (j = 12; j <= 18; j++)	   // calculate output for 3 resonators 
		{
			dblZout_0[j] = dblZComb + dblCoef[j] * dblZout_1[j] - dblR2 * dblZout_2[j];
			dblZout_2[j] = dblZout_1[j];
			dblZout_1[j] = dblZout_0[j];

							
			// scale each by transition coeff and + (Even) or - (Odd) 
			// Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
			// practical range of scaling .05 to .25
			// Scaling also accomodates for the filter "gain" of approx 60. 

			if (i >= intFilLen)
			{
				if (j == 12 || j == 18)
					intFilteredSample += 0.10601 * dblZout_0[j];
                        else if (j == 13 || j == 17)
							intFilteredSample -= 0.59383 * dblZout_0[j];
                        else if ((j & 1) == 0) 
                            intFilteredSample += dblZout_0[j];
                        else
                            intFilteredSample -= dblZout_0[j];
			}
		}     
         
		if (i >= intFilLen)
		{
			intFilteredSample = intFilteredSample * 0.00833333333; //  rescales for gain of filter
			if (intFilteredSample > 32700)  // Hard clip above 32700
				intFilteredSample = 32700;
			else if (intFilteredSample < -32700)
				intFilteredSample = -32700;

			intNewSamples[i - intFilLen] = (short)intFilteredSample;
		}
	}
}

/*
    ' Function to apply 1000 Hz filter for transmit 
    Public Function FSXmtFilter1000_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for FSK modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 11 100 Hz wide sections centered on 1500 Hz  (~1000 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables
        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(20) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(20) As Double ' resonator outputs
        Dim dblZout_1(20) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(20) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        'Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting
        Dim intPeakSample As Int32 = 0
        ' Initialize the coefficients
        If dblCoef(20) = 0 Then
            For i As Integer = 10 To 20
                dblCoef(i) = 2 * dblR * Cos(2 * PI * i / intN) ' For Frequency = bin i
            Next i
        End If
        Try
            For i As Integer = 0 To intNewSamples.Length + intFilLen - 1

                If i < intN Then
                    ' dblUnfilteredSamples(i) = intNewSamples(i) ' debug code for waveform plotting.
                    dblZin = intNewSamples(i)
                ElseIf i < intNewSamples.Length Then
                    'dblUnfilteredSamples(i) = intNewSamples(i) ' debug code for waveform plotting.
                    dblZin = intNewSamples(i) - dblRn * intNewSamples(i - intN)
                Else
                    dblZin = -dblRn * intNewSamples(i - intN)
                End If
                ' Compute the Comb
                dblZComb = dblZin - dblZin_2 * dblR2
                dblZin_2 = dblZin_1
                dblZin_1 = dblZin

                ' Now the resonators

                For j As Integer = 10 To 20   ' calculate output for 11 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 
                    ' Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
                    ' practical range of scaling .05 to .25
                    ' Scaling also accomodates for the filter "gain" of approx 60. 
                    If i >= intFilLen Then
                        If j = 10 Or j = 20 Then
                            intFilteredSamples(i - intFilLen) += 0.389 * dblZout_0(j)
                        ElseIf j Mod 2 = 0 Then
                            intFilteredSamples(i - intFilLen) += dblZout_0(j)
                        Else
                            intFilteredSamples(i - intFilLen) -= dblZout_0(j)
                        End If
                    End If
                Next j
                If i >= intFilLen Then
                    intFilteredSamples(i - intFilLen) = intFilteredSamples(i - intFilLen) * 0.00833333333 '  rescales for gain of filter
                    If intFilteredSamples(i - intFilLen) > 32700 Then ' Hard clip above 32700
                        intFilteredSamples(i - intFilLen) = 32700
                    ElseIf intFilteredSamples(i - intFilLen) < -32700 Then
                        intFilteredSamples(i - intFilLen) = -32700
                    End If
                    'dblFilteredSamples(i - intFilLen) = intFilteredSamples(i - intFilLen) ' debug code
                    If Abs(intFilteredSamples(i - intFilLen)) > Abs(intPeakSample) Then intPeakSample = intFilteredSamples(i - intFilLen)
                End If
            Next i
            ' *********************************
            ' Debug code to look at filter output
            'Dim objWT As New WaveTools
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered" & strFilename, 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered" & strFilename, 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[Filters.FSXmtFilterFSK500_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
    End Function ' FSXmtFilter1000_1500Hz

    ' Function to apply 2000 Hz filter for transmit 
    Public Function FSXmtFilter2000_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for FSK modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 21 100 Hz wide sections centered on 1500 Hz  (~2000 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables

        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(25) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(25) As Double ' resonator outputs
        Dim dblZout_1(25) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(25) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        ' Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting
        Dim intPeakSample As Int32 = 0
        ' Initialize the coefficients
        If dblCoef(25) = 0 Then
            For i As Integer = 5 To 25
                dblCoef(i) = 2 * dblR * Cos(2 * PI * i / intN) ' For Frequency = bin i
            Next i
        End If
        Try
            For i As Integer = 0 To intNewSamples.Length + intFilLen - 1

                If i < intN Then
                    'dblUnfilteredSamples(i) = intNewSamples(i) ' debug code for waveform plotting.
                    dblZin = intNewSamples(i)
                ElseIf i < intNewSamples.Length Then
                    'dblUnfilteredSamples(i) = intNewSamples(i) ' debug code for waveform plotting.
                    dblZin = intNewSamples(i) - dblRn * intNewSamples(i - intN)
                Else
                    dblZin = -dblRn * intNewSamples(i - intN)
                End If
                ' Compute the Comb
                dblZComb = dblZin - dblZin_2 * dblR2
                dblZin_2 = dblZin_1
                dblZin_1 = dblZin

                ' Now the resonators

                For j As Integer = 5 To 25  ' calculate output for 21 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 
                    ' Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
                    ' practical range of scaling .05 to .25
                    ' Scaling also accomodates for the filter "gain" of approx 60. 
                    If i >= intFilLen Then
                        If j = 5 Or j = 25 Then
                            intFilteredSamples(i - intFilLen) -= 0.389 * dblZout_0(j)
                        ElseIf j Mod 2 = 0 Then
                            intFilteredSamples(i - intFilLen) += dblZout_0(j)
                        Else
                            intFilteredSamples(i - intFilLen) -= dblZout_0(j)
                        End If
                    End If
                Next j
                If i >= intFilLen Then
                    intFilteredSamples(i - intFilLen) = intFilteredSamples(i - intFilLen) * 0.00833333333 '  rescales for gain of filter
                    If intFilteredSamples(i - intFilLen) > 32700 Then ' Hard clip above 32700
                        intFilteredSamples(i - intFilLen) = 32700
                    ElseIf intFilteredSamples(i - intFilLen) < -32700 Then
                        intFilteredSamples(i - intFilLen) = -32700
                    End If
                    'dblFilteredSamples(i - intFilLen) = intFilteredSamples(i - intFilLen) ' debug code
                    If Abs(intFilteredSamples(i - intFilLen)) > Abs(intPeakSample) Then intPeakSample = intFilteredSamples(i - intFilLen)
                End If
            Next i
            ' *********************************
            ' Debug code to look at filter output
            'Dim objWT As New WaveTools
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered2000Hz.wav", 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered2000Hz.wav", 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.FSXmitFilterFSK2000_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
    End Function  'FSXmtFilter2000_1500Hz
*/

// A function to compute the parity symbol used in the frame type encoding

UCHAR ComputeTypeParity(UCHAR bytFrameType)
{
	UCHAR bytMask = 0xC0;
	UCHAR bytParitySum = 1;
	UCHAR bytSym = 0;
	int k;

	for (k = 0; k < 4; k++)
	{
		bytSym = (bytMask & bytFrameType) >> (2 * (3 - k));
		bytParitySum = bytParitySum ^ bytSym;
		bytMask = bytMask >> 2;
	}
    
	return bytParitySum & 0x3;
}

// Subroutine to add trailer before filtering

int AddTrailer(short * intSamples, int Length)
{
	int intPtr = Length;
	int intAddedSymbols = 1 + TrailerLength / 10; // add 1 symbol + 1 per each 10 ms of MCB.Trailer
	int i, k;

	for (i = 1; i <= intAddedSymbols; i++)
	{
		for (k = 0; k < 120; k++)
		{
			intSamples[intPtr] = intPSK100bdCarTemplate[4][0][k];
			intPtr += 1;
		}
	}
	return intPtr;
}

// Subroutine to make a CW ID Wave File

void sendCWID(char * Call, BOOL Play)
{
}

void CWID(char * strID, short * intSamples, BOOL blnPlay)
{
	// This generates a phase synchronous FSK MORSE keying of strID
	// FSK used to maintain VOX on some sound cards
	// Sent at 90% of  max ampllitude

    char strAlphabe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/";

	// Look up table for strAlphabet...each bit represents one dot time, 3 adjacent dots = 1 dash
	// one dot spacing between dots or dashes
/*
        Dim intCW() As Integer = {&H17, &H1D5, &H75D, &H75, &H1, &H15D, _
           &H1DD, &H55, &H5, &H1777, &H1D7, &H175, _
           &H77, &H1D, &H777, &H5DD, &H1DD7, &H5D, _
           &H15, &H7, &H57, &H157, &H177, &H757, _
           &H1D77, &H775, &H77777, &H17777, &H5777, &H1577, _
           &H557, &H155, &H755, &H1DD5, &H7775, &H1DDDD, &H1D57, &H1D57}

        If strID.IndexOf("-") <> -1 Then
            ' strip off the -ssid for CWID
            strID = strID.Substring(0, strID.IndexOf("-"))
        End If

        Dim dblHiPhaseInc As Double = 2 * PI * 1609.375 / 12000 ' 1609.375 Hz High tone
        Dim dblLoPhaseInc As Double = 2 * PI * 1390.625 / 12000 ' 1390.625  low tone
        Dim dblHiPhase As Double
        Dim dblLoPhase As Double
        Dim intDotSampCnt As Integer = 768 ' about 12 WPM or so (should be a multiple of 256
        Dim intDot(intDotSampCnt - 1) As Int16
        Dim intSpace(intDotSampCnt - 1) As Int16

        ' Generate the dot samples (high tone) and space samples (low tone) 
        For i As Integer = 0 To intDotSampCnt - 1
            intSpace(i) = CShort(Sin(dblLoPhase) * 0.9 * intAmp)
            intDot(i) = CShort(Sin(dblHiPhase) * 0.9 * intAmp)
            dblHiPhase += dblHiPhaseInc
            If dblHiPhase > 2 * PI Then dblHiPhase -= 2 * PI
            dblLoPhase += dblLoPhaseInc
            If dblLoPhase > 2 * PI Then dblLoPhase -= 2 * PI
        Next i
        Dim intMask As Int32
        Dim intWav((6 * intDotSampCnt) - 1) As Int32
        ' Generate leader for VOX 6 dots long
        For k As Integer = 6 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k

        For j As Integer = 0 To strID.Length - 1 ' for each character in the string
            intMask = &H40000000
            Dim intIdx As Integer = strAlphabet.IndexOf(strID.ToUpper.Substring(j, 1))
            If intIdx = -1 Then
                ' process this as a space adding 6 dots worth of space to the wave file
                ReDim Preserve intWav(intWav.Length + (6 * intDotSampCnt) - 1)
                For k As Integer = 6 To 1 Step -1
                    Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
                Next k
            Else
                While intMask > 0 ' search for the first non 0 bit
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Exit While ' intMask is pointing to the first non 0 entry
                    Else
                        intMask = intMask \ 2 ' Right shift mask
                    End If
                End While
                While intMask > 0
                    ReDim Preserve intWav(intWav.Length + intDotSampCnt - 1)
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Array.Copy(intDot, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    Else
                        Array.Copy(intSpace, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    End If
                    intMask = intMask \ 2 ' Right shift mask
                End While
            End If
            ' add 3 dot spaces for inter letter spacing
            ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
            For k As Integer = 3 To 1 Step -1
                Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
            Next k
        Next
        ' add 3 spaces for the end tail
        ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
        For k As Integer = 3 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k
        If Not blnPlay Then
            intSamples = intWav
            Exit Sub
        End If
        ' Convert the integer array to bytes
        Dim aryWave(2 * intWav.Length - 1) As Byte
        For j As Integer = 0 To intWav.Length - 1
            aryWave(2 * j) = CByte(intWav(j) And &HFF) ' LSByte
            aryWave(1 + 2 * j) = CByte((intWav(j) And &HFF00) \ 256) ' MSbyte
        Next j
        ' *********************************
        ' Debug code to look at wave file 
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWave.WriteRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\CWID.wav", 12000, 16, aryWave)
        '' End of debug code
        '************************************
        objWave.WriteRIFFStream(memWaveStream, 12000, 16, aryWave)
*/
}


// Function to look up the byte value from the frame string name

UCHAR FrameCode(char * strFrameName)
{
	int i;

    for (i = 0; i < 256; i++)
	{
		if (strcmp(strFrameType[i], strFrameName) == 0)
		{
			return i;
		}
	}
	return 0;
}

int GenCRC16(unsigned char * Data, unsigned short length)
{
	// For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length); j++)	
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}
 
	return intRegister;
}

BOOL checkcrc16(unsigned char * Data, unsigned short length)
{
	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length - 2); j++)		// ' 2 bytes short of data length
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}

    if (Data[length - 2] == intRegister >> 8)
		if (Data[length - 1] == (intRegister & 0xFF))
			return TRUE;
   
	return FALSE;
}


//	Subroutine to compute a 16 bit CRC value and append it to the Data... With LS byte XORed by bytFrameType
    
void GenCRC16FrameType(char * Data, int intStartIndex, int intStopIndex, UCHAR bytFrameType)
{
	int CRC = GenCRC16(&Data[intStartIndex], intStopIndex - intStartIndex + 1);

	// Put the two CRC bytes after the stop index

	Data[intStopIndex + 1] = (CRC >> 8);		 // MS 8 bits of Register
	Data[intStopIndex + 2] = (CRC & 0xFF) ^ bytFrameType;  // LS 8 bits of Register
}

/*
    ' Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) XORing LS byte with bytFrameType..
    Public Function CheckCRC16FrameType(ByRef Data() As Byte, Optional bytFrameType As Byte = 0) As Boolean
        ' Returns True if CRC matches, else False
        ' For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
        ' intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

        Dim intPoly As Integer = &H8810 ' This implements the CRC polynomial  x^16 + x^12 +x^5 + 1
        Dim intRegister As Int32 = &HFFFF ' initialize the register to all 1's

        For j As Integer = 0 To Data.Length - 3 ' 2 bytes short of data length
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (Data(j) And CByte(2 ^ i)) <> 0
                If (intRegister And &H8000) = &H8000 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                End If
            Next i
        Next j

        ' Compare the register with the last two bytes of Data (the CRC) 
        If Data(Data.Length - 2) = CByte((intRegister And &HFF00) \ 256) Then
            If Data(Data.Length - 1) = (CByte(intRegister And &HFF) Xor bytFrameType) Then
                Return True
            End If
        End If
        Return False
    End Function 'CheckCRC16FrameType
*/

// Subroutine to get intDataLen bytes from outbound queue (bytDataToSend)

void ClearDataToSend()
{
	TXQueueLen = 0;
}

int GetDataFromQueue(UCHAR * Data, int MaxLen)
{
	int Returned = MaxLen;

	if (MaxLen == 0)
		return 0;

	GetSemaphore();

	if (MaxLen > TXQueueLen)
		Returned = TXQueueLen;

	memcpy(Data, TXQueue, Returned);

	TXQueueLen -= Returned;

	if (TXQueueLen)
		memmove(Data, &Data[Returned], TXQueueLen);

	FreeSemaphore();

	return Returned;
}

void KeyPTT(BOOL State)
{
}
