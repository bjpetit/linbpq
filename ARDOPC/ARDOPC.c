// ARDOPC.cpp : Defines the entry point for the console application.
//

#include "ARDOPC.h"

char GridSquare[7] = "IO68VL";
char Callsign[10] = "G8BPQ-2";
BOOL wantCWID = FALSE;
int LeaderLength = 500;
int TrailerLength = 0;

enum _ReceiveState State;
enum _ARDOPState ProtocolState;
enum _ARDOPState ARDOPState;

BOOL SoundIsPlaying = FALSE;

char ProtocolMode[4]= "";
int intSamplesToCompleteFrame;

time_t Now = 0;
UCHAR bytValidFrameTypes[256]= {0};

int bytValidFrameTypesLength = 0;

BOOL blnTimeoutTriggered= FALSE;

int MaxCorrections;

char TXQueue[4096] = "Hello";					// May malloc this, or change to cyclic buffer
int TXQueueLen = 5;

UCHAR bytSessionID;

int intLastRcvdFrameQuality;

int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 


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

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

#ifdef WIN32
float round(float x)
{
	return floor(x + 0.5);
}
#endif

void GetSemaphore()
{
}

void FreeSemaphore()
{
}

void CompressCallsign(char * Callsign, UCHAR * Compressed);
void CompressGridSquare(char * Square, UCHAR * Compressed);
void  ASCIIto6Bit(char * Padded, UCHAR * Compressed);
void GetTwoToneLeaderWithSync(int intSymLen, short * intLeader);

void SendID(BOOL blnEnableCWID);

void ardopmain()
{
	register int i;
	int t;

	blnTimeoutTriggered = FALSE;


//	GenerateTwoToneLeaderTemplate();
	GenerateFSKTemplates();
//	GeneratePSKTemplates();
	InitValidFrameTypes();
	InitSound();

	 SendID(0);
	 
	 ProtocolState = FECSend;
//	GetNextFECFrame();

// 
//  SendID(0);
//  ModTwoToneTest();

  return 0;
}




void AddTagToDataAndSendToHost(char * Msg, char * Type)
{
}

void SendCWID(char * Callsign, BOOL x)
{
}

// Subroutine to generate 1 symbol of leader

//	 Returns pointer to Frame Type Name

char * Name(UCHAR bytID)
{
	if (bytID < 0x20)
		return strFrameType[0];
	else if (bytID >= 0xE0)
		return strFrameType[0xE0];
	else
		return strFrameType[bytID];
}

// Function to look up frame info from bytFrameType

BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
			   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType)
{
	//Used to "lookup" all parameters by frame Type. 
	// Returns True if all fields updated otherwise FALSE (improper bytFrameType)

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

int NPAR = -1;	// Number of Parity Bytes - used in RS Code

int RSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen)
{
	// This just returns the Parity Bytes. I don't see the point
	// in copying the message about

	int i;

	unsigned char Padded[256];		// The padded Data

	int Length = DataLen + RSLen;	// Final Length of packet
	int PadLength = 253 - DataLen;	// Padding bytes needed for shortened RS codes

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	if (NPAR != RSLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = RSLen;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen); 

	encode_data(Padded, 253, RSBytes);

	return RSLen;
}

//	Main RS decode function


BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, UCHAR * Corrected, BOOL * blnRSOK)
{	
	// Using a modified version of Henry Minsky's code
	
	//Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009

	// Rick's Implementation processes the byte array in reverse. and also 
	//	has the check bytes in the opposite order. I've modified the encoder
	//	to allow for this, but so far haven't found a way to mske the decoder
	//	work, so I have to reverse the data and checksum to decode G8BPQ Nov 2015

	//	Returns TRUE if was ok or correction succeeded, FALSE if correction impossible

	UCHAR intTemp[256];				// WOrk Area to pass to Decoder		
	int i;
	UCHAR * ptr2 = intTemp;
	UCHAR * ptr1 = &bytRcv[Length - CheckLen -1]; // Last Byte of Data

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;

	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		initialize_ecc();
	}


	//	We reverse the data while zero padding it to speed things up

	//	We Need (Data Reversed) (Zero Padding) (Checkbytes Reversed)

	// Reverse Data

	for (i = 0; i < DataLen; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}

	//	Clear padding

	memset(ptr2, 0, PadLength);	

	ptr2+= PadLength;
	
	// Error Bits

	ptr1 = &bytRcv[Length - 1];			// End of check bytes

	for (i = 0; i < CheckLen; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}
	
	decode_data(intTemp, 255);

	// check if syndrome is all zeros 

	if (check_syndrome() == 0)
	{
		// RS ok, so no need to correct

		*blnRSOK = TRUE;
		return TRUE;		// No Need to Correct
	}

    if (correct_errors_erasures (intTemp, 255, 0, 0) == 0) // Dont support erasures at the momnet

		// Uncorrectable

		return FALSE;

	// Data has been corrected, so need to reverse again

	ptr1 = &intTemp[DataLen - 1];
	ptr2 = bytRcv; // Last Byte of Data

	for (i = 0; i < Length; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}

	// ?? Do we need to return the check bytes ??
 
	return TRUE;
}

/*

Old code

	// convert to indexed form

	for(i = 0; i < 256; i++)
	{
		intIsave = i;
		intIndexSave = index_of[intTemp[i]];
		recd[i] = index_of[intTemp[i]];
	}

	printtick("entering decode_rs");

	decode_rs();

	printtick("decode_rs Done");

	*blnRSOK = blnErrorsCorrected;

	if(blnErrorsCorrected)
	{
		for (i = 0; i < Length - (2 * tt); i++)
		{
			Corrected[i] = recd[i + intStartIndex];
		}
	}
	else
	{
		// Just return input minus RS (can't we just use input??

		memcpy(Corrected, bytRcv, Length - 2 * tt);
		*blnRSOK = TRUE;
	}
}
*/

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
	UCHAR RSBytes[MAXNPAR];


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

			EncLen = RSEncode(bytToRS, RSBytes, intDataLen, intRSLen);  // Generate the RS encoding ...now 14 bytes total
        
			memcpy(&bytEncodedData[intEncodedDataPtr], bytToRS, intDataLen);
 
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

	UCHAR * bytToRS= &bytReturn[2];

	 // If (Not CheckValidCallsignSyntax(strMyCallsign)) Then
     //       Logs.Exception("[EncodeModulate.EncodeIDFrame] Illegal Callsign syntax or Gridsquare length. MyCallsign = " & strMyCallsign & ", Gridsquare = " & strGridSquare)
      //      Return Nothing


	bytReturn[0] = 0x30;
	bytReturn[1] = 0x30 ^ 0xFF;

	// Modified May 9, 2015 to use RS instead of 2 byte CRC.
       
	CompressCallsign(Callsign, &bytToRS[0]);

    if (Square[0])
		CompressGridSquare(Square, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	RSEncode(bytToRS, &bytReturn[14], 12, 2);  // Generate the RS encoding ...now 14 bytes total

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
	SampleSink(0);	// 5 secs
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

void Bit6ToASCII(UCHAR * Padded, UCHAR * UnCompressed)
{
	// uncompress 6 to 8

	// Input must be 6 bytes which represent packed 6 bit characters that well 
	// result will be 8 ASCII character set values from 32 to 95...

	unsigned long long intSum = 0;

	int i;

	for (i=0; i<3; i++)
	{
		intSum = (intSum << 8) + Padded[i];
	}

	UnCompressed[0] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[1] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[2] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[3] = (UCHAR)(intSum & 63) + 32;

	intSum = 0;

	for (i=3; i<6; i++)
	{
		intSum = (intSum << 8) + Padded[i] ;
	}

	UnCompressed[4] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[5] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[6] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[7] = (UCHAR)(intSum & 63) + 32;
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

// Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
  
void DeCompressCallsign(char * bytCallsign, char * returned)
{
	char bytTest[10];
	char SSID[8] = "";
    
	Bit6ToASCII(bytCallsign, bytTest);

	memcpy(returned, bytTest, 6);
	returned[6] = 0;
	strlop(returned, ' ');		// remove trailing space

	if (bytTest[7] == '0') // Value of "0" so No SSID
		returned[6] = 0;
	else if (bytTest[7] >= 58 && bytTest[7] <= 63) //' handles special case for -10 to -15
		sprintf(SSID, "-%d", bytTest[7] - 48);
	else
		sprintf(SSID, "-%c", bytTest[7]);
	
	strcat(returned, SSID);
}


// Function to decompress 6 byte Grid square to 4, 6 or 8 characters

void DeCompressGridSquare(char * bytGS, char * returned)
{
	char bytTest[10];
	Bit6ToASCII(bytGS, bytTest);

	strlop(bytTest, ' ');
	strcpy(returned, bytTest);
}

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

        Dim dblHiPhaseInc As float = 2 * PI * 1609.375 / 12000 ' 1609.375 Hz High tone
        Dim dblLoPhaseInc As float = 2 * PI * 1390.625 / 12000 ' 1390.625  low tone
        Dim dblHiPhase As float
        Dim dblLoPhase As float
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
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = FALSE Then
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
        ' Returns True if CRC matches, else FALSE
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
        Return FALSE
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
