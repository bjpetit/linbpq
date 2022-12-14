//
//	Rig Control Module
//

// Dec 29 2009

//	Add Scan Control for SCS 

// August 2010

// Fix logic error in Port Initialisation (wasn't always raising RTS and DTR
// Clear RTS and DTR on close

// Fix Kenwood processing of multiple messages in one packet.

// Fix reporting of set errors in scan to the wrong session

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <commctrl.h>

//#include <process.h>
//#include <time.h>

#include "TNCInfo.h"
#include "ASMStrucs.h"

#include "bpq32.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd
extern char * RigConfigMsg[35];

extern UINT FREE_Q;
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

int Row = -20;

BOOL RIG_DEBUG = FALSE;

extern struct PORTCONTROL * PORTTABLE;

VOID __cdecl Debugprintf(const char * format, ...);

struct RIGINFO * RigConfig(char * buf, int Port);
struct RIGPORTINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL RigCloseConnection(struct RIGPORTINFO * PORT);
BOOL NEAR RigWriteCommBlock(struct RIGPORTINFO * PORT);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct RIGPORTINFO * PORT);
static OpenRigCOMMPort(struct RIGPORTINFO * PORT, int Port, int Speed);
VOID ICOMPoll(struct RIGPORTINFO * PORT);
VOID ProcessFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuff, int len);
VOID ProcessICOMFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuffer, int Len);
SendResponse(int Stream, char * Msg);
VOID ProcessYaesuFrame(PORT);
VOID YaesuPoll(struct RIGPORTINFO * PORT);
VOID ProcessYaesuCmdAck(struct RIGPORTINFO * PORT);
VOID ProcessKenwoodFrame(struct RIGPORTINFO * PORT, int Length);
VOID KenwoodPoll(struct RIGPORTINFO * PORT);
VOID SwitchAntenna(struct RIGINFO * RIG, char Antenna);
VOID DoBandwidthandAntenna(struct RIGINFO *RIG, struct ScanEntry * ptr);
VOID SetupScanInterLockGroups(struct RIGINFO *RIG);
VOID ProcessFT100Frame(struct RIGPORTINFO * PORT);
VOID AddNMEAChecksum(char * msg);
VOID ProcessNMEA(struct RIGPORTINFO * PORT, char * NMEAMsg, int len);

VOID SetupPortRIGPointers();

extern  struct TRANSPORTENTRY * L4TABLE;
HANDLE hInstance;

VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset); 
BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase);

INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


char Modes[8][6] = {"LSB",  "USB", "AM", "CW", "RTTY", "FM", "WFM", "????"};

//							0		1	  2		3	   4	5	6	7	8	9		0A  0B    0C    88

char YaesuModes[16][6] = {"LSB",  "USB", "CW", "CWR", "AM", "", "", "", "FM", "", "DIG", "", "PKT", "FMN", "????"};

char FT100Modes[9][6] = {"LSB",  "USB", "CW", "CWR", "AM", "DIG", "FM", "WFM", "????"};


char KenwoodModes[16][6] = {"????", "LSB",  "USB", "CW", "FM", "AM", "FSK", "????"};

char FT2000Modes[16][6] = {"????", "LSB",  "USB", "CW", "FM", "AM", "FSK", "PKT-L", "FSK-R", "PKT-FM", "FM-N", "PKT-U", "????"};

char FLEXModes[16][6] = {"LSB", "USB", "DSB", "CWL", "CWU", "FM", "AM", "DIGU", "SPEC", "DIGL", "SAM", "DRM"};

char AuthPassword[100] = "";

char LastPassword[17];

int NumberofPorts = 0;

struct RIGPORTINFO * PORTInfo[34] = {NULL};		// Records are Malloc'd


struct TimeScan * AllocateTimeRec(struct RIGINFO * RIG)
{
	struct TimeScan * Band = malloc(sizeof (struct TimeScan));
	
	RIG->TimeBands = realloc(RIG->TimeBands, (++RIG->NumberofBands+2)*4);
	RIG->TimeBands[RIG->NumberofBands] = Band;
	RIG->TimeBands[RIG->NumberofBands+1] = NULL;

	return Band;
}

DllExport struct ScanEntry ** APIENTRY CheckTimeBands(struct RIGINFO * RIG)
{
	int i = 0;
	time_t NOW = time(NULL) % 86400;
				
	// Find TimeBand

	while (i < RIG->NumberofBands)
	{
		if (RIG->TimeBands[i + 1]->Start > NOW)
		{
			break;
		}
		i++;
	}

	RIG->FreqPtr = RIG->TimeBands[i]->Scanlist;

	return RIG->FreqPtr;
}


DllExport VOID APIENTRY Rig_PTT(struct RIGINFO * RIG, BOOL PTTState)
{
	struct RIGPORTINFO * PORT;

	if (RIG == NULL) return;

	PORT = RIG->PORT;

	if (PTTState)
	{
		SetWindowText(RIG->hPTT, "T");
		RIG->PTTTimer = PTTLimit;
	}
	else
	{
		SetWindowText(RIG->hPTT, "");
		RIG->PTTTimer = 0;
	}

	if (RIG->PTTMode & PTTCI_V)
	{
		UCHAR * Poll = PORT->TXBuffer;

		switch (PORT->PortType)
		{
		case ICOM:

			*(Poll++) = 0xFE;
			*(Poll++) = 0xFE;
			*(Poll++) = RIG->RigAddr;
			*(Poll++) = 0xE0;
			*(Poll++) = 0x1C;		// RIG STATE
			*(Poll++) = 0x00;		// PTT
			*(Poll++) = PTTState;	// OFF/ON

			*(Poll++) = 0xFD;
	
			PORT->TXLen = 8;
			RigWriteCommBlock(PORT);

			PORT->Retries = 1;

			return;

		case KENWOOD:
		case FT2000:
		case FLEX:

			if (PTTState)
			{
				strcpy(Poll, RIG->PTTOn);
				PORT->TXLen = RIG->PTTOnLen;
			}
			else
			{
				strcpy(Poll, RIG->PTTOff);
				PORT->TXLen = RIG->PTTOffLen;
			}

			RigWriteCommBlock(PORT);

			PORT->Retries = 1;
			PORT->Timeout = 0;
			return;

		case FT100:

			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = PTTState;	// OFF/ON
			*(Poll++) = 15;
	
			PORT->TXLen = 5;
			RigWriteCommBlock(PORT);

			PORT->Retries = 1;
			PORT->Timeout = 0;

			return;

		case YAESU:  // 897 - maybe others

			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = 0;
			*(Poll++) = PTTState ? 0x08 : 0x88;		// CMD = 08 : PTT ON CMD = 88 : PTT OFF
	
			PORT->TXLen = 5;
			RigWriteCommBlock(PORT);

			PORT->Retries = 1;
			PORT->Timeout = 0;

			return;

		}
	}

	if (RIG->PTTMode & PTTRTS)
		if (PTTState)
			EscapeCommFunction(PORT->hPTTDevice,SETRTS);
		else
			EscapeCommFunction(PORT->hPTTDevice,CLRRTS);

	if (RIG->PTTMode & PTTDTR)
		if (PTTState)
			EscapeCommFunction(PORT->hPTTDevice,SETDTR);
		else
			EscapeCommFunction(PORT->hPTTDevice,CLRDTR);
		
}

DllExport struct RIGINFO * APIENTRY Rig_GETPTTREC(int Port)
{
	struct RIGINFO * RIG;
	struct RIGPORTINFO * PORT;
	int i, p;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->BPQPort & (1 << Port))
				return RIG;
		}
	}

	return NULL;
}



DllExport int APIENTRY Rig_Command(int Session, char * Command)
{
	int n, Port, ModeNo, Filter;
	double Freq = 0.0;
	char FreqString[80]="", FilterString[80]="", Mode[80]="", Data[80] = "";
	UINT * buffptr;
	UCHAR * Poll;
	char * 	Valchar ;
	int dec, sign;
	struct RIGPORTINFO * PORT;
	int i, p;
	struct RIGINFO * RIG;
	struct TRANSPORTENTRY * L4 = L4TABLE;
	char * ptr;
	int Split, DataFlag, Bandwidth, Antenna;
	struct ScanEntry * FreqPtr;
	char * CmdPtr;
	int Len;

	//	Only Allow RADIO from Secure Applications

	_strupr(Command);

	ptr = strchr(Command, 13);
	if (ptr) *(ptr) = 0;						// Null Terminate

	if (memcmp(Command, " AUTH ", 6) == 0)
	{
		if (AuthPassword[0] && (memcmp(LastPassword, &Command[6], 16) != 0))
		{
			if (CheckOneTimePassword(&Command[6], AuthPassword))
			{
				L4 += Session;
				L4->Secure_Session = 1;

				wsprintf(Command, "Ok\r");

				memcpy(LastPassword, &Command[6], 16);	// Save

				return FALSE;
			}
		}
		
		wsprintf(Command, "Sorry AUTH failed\r");
		return FALSE;
	}

	if (Session != -1)				// Used for internal Stop/Start
	{		
		L4 += Session;

		if (L4->Secure_Session == 0)
		{
			wsprintf(Command, "Sorry - you are not allowed to use this command\r");
			return FALSE;
		}
	}
	if (NumberofPorts == 0)
	{
		wsprintf(Command, "Sorry - Rig Control not configured\r");
		return FALSE;
	}

	n = sscanf(Command,"%d %s %s %s %s", &Port, &FreqString, &Mode, &FilterString, &Data);

	// Look for the port 

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->BPQPort & (1 << Port))
				goto portok;
		}
	}

	wsprintf(Command, "Sorry - Port not found\r");
	return FALSE;

portok:

	if (RIG->RIGOK == 0 && Session != -1)
	{
		wsprintf(Command, "Sorry - Radio not responding\r");
		return FALSE;
	}

	if (n > 1)
	{
		if (_stricmp(FreqString, "SCANSTART") == 0)
		{
			if (RIG->NumberofBands)
			{
				RIG->ScanStopped &= (0xffffffff ^ (1 << Port));

				if (Session != -1)				// Used for internal Stop/Start
					RIG->ScanStopped &= 0xfffffffe; // Clear Manual Stopped Bit

				if (n > 2)
					RIG->ScanCounter = atoi(Mode) * 10;  //Start Delay
				else
					RIG->ScanCounter = 10;

				RIG->WaitingForPermission = FALSE;		// In case stuck	

				if (RIG->ScanStopped == 0)
					SetWindowText(RIG->hSCAN, "S");

				wsprintf(Command, "Ok\r");

				if (RIG_DEBUG)
					Debugprintf("BPQ32 SCANSTART Port %d", Port);
			}
			else
				wsprintf(Command, "Sorry no Scan List defined for this port\r");

			return FALSE;
		}

		if (_stricmp(FreqString, "SCANSTOP") == 0)
		{
			RIG->ScanStopped |= (1 << Port);

			if (Session != -1)				// Used for internal Stop/Start
				RIG->ScanStopped |= 1;		// Set Manual Stopped Bit

			SetWindowText(RIG->hSCAN, "");

			wsprintf(Command, "Ok\r");

			if (RIG_DEBUG)
				Debugprintf("BPQ32 SCANSTOP Port %d", Port);

			return FALSE;
		}
	}

	RIG->Session = Session;		// BPQ Stream

	Freq = atof(FreqString);

	if (Freq < 0.1)
	{
		strcpy(Command, "Sorry - Invalid Frequency\r");
		return FALSE;
	}

	Freq = Freq * 1000000.0;

	Valchar = _fcvt(Freq, 0, &dec, &sign);

	if (dec == 9)	// 10-100
		wsprintf(FreqString, "%s", Valchar);
	else
	if (dec == 8)	// 10-100
		wsprintf(FreqString, "0%s", Valchar);
	else
	if (dec == 7)	// 1-10
		wsprintf(FreqString, "00%s", Valchar);
	else
	if (dec == 6)	// 0 - 1
		wsprintf(FreqString, "000%s", Valchar);

	if (PORT->PortType != ICOM)
		strcpy(Data, FilterString);			// Others don't have a filter.

	Split = DataFlag = Bandwidth = Antenna = 0;

	_strupr(Data);

	if (strchr(Data, '+'))
		Split = '+';
	else if (strchr(Data, '-'))				
		Split = '-';
	else if (strchr(Data, 'S'))
		Split = 'S';	
	else if (strchr(Data, 'D'))	
		DataFlag = 1;
								
	if (strchr(Data, 'W'))
		Bandwidth = 'W';	
	else if (strchr(Data, 'N'))
		Bandwidth = 'N';

	if (strstr(Data, "A1"))
		Antenna = '1';
	else if (strstr(Data, "A2"))
		Antenna = '2';
	if (strstr(Data, "A3"))
		Antenna = '3';
	else if (strstr(Data, "A4"))
		Antenna = '4';

	switch (PORT->PortType)
	{ 
	case ICOM:

		if (n == 2)
			// Set Freq Only

			ModeNo = -1;
		else
		{
			if (n < 4)
			{
				strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode Filter Width\r");
				return FALSE;
			}

			Filter = atoi(FilterString);

			for (ModeNo = 0; ModeNo < 8; ModeNo++)
			{
				if (_stricmp(Modes[ModeNo], Mode) == 0)
					break;
			}

			if (ModeNo == 8)
			{
				wsprintf(Command, "Sorry - Invalid Mode\r");
				return FALSE;
			}
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)&buffptr[2];

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		CmdPtr = FreqPtr->Cmd1 = (UCHAR *)&buffptr[20];
		FreqPtr->Cmd2 = NULL;
		FreqPtr->Cmd3 = NULL;

		*(CmdPtr++) = 0xFE;
		*(CmdPtr++) = 0xFE;
		*(CmdPtr++) = RIG->RigAddr;
		*(CmdPtr++) = 0xE0;
		*(CmdPtr++) = 0x5;		// Set frequency command

		// Need to convert two chars to bcd digit
	
		*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
		*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
		*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
		*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
		*(CmdPtr++) = (FreqString[0] - 48);

		*(CmdPtr++) = 0xFD;

		if (ModeNo != -1)			// Dont Set
		{		
			CmdPtr = FreqPtr->Cmd2 = (UCHAR *)&buffptr[30];
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;
			*(CmdPtr++) = 0x6;		// Set Mode
			*(CmdPtr++) = ModeNo;
			*(CmdPtr++) = Filter;
			*(CmdPtr++) = 0xFD;

			if (Split)
			{
				CmdPtr = FreqPtr->Cmd3 = (UCHAR *)&buffptr[40];
				FreqPtr->Cmd3Len = 7;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = RIG->RigAddr;
				*(CmdPtr++) = 0xE0;
				*(CmdPtr++) = 0xF;		// Set Mode
				if (Split == 'S')
					*(CmdPtr++) = 0x10;
					else
						if (Split == '+')
							*(CmdPtr++) = 0x12;
					else
						if (Split == '-')
							*(CmdPtr++) = 0x11;
			
					*(CmdPtr++) = 0xFD;
			}
			else if (DataFlag)
			{
				CmdPtr = FreqPtr->Cmd3 = (UCHAR *)&buffptr[40];
				FreqPtr->Cmd3Len = 8;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = RIG->RigAddr;
				*(CmdPtr++) = 0xE0;
				*(CmdPtr++) = 0x1a;		
				*(CmdPtr++) = 0x6;		// Set Data
				*(CmdPtr++) = 0x1;		//On		
				*(CmdPtr++) = 0xFD;
			}
		}

		buffptr[1] = 200;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case YAESU:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 15; ModeNo++)
		{
			if (_stricmp(YaesuModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo == 15)
		{
			wsprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)&buffptr[2];

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = (UCHAR *)&buffptr[20];

		// Send Mode then Freq - setting Mode seems to change frequency

		*(Poll++) = ModeNo;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 7;		// Set Mode

		*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
		*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
		*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
		*(Poll++) = 1;		// Set Freq
					
		buffptr[1] = 10;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;


	case FT100:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 8; ModeNo++)
		{
			if (_stricmp(FT100Modes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo == 8)
		{
			wsprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)&buffptr[2];

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = (UCHAR *)&buffptr[20];

		// Send Mode then Freq - setting Mode seems to change frequency

		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = ModeNo;
		*(Poll++) = 12;		// Set Mode

		*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
		*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
		*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
		*(Poll++) = 10;		// Set Freq

		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 16;		// Status Poll
	
		buffptr[1] = 15;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case KENWOOD:
	case FT2000:
	case FLEX:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 14; ModeNo++)
		{
			if (PORT->PortType == FT2000)
				if (_stricmp(FT2000Modes[ModeNo], Mode) == 0)
				break;

			if (PORT->PortType == KENWOOD)
				if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
				break;
			if (PORT->PortType == FLEX)
				if (_stricmp(FLEXModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo > 12)
		{
			wsprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = GetBuff();

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)&buffptr[2];

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = (UCHAR *)&buffptr[20];

		if (PORT->PortType == FT2000)
			buffptr[1] = wsprintf(Poll, "FA%s;MD0%X;FA;MD;", &FreqString[1], ModeNo);
		else
		if (PORT->PortType == FLEX)
			buffptr[1] = wsprintf(Poll, "ZZFA00%s;ZZMD%02d;ZZFA;ZZMD;", &FreqString[1], ModeNo);
		else
			buffptr[1] = wsprintf(Poll, "FA00%s;MD%d;FA;MD;", FreqString, ModeNo);
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case NMEA:
			
		if (n < 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}
		buffptr = GetBuff();

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Build a ScanEntry in the buffer

		FreqPtr = (struct ScanEntry *)&buffptr[2];

		FreqPtr->Freq = Freq;
		FreqPtr->Bandwidth = Bandwidth;
		FreqPtr->Antenna = Antenna;

		Poll = (UCHAR *)&buffptr[20];

		i = sprintf(Poll, "$PICOA,90,%02x,RXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
		AddNMEAChecksum(Poll);
		Len = i;
		i = sprintf(Poll + Len, "$PICOA,90,%02x,TXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
		AddNMEAChecksum(Poll + Len);
		Len += i;
		i = sprintf(Poll + Len, "$PICOA,90,%02x,MODE,%s*xx\r\n", RIG->RigAddr, Mode);
		AddNMEAChecksum(Poll + Len);
			
		buffptr[1] = i + Len;
		
		C_Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	}
	return TRUE;
}

int BittoInt(UINT BitMask)
{
	// Returns bit position of first 1 bit in BitMask
	
	int i = 0;
	while ((BitMask & 1) == 0)
	{	
		BitMask >>= 1;
		i ++;
	}
	return i;
}


DllExport BOOL APIENTRY Rig_Init()
{
	struct RIGPORTINFO * PORT;
	int i, p, port;
	struct RIGINFO * RIG;
	char szPort[15];
	struct TNCINFO * TNC;
	HWND hDlg;
	int RigRow;

	// Get config info

	for (port = 1; port < 33; port++)
	{
		TNC = TNCInfo[port];

		if (TNC == NULL)
			continue;

		if (RigConfigMsg[port])
		{
			char msg[1000];
			
			char * SaveRigConfig = _strdup(RigConfigMsg[port]);
			char * RigConfigMsg1 = _strdup(RigConfigMsg[port]);

			RIG = TNC->RIG = RigConfig(RigConfigMsg1, port);
			
			if (TNC->RIG == NULL)
			{
				// Report Error

				wsprintf(msg,"Port %d Invalid Rig Config %s", port, SaveRigConfig);
				WritetoConsole(msg);
			}

			TNC->RIG->PTTMode = TNC->PTTMode;

			free(SaveRigConfig);
			free(RigConfigMsg1);

			hDlg = TNC->hDlg;

			if (hDlg == 0)
			{
				// Running on a port without a window, eg  UZ7HO or MultiPSK

				CreatePactorWindow(TNC, "RIGCONTROL", "RigControl", 10, PacWndProc, 300, 60);
				hDlg = TNC->hDlg;
				TNC->ClientHeight = 60;
				TNC->ClientWidth = 300;
			}

			RigRow = TNC->RigControlRow;

			RIG->hLabel = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
				10, RigRow, 80,20, hDlg, NULL, hInstance, NULL);
	
			RIG->hCAT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 90, RigRow, 40,20, hDlg, NULL, hInstance, NULL);
	
			RIG->hFREQ = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 135, RigRow, 100,20, hDlg, NULL, hInstance, NULL);
	
			RIG->hMODE = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 240, RigRow, 60,20, hDlg, NULL, hInstance, NULL);
	
			RIG->hSCAN = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 300, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

			RIG->hPTT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 320, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

		//if (PORT->PortType == ICOM)
		//{
		//	wsprintf(msg,"%02X", PORT->Rigs[i].RigAddr);
		//	SetWindowText(RIG->hCAT, msg);
		//}
			SetWindowText(RIG->hLabel, RIG->RigName);
		}
	}

   
	if (NumberofPorts == 0)
	{
		SetupPortRIGPointers();
		return TRUE;
	}

	Row = 0;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];
		
//		CreateDisplay(PORT);

		OpenRigCOMMPort(PORT, PORT->IOBASE, PORT->SPEED);

		if (PORT->PTTIOBASE)		// Using separare port for PTT?
		{
			wsprintf(szPort, "//./COM%d", PORT->PTTIOBASE) ;

			PORT->hPTTDevice = CreateFile(szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
				  
			if (PORT->hPTTDevice == (HANDLE) -1)
			{
				char buf[80];

				wsprintf(buf," COM%d Setup Failed - Error %d ", PORT->PTTIOBASE, GetLastError());
				WritetoConsole(buf);
				OutputDebugString(buf);
				SetWindowText(PORT->hStatus, &buf[1]);
				PORT->hPTTDevice = 0;
			}
		}
		else
			PORT->hPTTDevice = PORT->hDevice;	// Use same port for PTT
	}

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i < PORT->ConfiguredRigs; i++)
		{
			int j;
			int k = 0;
			int BitMask;
			struct _EXTPORTDATA * PortEntry;

			RIG = &PORT->Rigs[i];

			SetupScanInterLockGroups(RIG);

			// Get record for each port in Port Bitmap

			// The Scan "Request Permission to Change" code needs the Port Records in order - 
			// Those with active connect lock (eg SCS) first, then those with just a connect pending lock (eg WINMOR)
			// then those with neither
			
			BitMask = RIG->BPQPort;
			for (j = 0; j < 32; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == CONLOCK)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			BitMask = RIG->BPQPort;
			for (j = 0; j < 32; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == SIMPLE)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			BitMask = RIG->BPQPort;
			for (j = 0; j < 32; j++)
			{
				if (BitMask & 1)
				{
					PortEntry = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(j);		// BPQ32 port record for this port
					if (PortEntry)
						if (PortEntry->SCANCAPABILITIES == NONE)
							RIG->PortRecord[k++] = PortEntry;
				}
				BitMask >>= 1;
			}

			RIG->PORT = PORT;		// For PTT
			
			if (RIG->NumberofBands)
				CheckTimeBands(RIG);		// Set initial timeband
		}
	}
//	MoveWindow(hDlg, Rect.left, Rect.top, Rect.right - Rect.left, Row + 100, TRUE);

	SetupPortRIGPointers();

	WritetoConsole("\nRig Control Enabled\n");

	return TRUE;
}

DllExport BOOL APIENTRY Rig_Close()
{
	struct RIGPORTINFO * PORT;
	struct TNCINFO * TNC;
	int n, p;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];
		EscapeCommFunction(PORT->hDevice,CLRDTR);
		EscapeCommFunction(PORT->hDevice,CLRRTS);

		CloseHandle(PORT->hDevice);

		if (PORT->hPTTDevice != PORT->hDevice)
			CloseHandle(PORT->hPTTDevice);

		// Free the RIG and Port Records

		for (n = 0; n < PORT->ConfiguredRigs; n++)
		{
			if (PORT->Rigs[n].TimeBands)
				free (PORT->Rigs[n].TimeBands[1]->Scanlist);
		}

		free (PORT);
		PORTInfo[p] = NULL;
	}

	NumberofPorts = 0;		// For possible restart

	// And free the TNC config info

	for (p = 1; p < 33; p++)
	{
		TNC = TNCInfo[p];

		if (TNC == NULL)
			continue;

		TNC->RIG = NULL;

//		memset(TNC->WL2KInfoList, 0, sizeof(TNC->WL2KInfoList));

	}

	return TRUE;
}


DllExport BOOL APIENTRY Rig_Poll()
{
	int p, i;
	
	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		if (PORT == NULL || PORT->hDevice == (HANDLE) -1)
			continue;

		// Check PTT Timers

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->PTTTimer)
			{
				RIG->PTTTimer--;
				if (RIG->PTTTimer == 0)
					Rig_PTT(RIG, FALSE);
			}
		}

		CheckRX(PORT);

		switch (PORT->PortType)
		{ 
		case ICOM:
			
			ICOMPoll(PORT);
			break;

		case YAESU:
		case FT100:
			
			YaesuPoll(PORT);
			break;

		case KENWOOD:
		case FT2000:
		case FLEX:
		case NMEA:
			
			KenwoodPoll(PORT);
			break;
		}
	}

	return TRUE;
}
 

BOOL RigCloseConnection(struct RIGPORTINFO * PORT)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(PORT->hDevice, 0);

   // drop DTR

   EscapeCommFunction(PORT->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(PORT->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(PORT->hDevice);
 
   return TRUE;

} // end of CloseConnection()

OpenRigCOMMPort(struct RIGPORTINFO * PORT, int Port, int Speed)
{
	char szPort[15];
	char buf[80];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	DCB	dcb;

	// load the COM prefix string and append port number
   
	wsprintf( szPort, "//./COM%d", Port) ;

	// open COMM device

	PORT->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (PORT->hDevice == (HANDLE) -1)
	{
		wsprintf(buf," COM%d Setup Failed - Error %d ", Port, GetLastError());
		WritetoConsole(buf);
		OutputDebugString(buf);
		SetWindowText(PORT->hStatus, &buf[1]);

		return (FALSE);
	}

	SetupComm(PORT->hDevice, 4096, 4096); // setup device buffers

	// purge any information in the buffer

	PurgeComm(PORT->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(PORT->hDevice, &CommTimeOuts);
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(PORT->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;

	dcb.BaudRate = Speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	fRetVal = SetCommState(PORT->hDevice, &dcb);

	if (PORT->PortType != PTT)
	{
		EscapeCommFunction(PORT->hDevice,SETDTR);
		EscapeCommFunction(PORT->hDevice,SETRTS);
	}

	wsprintf(buf,"COM%d Open", Port);
	SetWindowText(PORT->hStatus, buf);

	return TRUE;
}

void CheckRX(struct RIGPORTINFO * PORT)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;
	char NMEAMsg[100];
	char * ptr;
	int len;

	if (PORT->hDevice == (HANDLE) -1) 
		return;


	// only try to read number of bytes in queue 

	if (PORT->RXLen == 500)
		PORT->RXLen = 0;

	ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)PORT->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	fReadStat = ReadFile(PORT->hDevice, &PORT->RXBuffer[PORT->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	PORT->RXLen += Length;

	Length = PORT->RXLen;

	switch (PORT->PortType)
	{ 
	case ICOM:
	
		if (Length < 6)				// Minimum Frame Sise
			return;

		if (PORT->RXBuffer[Length-1] != 0xfd)
			return;	

		ProcessICOMFrame(PORT, PORT->RXBuffer, Length);	// Could have multiple packets in buffer

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case YAESU:

		// Possible responses are a single byte ACK/NAK or a 5 byte info frame

		if (Length == 1 && PORT->CmdSent)
		{
			ProcessYaesuCmdAck(PORT);
			return;
		}
	
		if (Length < 5)			// Frame Sise
			return;

		if (Length > 5)			// Frame Sise
		{
			PORT->RXLen = 0;	// Corruption - reset and wait for retry	
			return;
		}

		ProcessYaesuFrame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case FT100:

		// Only responseshould be a 16 byte info frame
	
		if (Length < 32)		// Frame Sise
			return;

		if (Length > 32)			// Frame Sise
		{
			PORT->RXLen = 0;	// Corruption - reset and wait for retry	
			return;
		}

		ProcessFT100Frame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case KENWOOD:
	case FT2000:
	case FLEX:

		if (Length < 2)				// Minimum Frame Sise
			return;

		if (Length > 50)			// Garbage
		{
			PORT->RXLen = 0;		// Ready for next frame	
			return;
		}

		if (PORT->RXBuffer[Length-1] != ';')
			return;	

		ProcessKenwoodFrame(PORT, Length);	

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case NMEA:

		ptr = memchr(PORT->RXBuffer, 0x0a, Length);

		while (ptr != NULL)
		{
			ptr++;									// include lf
			len = ptr-(char *)&PORT->RXBuffer;	
			
			memcpy(NMEAMsg, PORT->RXBuffer, len);	

			NMEAMsg[len] = 0;

//			if (Check0183CheckSum(NMEAMsg, len))
				ProcessNMEA(PORT, NMEAMsg, len);

			Length -= len;							// bytes left

			if (Length > 0)
			{
				memmove(PORT->RXBuffer, ptr, Length);
				ptr = memchr(PORT->RXBuffer, 0x0a, Length);
			}
			else
				ptr=0;
		}

		PORT->RXLen = Length;
	}
}

VOID ProcessICOMFrame(struct RIGPORTINFO * PORT, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;

	//	Split into Packets. By far the most likely is a single KISS frame
	//	so treat as special case
	
	FendPtr = memchr(rxbuffer, 0xfd, Len);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessFrame(PORT, rxbuffer, Len);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer +1;

	ProcessFrame(PORT, rxbuffer, NewLen);
	
	// Loop Back

	ProcessICOMFrame(PORT, FendPtr+1, Len - NewLen);
	return;
}



BOOL NEAR RigWriteCommBlock(struct RIGPORTINFO * PORT)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(PORT->hDevice, PORT->TXBuffer, PORT->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (PORT->TXLen != dwBytesWritten))
	{
		ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);
	}

	PORT->Timeout = 100;		// 2 secs

	return TRUE;  
}

VOID ReleasePermission(struct RIGINFO *RIG)
{
	int i = 0;
	struct _EXTPORTDATA * PortRecord;

	while (RIG->PortRecord[i])
	{
		PortRecord = RIG->PortRecord[i];
		PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 3);	// Release Perrmission
		i++;
	}
}

GetPermissionToChange(struct RIGPORTINFO * PORT, struct RIGINFO *RIG)
{
	struct ScanEntry ** ptr;
	struct _EXTPORTDATA * PortRecord;
	int i;

	ptr = RIG->FreqPtr;

	if (ptr == NULL)
	{
		Debugprintf("Scan Debug - No freqs - quitting"); 
		return FALSE;					 // No Freqs
	}

	if (ptr[0] == (struct ScanEntry *)0) // End of list - reset to start
	{
		ptr = CheckTimeBands(RIG);
	}

	PORT->FreqPtr = ptr[0];				// Save Scan Command Block

	// Get Permission to change

	if (RIG->WaitingForPermission)
	{
		// TNC has been asked for permission, and we are waiting respoonse
		
		RIG->OKtoChange = RIG->PortRecord[0]->PORT_EXT_ADDR(6, RIG->PortRecord[0]->PORTCONTROL.PORTNUMBER, 2);	// Get Ok Flag
	
		if (RIG->OKtoChange == 1)
			goto DoChange;

		if (RIG->OKtoChange == -1)
		{
			// Permission Refused. Wait Scan Interval and try again

			Debugprintf("Scan Debug %s Refused permission - waiting ScanInterval",
				RIG->PortRecord[0]->PORT_DLL_NAME); 

			RIG->WaitingForPermission = FALSE;
			SetWindowText(RIG->hSCAN, "-");

			RIG->ScanCounter = PORT->FreqPtr->Dwell; 
			return FALSE;
		}
		
		return FALSE;			// Haven't got reply yet.
	}
	else
	{
		if (RIG->PortRecord[0]->PORT_EXT_ADDR)
			RIG->WaitingForPermission = RIG->PortRecord[0]->PORT_EXT_ADDR(6, RIG->PortRecord[0]->PORTCONTROL.PORTNUMBER, 1);	// Request Perrmission
				
		// If it returns zero there is no need to wait.
				
		if (RIG->WaitingForPermission)
		{
			return FALSE;
		}		
	}

DoChange:

	// First TNC has given permission. Ask any others (these are assumed to give immediate yes/no

	i = 1;

	while (RIG->PortRecord[i])
	{
		PortRecord = RIG->PortRecord[i];

		if (PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 1))
		{
			// 1 means can't change - release all

			Debugprintf("Scan Debug %s Refused permission - waiting ScanInterval", PortRecord->PORT_DLL_NAME); 

			RIG->WaitingForPermission = FALSE;
			SetWindowText(RIG->hSCAN, "-");
			RIG->ScanCounter = PORT->FreqPtr->Dwell; 

			ReleasePermission(RIG);
			return FALSE;
		}
		i++;
	}

	SetWindowText(RIG->hSCAN, "S");
	RIG->WaitingForPermission = FALSE;

	RIG->ScanCounter = PORT->FreqPtr->Dwell; 
	
	// Do Bandwidth and antenna switches (if needed)

	DoBandwidthandAntenna(RIG, ptr[0]);

	return TRUE;
}

VOID DoBandwidthandAntenna(struct RIGINFO *RIG, struct ScanEntry * ptr)
{
	// If Bandwidth Change needed, do it

	int i;
	struct _EXTPORTDATA * PortRecord;

	if (ptr->Bandwidth || ptr->RPacketMode || ptr->HFPacketMode || ptr->PMaxLevel)
	{
		i = 0;

		while (RIG->PortRecord[i])
		{
			PortRecord = RIG->PortRecord[i];

			RIG->CurrentBandWidth = ptr->Bandwidth;

			PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, ptr);

/*			if (ptr->Bandwidth == 'R')			// Robust Packet
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 6);	// Set Robust Packet
			else 
				
			if (ptr->Bandwidth == 'W')
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 4);	// Set Wide Mode
			else
				PortRecord->PORT_EXT_ADDR(6, PortRecord->PORTCONTROL.PORTNUMBER, 5);	// Set Narrow Mode
*/
			i++;
		}
	}

	// If Antenna Change needed, do it

	if (ptr->Antenna)
	{
		SwitchAntenna(RIG, ptr->Antenna);
	}

	return;	
}

VOID ICOMPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	int i;

	struct RIGINFO * RIG;

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];

		if (RIG->ScanStopped == 0)
			if (RIG->ScanCounter)
				RIG->ScanCounter--;
	}

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		RIG = &PORT->Rigs[PORT->CurrentRig];


		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
//		SetWindowText(RIG->hFREQ, "145.810000");
//		SetWindowText(RIG->hMODE, "RTTY/1");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	PORT->CurrentRig++;

	if (PORT->CurrentRig >= PORT->ConfiguredRigs)
		PORT->CurrentRig = 0;

	RIG = &PORT->Rigs[PORT->CurrentRig];

	RIG->DebugDelay ++;

/*
	if (RIG->DebugDelay > 600)
	{
		RIG->DebugDelay = 0;
		Debugprintf("Scan Debug %d %d %d %d %d %d", PORT->CurrentRig, 
			RIG->NumberofBands, RIG->RIGOK, RIG->ScanStopped, RIG->ScanCounter,
			RIG->WaitingForPermission);
	}
*/
	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if	(GetPermissionToChange(PORT, RIG))
			{
				if (RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq/1000000.0);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, 24);
				RIG->FreqPtr++;
	
				PORT->TXLen = 11;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->AutoPoll = TRUE;

				return;
			}
		}
	}

	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr+2, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.

		if (RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq/1000000.0);


		memcpy(Poll, &buffptr[20], 12);

		if (PORT->ScanEntry.Cmd2)
		{
			PORT->ScanEntry.Cmd2 = (char *)&PORT->Line2;	// Put The Set mode Command into ScanStruct
			memcpy(PORT->Line2, &buffptr[30], 10);

			if (PORT->ScanEntry.Cmd3)
			{
				PORT->ScanEntry.Cmd3 = (char *)&PORT->Line3;
				memcpy(PORT->Line3, &buffptr[40], 10);
			}
		}

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		
		PORT->TXLen = 11;					// First send the set Freq
		RigWriteCommBlock(PORT);
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);

		PORT->AutoPoll = FALSE;

		return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it
		
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter)
			return;
	}

	if (RIG->RIGOK)
	{
		PORT->Retries = 2;
		RIG->PollCounter = 10 / PORT->ConfiguredRigs;			// Once Per Sec
	}
	else
	{
		PORT->Retries = 1;
		RIG->PollCounter = 100 / PORT->ConfiguredRigs;			// Slow Poll if down
	}

	PORT->AutoPoll = TRUE;

	// Read Frequency 

	Poll[0] = 0xFE;
	Poll[1] = 0xFE;
	Poll[2] = RIG->RigAddr;
	Poll[3] = 0xE0;
	Poll[4] = 0x3;		// Get frequency command
	Poll[5] = 0xFD;

	PORT->TXLen = 6;
	RigWriteCommBlock(PORT);
	return;
}


VOID ProcessFrame(struct RIGPORTINFO * PORT, UCHAR * Msg, int framelen)
{
	char Status[80];
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG;
	int i;

	if (Msg[0] != 0xfe || Msg[1] != 0xfe)

		// Duff Packer - return

		return;	

	if (Msg[2] != 0xe0)
	{
		// Echo - Proves a CI-V interface is attached

		if (PORT->PORTOK == FALSE)
		{
			// Just come up
			char Status[80];
		
			PORT->PORTOK = TRUE;
			wsprintf(Status,"COM%d CI-V link OK", PORT->IOBASE);
			SetWindowText(PORT->hStatus, Status);
		}
		return;
	}

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];
		if (Msg[3] == RIG->RigAddr)
			goto ok;
	}

	return;

ok:

	if (Msg[4] == 0xFB)
	{
		// Accept

		// if it was the set freq, send the set mode

		if (PORT->TXBuffer[4] == 5)
		{
			if (PORT->FreqPtr->Cmd2)
			{
				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd2, 8);
				PORT->TXLen = 8;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
			}
			else
			{
				if (!PORT->AutoPoll)
					SendResponse(RIG->Session, "Frequency Set OK");
		
				PORT->Timeout = 0;

			}

			return;
		}

		if (PORT->TXBuffer[4] == 6)
		{
			if (PORT->FreqPtr->Cmd3)
			{
				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd3, 10);
				PORT->TXLen = PORT->FreqPtr->Cmd3Len;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

			goto SetFinished;
		}

		if (PORT->TXBuffer[4] == 0x0f || PORT->TXBuffer[4] == 0x01a)	// Set DUP or Set Data
		{

SetFinished:

			// Set Mode Response - if scanning read freq, else return OK to user

			if (RIG->ScanStopped == 0)
			{
				ReleasePermission(RIG);	// Release Perrmission

				Poll[0] = 0xFE;
				Poll[1] = 0xFE;
				Poll[2] = RIG->RigAddr;
				Poll[3] = 0xE0;
				Poll[4] = 0x3;		// Get frequency command
				Poll[5] = 0xFD;

				PORT->TXLen = 6;
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

			else
				if (!PORT->AutoPoll)
					SendResponse(RIG->Session, "Frequency and Mode Set OK");
		}

		PORT->Timeout = 0;
		return;
	}

	if (Msg[4] == 0xFA)
	{
		// Reject

		PORT->Timeout = 0;

		if (!PORT->AutoPoll)
		{
			if (PORT->TXBuffer[4] == 5)
				SendResponse(RIG->Session, "Sorry - Set Frequency Command Rejected");
			else
			if (PORT->TXBuffer[4] == 6)
				SendResponse(RIG->Session, "Sorry - Set Mode Command Rejected");
			else
			if (PORT->TXBuffer[4] == 0x0f)
				SendResponse(RIG->Session, "Sorry - Set Shift Command Rejected");
		}
		return;
	}

	if (Msg[4] == PORT->TXBuffer[4])
	{
		// Response to our command

		// Any valid frame is an ACK

		RIG->RIGOK = TRUE;
		PORT->Timeout = 0;
	}
	else 
		return;		// What does this mean??


	if (PORT->PORTOK == FALSE)
	{
		// Just come up
//		char Status[80];
		
		PORT->PORTOK = TRUE;
//		wsprintf(Status,"COM%d PORT link OK", PORT->IOBASE);
//		SetWindowText(PORT->hStatus, Status);
	}

	if (Msg[4] == 3)
	{
		// Rig Frequency
		int n, j, Freq = 0, decdigit;

		for (j = 9; j > 4; j--)
		{
			n = Msg[j];
			decdigit = (n >> 4);
			decdigit *= 10;
			decdigit += n & 0xf;
			Freq = (Freq *100 ) + decdigit;
		}

		RIG->RigFreq = Freq / 1000000.0;

//		Valchar = _fcvt(FreqF, 6, &dec, &sign);
		_gcvt(RIG->RigFreq, 9, RIG->Valchar);
 
		wsprintf(Status,"%s", RIG->Valchar);
		SetWindowText(RIG->hFREQ, Status);

		// Now get Mode

			Poll[0] = 0xFE;
			Poll[1] = 0xFE;
			Poll[2] = RIG->RigAddr;
			Poll[3] = 0xE0;
			Poll[4] = 0x4;		// Get Mode
			Poll[5] = 0xFD;

		PORT->TXLen = 6;
		RigWriteCommBlock(PORT);
		PORT->Retries = 2;
		return;
	}
	if (Msg[4] == 4)
	{
		// Mode

		unsigned int Mode = Msg[5];

		if (Mode > 7) Mode = 7;

		wsprintf(Status,"%s/%d", Modes[Mode], Msg[6]);
		SetWindowText(RIG->hMODE, Status);
	}
}

VOID * APIENTRY GetBuff();

SendResponse(int Session, char * Msg)
{
	PMESSAGE Buffer = GetBuff();
	struct BPQVECSTRUC * VEC;
	struct TRANSPORTENTRY * L4 = L4TABLE;

	if (Session == -1)
		return 0;

	L4 += Session;

	Buffer->LENGTH = wsprintf((LPSTR)Buffer, "       \xf0%s\r", Msg);

	VEC = L4->L4TARGET;

	C_Q_ADD((UINT *)&L4->L4TX_Q, (UINT *)Buffer);

	PostMessage(VEC->HOSTHANDLE, BPQMsg, VEC->HOSTSTREAM, 2);  

	return 0;
}

VOID ProcessFT100Frame(struct RIGPORTINFO * PORT)
{
	// Only one we should see is a Status Message

	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int Freq;
	double FreqF;
	char Status[80];
	unsigned int Mode;
	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	// Bytes 0 is Band
	// 1 - 4 is Freq in binary in units of 1.25 HZ (!)
	// Byte 5 is Mode

	Freq =  (Msg[1] << 24) + (Msg[2] << 16) + (Msg[3] << 8) + Msg[4];
	
	FreqF = (Freq * 1.25) / 1000000;

	_gcvt(FreqF, 9, RIG->Valchar);

	wsprintf(Status,"%s", RIG->Valchar);
	SetWindowText(RIG->hFREQ, Status);

	Mode = Msg[5] & 15;

	if (Mode > 8) Mode = 8;

	wsprintf(Status,"%s", FT100Modes[Mode]);
	SetWindowText(RIG->hMODE, Status);

	if (!PORT->AutoPoll)
		SendResponse(RIG->Session, "Mode and Frequency Set OK");

}


VOID ProcessYaesuCmdAck(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	PORT->Timeout = 0;
	PORT->RXLen = 0;					// Ready for next frame	

	if (PORT->CmdSent == 1)				// Set Freq
	{
		ReleasePermission(RIG);			// Release Perrmission
		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Frequency Rejected");

			return;
		}
		else
		{
			if (RIG->ScanStopped == 0)
			{
				// Send a Get Freq - We Don't Poll when scanning

				Poll[0] = Poll[1] = Poll[2] = Poll[3] = 0;
				Poll[4] = 0x3;		// Get frequency amd mode command

				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->CmdSent = 0;
			}
			else

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Mode and Frequency Set OK");

			return;
		}
	}

	if (PORT->CmdSent == 7)						// Set Mode
	{
		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Mode Rejected");

			return;
		}
		else
		{
			// Send the Frequency
			
			memcpy(Poll, &Poll[5], 5);
			RigWriteCommBlock(PORT);
			PORT->CmdSent = Poll[4];
			PORT->Retries = 2;

			return;
		}
	}

}
VOID ProcessYaesuFrame(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int n, j, Freq = 0, decdigit;
	double FreqF;
	char Status[80];
	unsigned int Mode;

	// I'm not sure we get anything but a Command Response,
	// and the only command we send is Get Rig Frequency and Mode

	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	for (j = 0; j < 4; j++)
		{
			n = Msg[j];
			decdigit = (n >> 4);
			decdigit *= 10;
			decdigit += n & 0xf;
			Freq = (Freq *100 ) + decdigit;
		}

		FreqF = Freq / 100000.0;

//		Valchar = _fcvt(FreqF, 6, &dec, &sign);
		_gcvt(FreqF, 9, RIG->Valchar);

		wsprintf(Status,"%s", RIG->Valchar);
		SetWindowText(RIG->hFREQ, Status);

		Mode = Msg[4];

		if (Mode > 15) Mode = 15;

		wsprintf(Status,"%s", YaesuModes[Mode]);
		SetWindowText(RIG->hMODE, Status);
}

VOID YaesuPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	if (RIG->ScanStopped == 0)
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if	(GetPermissionToChange(PORT, RIG))
			{
				if (RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, 24);
				RIG->FreqPtr++;

				if (PORT->PortType == YAESU)
				{
					PORT->TXLen = 5;
					RigWriteCommBlock(PORT);
					PORT->CmdSent = Poll[4];
					PORT->Retries = 2;
					PORT->AutoPoll = TRUE;
					return;
				}

				// FT100

				PORT->TXLen = 15;			// Set Mode, Set Freq, Poll
				RigWriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->AutoPoll = TRUE;
			}
		}
	}
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr+2, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.
		
		if (RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		memcpy(Poll, &buffptr[20], datalen);

		if (PORT->PortType == YAESU)
			PORT->TXLen = 5;					// First send the set Freq
		else
			PORT->TXLen = 15;					// First send the set Freq

		RigWriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}

	if (RIG->ScanStopped == 0)
		return;						// no point in reading freq if we are about to change it
		
	// Read Frequency 

	Poll[0] = 0;
	Poll[1] = 0;
	Poll[2] = 0;
	Poll[3] = 0;
	
	if (PORT->PortType == YAESU)
		Poll[4] = 0x3;		// Get frequency amd mode command
	else
		Poll[4] = 16;		// FT100 Get frequency amd mode command

	PORT->TXLen = 5;
	RigWriteCommBlock(PORT);
	PORT->Retries = 2;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

VOID ProcessNMEA(struct RIGPORTINFO * PORT, char * Msg, int Length)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	Msg[Length] = 0;
	
	if (PORT->PORTOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		PORT->PORTOK = TRUE;
		SetWindowText(PORT->hStatus, Status);
	}

	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	if (!PORT->AutoPoll)
	{
		// Response to a RADIO Command

		if (Msg[0] == '?')
			SendResponse(RIG->Session, "Sorry - Command Rejected");
		else
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
	
		PORT->AutoPoll = TRUE;
	}

	if (memcmp(&Msg[13], "RXF", 3) == 0)
	{
		double Freq;
		char Status[80];

		if (Length < 24)
			return;

		Freq = atof(&Msg[17]);

		sprintf(Status,"%f", Freq);
		SetWindowText(RIG->hFREQ, Status);
		strcpy(RIG->Valchar, Status);

		return;
	}

	if (memcmp(&Msg[13], "MODE", 3) == 0)
	{
		char * ptr;

		if (Length < 24)
			return;

		ptr = strchr(&Msg[18], '*');
		if (ptr) *(ptr) = 0;

		SetWindowText(RIG->hMODE, &Msg[18]);
		return;
	}


}


//FA00014103000;MD2;

VOID ProcessKenwoodFrame(struct RIGPORTINFO * PORT, int Length)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
//	int n, j, Freq = 0, decdigit;
//	double FreqF;
//	char Valchar[_CVTBUFSIZE];
	char Status[80];
//	unsigned int Mode;
	char * ptr;
	int CmdLen;

	Msg[Length] = 0;
	
	if (PORT->PORTOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		PORT->PORTOK = TRUE;
		wsprintf(Status,"COM%d PORT link OK", PORT->IOBASE);
		SetWindowText(PORT->hStatus, Status);
	}

	RIG->RIGOK = TRUE;

	if (!PORT->AutoPoll)
	{
		// Response to a RADIO Command

		if (Msg[0] == '?')
			SendResponse(RIG->Session, "Sorry - Command Rejected");
		else
			SendResponse(RIG->Session, "Mode and Frequency Set OK");
	
		PORT->AutoPoll = TRUE;
		return;
	}


Loop:

	if (PORT->PortType == FLEX)
	{
		Msg += 2;						// Skip ZZ
		Length -= 2;
	}

	ptr = strchr(Msg, ';');
	CmdLen = ptr - Msg +1;

	if (Msg[0] == 'F' && Msg[1] == 'A' && CmdLen > 9)
	{
		char FreqDecimal[10];
		int F1, i;

		if (PORT->PortType == FT2000)
		{
			memcpy(FreqDecimal,&Msg[4], 6);
			Msg[4] = 0;
		}
		else
		{
			memcpy(FreqDecimal,&Msg[7], 6);
			Msg[7] = 0;
		}
		FreqDecimal[6] = 0;

		for (i = 5; i > 2; i--)
		{
			if (FreqDecimal[i] == '0')
				FreqDecimal[i] = 0;
			else
				break;
		}


		F1 = atoi(&Msg[2]);

		wsprintf(Status,"%d.%s", F1, FreqDecimal);
		SetWindowText(RIG->hFREQ, Status);
		strcpy(RIG->Valchar, Status);

		PORT->Timeout = 0;
	}
	else if (Msg[0] == 'M' && Msg[1] == 'D')
	{
		int Mode;
		
		if (PORT->PortType == FT2000)
		{
			Mode = Msg[3] - 48;
			if (Mode > 13) Mode = 13;
			SetWindowText(RIG->hMODE, FT2000Modes[Mode]);
		}
		else if (PORT->PortType == FLEX)
		{
			Mode = atoi(&Msg[3]);
			if (Mode > 12) Mode = 12;
			SetWindowText(RIG->hMODE, FLEXModes[Mode]);
		}
		else
		{
			Mode = Msg[2] - 48;
			if (Mode > 7) Mode = 7;
			SetWindowText(RIG->hMODE, KenwoodModes[Mode]);
		}
	}

	if (CmdLen < Length)
	{
		// Another Message in Buffer

		ptr++;
		Length -= (ptr - Msg);

		if (Length <= 0)
			return;

		memmove(Msg, ptr, Length +1);

		goto Loop;
	}
}


VOID KenwoodPoll(struct RIGPORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Kenwood

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			RigWriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");

		RIG->RIGOK = FALSE;

		return;
	}

	// Send Data if avail, else send poll

	if (RIG->RIGOK && RIG->ScanStopped == 0)
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			if (GetPermissionToChange(PORT, RIG))
			{
				if (RIG_DEBUG)
					Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

				memcpy(PORT->TXBuffer, PORT->FreqPtr->Cmd1, PORT->FreqPtr->Cmd1Len);
				RIG->FreqPtr++;
				PORT->TXLen = PORT->FreqPtr->Cmd1Len;

				RigWriteCommBlock(PORT);
				PORT->CmdSent = 1;
				PORT->Retries = 0;	
				PORT->Timeout = 0;
				PORT->AutoPoll = TRUE;

				// There isn't a response to a set command, so clear Scan Lock here
			
				ReleasePermission(RIG);			// Release Perrmission

			return;
			}
		}
	}
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		// Copy the ScanEntry struct from the Buffer to the PORT Scanentry

		memcpy(&PORT->ScanEntry, buffptr+2, sizeof(struct ScanEntry));

		PORT->FreqPtr = &PORT->ScanEntry;		// Block we are currently sending.
		
		if (RIG_DEBUG)
			Debugprintf("BPQ32 Change Freq to %9.4f", PORT->FreqPtr->Freq);

		DoBandwidthandAntenna(RIG, &PORT->ScanEntry);

		memcpy(Poll, &buffptr[20], datalen);

		PORT->TXLen = datalen;
		RigWriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Timeout = 0;
		RIG->PollCounter = 10;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}
		
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter > 1)
			return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0) && RIG->NumberofBands)
		return;						// no point in reading freq if we are about to change it

	RIG->PollCounter = 10;			// Once Per Sec
		
	// Read Frequency 

	PORT->TXLen = RIG->PollLen;
	strcpy(Poll, RIG->Poll);
	
	RigWriteCommBlock(PORT);
	PORT->Retries = 1;
	PORT->Timeout = 10;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}
VOID SwitchAntenna(struct RIGINFO * RIG, char Antenna)
{
	struct RIGPORTINFO * PORT;
	char Ant[3]="  ";

	if (RIG == NULL) return;

	PORT = RIG->PORT;

	Ant[1] = Antenna;

	SetWindowText(RIG->hPTT, Ant);

	switch (Antenna)
	{
	case '1':
		EscapeCommFunction(PORT->hDevice,CLRRTS);
		EscapeCommFunction(PORT->hDevice,CLRDTR);
		break;
	case '2':
		EscapeCommFunction(PORT->hDevice,CLRRTS);
		EscapeCommFunction(PORT->hDevice,SETDTR);
		break;
	case '3':
		EscapeCommFunction(PORT->hDevice,SETRTS);
		EscapeCommFunction(PORT->hDevice,CLRDTR);
		break;
	case '4':
		EscapeCommFunction(PORT->hDevice,SETRTS);
		EscapeCommFunction(PORT->hDevice,SETDTR);
		break;
	}	
}

BOOL DecodeModePtr(char * Param, double * Dwell, double * Freq, char * Mode,
				   char * PMinLevel, char * PMaxLevel, char * PacketMode,
				   char * RPacketMode, char * Split, char * Data, char * WinmorMode,
				   char * Antenna, BOOL * Supress, char * Filter)
{
	char * Context;
	char * ptr;

	*Filter = '1';			// Default

	ptr = strtok_s(Param, ",", &Context);

	// "New" format - Dwell, Freq, Params.
	
	//	Each param is a 2 char pair, separated by commas

	// An - Antenna
	// Pn - Pactor
	// Wn - Winmor
	// Pn - Packet
	// Fn - Filter
	// Sx - Split

	// 7.0770/LSB,F1,A3,WN,P1,R1 

	*Dwell = atof(ptr);
	
	ptr = strtok_s(NULL, ",", &Context);
	*Freq = atof(ptr);

	ptr = strtok_s(NULL, ",", &Context);

	if (strlen(ptr) >  5)
		return FALSE;

	strcpy(Mode, ptr); 

	ptr = strtok_s(NULL, ",", &Context);

	while (ptr)
	{
		if (ptr[0] == 'A')
			*Antenna = ptr[1];
		
		else if (ptr[0] == 'F')
			*Filter = ptr[1];

		else if (ptr[0] == 'R')
			*RPacketMode = ptr[1];
		
		else if (ptr[0] == 'H')
			*PacketMode = ptr[1];

		else if (ptr[0] == 'N')
			*PacketMode = ptr[1];

		else if (ptr[0] == 'P')
		{
			*PMinLevel = ptr[1];
			*PMaxLevel = ptr[strlen(ptr) - 1];
		}
		else if (ptr[0] == 'W')
		{
			*WinmorMode = ptr[1];
			if (*WinmorMode == '1')
				*WinmorMode = 'N';
			else if (*WinmorMode == '2')
				*WinmorMode = 'W';
		}

		else if (ptr[0] == '+')
			*Split = '+';
		else if (ptr[0] == '-')
			*Split = '-';
		else if (ptr[0] == 'S')
			*Split = 'S';
		else if (ptr[0] == 'D')
			*Data = 1;
		else if (ptr[0] == 'X')
			*Supress = TRUE;

		ptr = strtok_s(NULL, ",", &Context);
	}
	return TRUE;

}

VOID AddNMEAChecksum(char * msg)
{
	UCHAR CRC = 0;

	msg++;					// Skip $

	while (*(msg) != '*')
	{
		CRC ^= *(msg++);
	}

	wsprintf(++msg, "%02X\r\n", CRC);
}

// Called by Port Driver .dll to add/update rig info 

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1


struct RIGINFO * RigConfig(char * buf, int Port)
{
	int i;
	char * ptr;
	int COMPort;
	char * RigName;
	int RigAddr;
	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;
	struct ScanEntry ** FreqPtr;
	char * CmdPtr;
	char * Context;
	struct TimeScan * SaveBand;
	char PTTRigName[] = "PTT";
	double ScanFreq;
	double Dwell;

	_strupr(buf);

	Debugprintf("Processing RIG line %s", buf);

	ptr = strtok_s(&buf[10], " \t\n\r", &Context);

	if (ptr == NULL) return FALSE;

	if (memcmp(ptr, "DEBUG", 5) == 0)
	{
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		RIG_DEBUG = TRUE;
	}

	if (memcmp(ptr, "AUTH", 4) == 0)
	{
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if (ptr == NULL) return FALSE;
		if (strlen(ptr) > 100) return FALSE;

		strcpy(AuthPassword, ptr);
		ptr = strtok_s(NULL, " \t\n\r", &Context);
	}

	if (memcmp(ptr, "COM", 3) == 0)
		COMPort = atoi(&ptr[3]);
	else if (memcmp(ptr, "VCOM", 4) == 0)
		COMPort = atoi(&ptr[4]);
	else
		return FALSE;

	// See if port is already defined. We may be adding another radio (ICOM only) or updating an existing one

	for (i = 0; i < NumberofPorts; i++)
	{
		PORT = PORTInfo[i];

		if (PORT->IOBASE == COMPort)
			goto PortFound;
	}

	// Allocate a new one

	PORT = PORTInfo[NumberofPorts++] = malloc(sizeof(struct RIGPORTINFO));
	memset(PORT, 0, sizeof(struct RIGPORTINFO));

	PORT->IOBASE = COMPort;

PortFound:

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr == NULL) return (FALSE);

	PORT->SPEED = atoi(ptr);

	ptr = strtok_s(NULL, " \t\n\r", &Context);
	if (ptr == NULL) return (FALSE);

	if (memcmp(ptr, "PTTCOM", 6) == 0)
	{
		PORT->PTTIOBASE = atoi(&ptr[6]);
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if (ptr == NULL) return (FALSE);
	}

//		if (strcmp(ptr, "ICOM") == 0 || strcmp(ptr, "YAESU") == 0 
//			|| strcmp(ptr, "KENWOOD") == 0 || strcmp(ptr, "PTTONLY") == 0 || strcmp(ptr, "ANTENNA") == 0)

			// RADIO IC706 4E 5 14.103/U1 14.112/u1 18.1/U1 10.12/l1
			// Read RADIO Lines

	if (strcmp(ptr, "ICOM") == 0)
		PORT->PortType = ICOM;
	else
	if (strcmp(ptr, "YAESU") == 0)
		PORT->PortType = YAESU;
	else
	if (strcmp(ptr, "KENWOOD") == 0)
		PORT->PortType = KENWOOD;
	else
	if (strcmp(ptr, "FLEX") == 0)
		PORT->PortType = FLEX;
	else
	if (strcmp(ptr, "NMEA") == 0)
		PORT->PortType = NMEA;
	else
	if (strcmp(ptr, "PTTONLY") == 0)
		PORT->PortType = PTT;
	else
	if (strcmp(ptr, "ANTENNA") == 0)
		PORT->PortType = ANT;
	else
		return FALSE;

	Debugprintf("Port type = %d", PORT->PortType);

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (ptr == NULL)
	{
		if (PORT->PortType == PTT)
			ptr = PTTRigName;
		else
			return FALSE;
	}

	if (strlen(ptr) > 9) return FALSE;
	
	RigName =  ptr;

	Debugprintf("Rigname = *%s*", RigName);

	// FT100 seems to be different to most other YAESU types

	if (strcmp(RigName, "FT100") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT100;
	}

	// FT2000 seems to be different to most other YAESU types

	if (strcmp(RigName, "FT2000") == 0 && PORT->PortType == YAESU)
	{
		PORT->PortType = FT2000;
	}

	// If PTTONLY, may be defining another rig using the other control line

	if (PORT->PortType == PTT)
	{
		// See if already defined

		for (i = 0; i < PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->PortNum == Port)
				goto PTTFound;
		}

		// Allocate a new one

		RIG = &PORT->Rigs[PORT->ConfiguredRigs++];

	PTTFound:

		strcpy(RIG->RigName, RigName);
		RIG->PortNum = Port;
		RIG->BPQPort |=  (1 << Port);

		return RIG;
	}

	// If ICOM, we may be adding a new Rig

	ptr = strtok_s(NULL, " \t\n\r", &Context);

	if (PORT->PortType == ICOM || PORT->PortType == NMEA)
	{
		if (ptr == NULL) return (FALSE);
		sscanf(ptr, "%x", &RigAddr);

		// See if already defined

		for (i = 0; i < PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->RigAddr == RigAddr)
				goto RigFound;
		}

		// Allocate a new one

		RIG = &PORT->Rigs[PORT->ConfiguredRigs++];
		RIG->RigAddr = RigAddr;

	RigFound:

		ptr = strtok_s(NULL, " \t\n\r", &Context);
//		if (ptr == NULL) return (FALSE);
	}
	else
	{
		// Only allows one RIG

		PORT->ConfiguredRigs = 1;
		RIG = &PORT->Rigs[0];
	}
	
	strcpy(RIG->RigName, RigName);

	RIG->PortNum = Port;
	RIG->BPQPort |=  (1 << Port);

	if (PORT->PortType == PTT || PORT->PortType == ANT)
		return RIG;

	if (ptr == NULL) return RIG;					// No Scanning, just Interactive control
	
	if (strchr(ptr, ',') == 0)				// Old Format
	{
		ScanFreq = atof(ptr);

		#pragma warning(push)
		#pragma warning(disable : 4244)

		RIG->ScanFreq = ScanFreq * 10;

		#pragma warning(push)

		ptr = strtok_s(NULL, " \t\n\r", &Context);
	}

	// Frequency List

	if (ptr)
		if (ptr[0] == ';' || ptr[0] == '#')
			ptr = NULL;

	if (ptr != NULL)
	{
		// Create Default Timeband

		struct TimeScan * Band = AllocateTimeRec(RIG);
		SaveBand = Band;

		Band->Start = 0;
		Band->End = 84540;	//23:59
		FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);
		memset(FreqPtr, 0, 1000);
	}

	while(ptr)
	{
		int ModeNo;
		BOOL Supress;
		double Freq = 0.0;
		char FreqString[80]="";
		char * Valchar;
		char * Modeptr = NULL;
		int dec, sign;
		char Split, Data, PacketMode, RPacketMode, PMinLevel, PMaxLevel, Filter;
		char Mode[6];
		char WinmorMode, Antenna;

		if (ptr[0] == ';' || ptr[0] == '#')
			break;

		Filter = PMinLevel = PMaxLevel = PacketMode = RPacketMode = Split =
			Data = WinmorMode = Antenna = ModeNo = Supress = 0;

		Dwell = 0.0;

		if (strchr(ptr, ':'))
		{
			// New TimeBand

			struct TimeScan * Band;
						
			Band = AllocateTimeRec(RIG);

			*FreqPtr = (struct ScanEntry *)0;		// Terminate Last Band
						
			Band->Start = (atoi(ptr) * 3600) + (atoi(&ptr[3]) * 60);
			Band->End = 84540;	//23:59
			SaveBand->End = Band->Start - 60;

			SaveBand = Band;

			FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);
			memset(FreqPtr, 0, 1000);

			ptr = strtok_s(NULL, " \t\n\r", &Context);										
		}

		if (strchr(ptr, ','))			// New Format
		{
			DecodeModePtr(ptr, &Dwell, &Freq, Mode, &PMinLevel, &PMaxLevel, &PacketMode,
				&RPacketMode, &Split, &Data, &WinmorMode, &Antenna, &Supress, &Filter);
		}
		else
		{
			Modeptr = strchr(ptr, '/');
					
			if (Modeptr)
				*Modeptr++ = 0;

			Freq = atof(ptr);

			if (Modeptr)
			{
				// Mode can include 1/2/3 for Icom Filers. W/N for Winmor/Pactor Bandwidth, and +/-/S for Repeater Shift (S = Simplex) 
				// First is always Mode
				// First char is Mode (USB, LSB etc)

				Mode[0] = Modeptr[0];
				Filter = Modeptr[1];

				if (strchr(&Modeptr[1], '+'))
					Split = '+';
				else if (strchr(&Modeptr[1], '-'))
					Split = '-';
				else if (strchr(&Modeptr[1], 'S'))
					Split = 'S';
				else if (strchr(&Modeptr[1], 'D'))
					Data = 1;
						
				if (strchr(&Modeptr[1], 'W'))
				{
					WinmorMode = 'W';
					PMaxLevel = '3';
					PMinLevel = '1';
				}
				else if (strchr(&Modeptr[1], 'N'))
				{
					WinmorMode = 'N';
					PMaxLevel = '2';
					PMinLevel = '1';
				}

				if (strchr(&Modeptr[1], 'R'))		// Robust Packet
					RPacketMode = '2';				// R600
				else if (strchr(&Modeptr[1], 'H'))	// HF Packet on Tracker
					PacketMode = '1';				// 300

				if (strchr(&Modeptr[1], 'X'))		// Dont Report to WL2K
					Supress = 1;

				if (strstr(&Modeptr[1], "A1"))
						Antenna = '1';
				else if (strstr(&Modeptr[1], "A2"))
					Antenna = '2';
				if (strstr(&Modeptr[1], "A3"))
					Antenna = '3';
				else if (strstr(&Modeptr[1], "A4"))
					Antenna = '4';
				}
			}
			
			switch(PORT->PortType)
			{
			case ICOM:						
						
				for (ModeNo = 0; ModeNo < 8; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (Modes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(Modes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;

			case YAESU:						
						
				for (ModeNo = 0; ModeNo < 16; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (YaesuModes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(YaesuModes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;

			case KENWOOD:
						
				for (ModeNo = 0; ModeNo < 8; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (KenwoodModes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;

			case FLEX:
						
				for (ModeNo = 0; ModeNo < 12; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (FLEXModes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(FLEXModes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;

			case FT2000:
						
				if (Modeptr)
				{
					if (strstr(Modeptr, "PL"))
					{
						ModeNo = 8;
						break;
					}
					if (strstr(Modeptr, "PU"))
					{
						ModeNo = 12;
						break;
					}
				}
				for (ModeNo = 0; ModeNo < 13; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (FT2000Modes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(FT2000Modes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;

			case FT100:						
				
				for (ModeNo = 0; ModeNo < 8; ModeNo++)
				{
					if (strlen(Mode) == 1)
					{
						if (FT100Modes[ModeNo][0] == Mode[0])
							break;
					}
					else
					{
						if (_stricmp(FT100Modes[ModeNo], Mode) == 0)
							break;
					}
				}
				break;
		}

		Freq = Freq * 1000000.0;

		Valchar = _fcvt(Freq, 0, &dec, &sign);

		if (dec == 9)	// 10-100
			wsprintf(FreqString, "%s", Valchar);
		else
		if (dec == 8)	// 10-100
			wsprintf(FreqString, "0%s", Valchar);		
		else
		if (dec == 7)	// 1-10
			wsprintf(FreqString, "00%s", Valchar);
		else
		if (dec == 6)	// 0.1 - 1
			wsprintf(FreqString, "000%s", Valchar);
		else
		if (dec == 5)	// 0.01 - .1
			wsprintf(FreqString, "000%s", Valchar);

		FreqPtr[0] = malloc(sizeof(struct ScanEntry));
		memset(FreqPtr[0], 0, sizeof(struct ScanEntry));

		#pragma warning(push)
		#pragma warning(disable : 4244)

		if (Dwell == 0.0)
			FreqPtr[0]->Dwell = ScanFreq * 10;
		else
			FreqPtr[0]->Dwell = Dwell * 10;

		#pragma warning(pop) 

		FreqPtr[0]->Freq = Freq;
		FreqPtr[0]->Bandwidth = WinmorMode;
		FreqPtr[0]->RPacketMode = RPacketMode;
		FreqPtr[0]->HFPacketMode = PacketMode;
		FreqPtr[0]->PMaxLevel = PMaxLevel;
		FreqPtr[0]->PMinLevel = PMinLevel;
		FreqPtr[0]->Antenna = Antenna;
		FreqPtr[0]->Supress = Supress;


		CmdPtr = FreqPtr[0]->Cmd1 = malloc(100);

		if (PORT->PortType == ICOM)
		{
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = 0xFE;
			*(CmdPtr++) = RIG->RigAddr;
			*(CmdPtr++) = 0xE0;
			*(CmdPtr++) = 0x5;		// Set frequency command

			// Need to convert two chars to bcd digit
	
			*(CmdPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
			*(CmdPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
			*(CmdPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
			*(CmdPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
			*(CmdPtr++) = (FreqString[0] - 48);

			*(CmdPtr++) = 0xFD;

			if (Filter)
			{						
				CmdPtr = FreqPtr[0]->Cmd2 = malloc(10);
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = 0xFE;
				*(CmdPtr++) = RIG->RigAddr;
				*(CmdPtr++) = 0xE0;
				*(CmdPtr++) = 0x6;		// Set Mode
				*(CmdPtr++) = ModeNo;
				*(CmdPtr++) = Filter - '0'; //Filter
				*(CmdPtr++) = 0xFD;

				if (Split)
				{
					CmdPtr = FreqPtr[0]->Cmd3 = malloc(10);
					FreqPtr[0]->Cmd3Len = 7;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = 0xFE;
					*(CmdPtr++) = RIG->RigAddr;
					*(CmdPtr++) = 0xE0;
					*(CmdPtr++) = 0xF;		// Set Mode
					if (Split == 'S')
						*(CmdPtr++) = 0x10;
					else
						if (Split == '+')
							*(CmdPtr++) = 0x12;
					else
						if (Split == '-')
							*(CmdPtr++) = 0x11;
			
					*(CmdPtr++) = 0xFD;
				}
				else
				{
					if (Data)
					{
						CmdPtr = FreqPtr[0]->Cmd3 = malloc(10);
						FreqPtr[0]->Cmd3Len = 8;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = 0xFE;
						*(CmdPtr++) = RIG->RigAddr;
						*(CmdPtr++) = 0xE0;
						*(CmdPtr++) = 0x1a;		
						*(CmdPtr++) = 0x6;		// Set Data
						*(CmdPtr++) = 0x1;		//On		
						*(CmdPtr++) = 0xFD;
					}
				}
			}
		}
		else if	(PORT->PortType == YAESU)
		{	
			//Send Mode first - changing mode can change freq

			*(CmdPtr++) = ModeNo;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 7;

			*(CmdPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
			*(CmdPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
			*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
			*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
			*(CmdPtr++) = 1;

		}
		else if	(PORT->PortType == KENWOOD)
		{	
			FreqPtr[0]->Cmd1Len = wsprintf(CmdPtr, "FA00%s;MD%d;FA;MD;", FreqString, ModeNo);

			RIG->PollLen = 6;
			strcpy(RIG->Poll, "FA;MD;");

			strcpy(RIG->PTTOn, "TX0;");
			RIG->PTTOnLen = 4;
			strcpy(RIG->PTTOff, "RX;");
			RIG->PTTOffLen = 3;
		}
		else if	(PORT->PortType == FLEX)
		{	
			FreqPtr[0]->Cmd1Len = wsprintf(CmdPtr, "ZZFA00%s;ZZMD%02d;ZZFA;ZZMD;", FreqString, ModeNo);

			RIG->PollLen = 10;
			strcpy(RIG->Poll, "ZZFA;ZZMD;");

			strcpy(RIG->PTTOn, "ZZTX1;");
			RIG->PTTOnLen = 6;
			strcpy(RIG->PTTOff, "ZZTX0;");
			RIG->PTTOffLen = 6;
		}
		else if	(PORT->PortType == FT2000)
		{	
			FreqPtr[0]->Cmd1Len = wsprintf(CmdPtr, "FA%s;MD0%X;FA;MD;", &FreqString[1], ModeNo);

			RIG->PollLen = 6;
			strcpy(RIG->Poll, "FA;MD;");

			strcpy(RIG->PTTOn, "TX1;");
			RIG->PTTOnLen = 4;
			strcpy(RIG->PTTOff, "TX0;");
			RIG->PTTOffLen = 4;
		}
		else if	(PORT->PortType == FT100)
		{	
			//Send Mode first - changing mode can change freq

			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = ModeNo;
			*(CmdPtr++) = 12;

			*(CmdPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
			*(CmdPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
			*(CmdPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
			*(CmdPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
			*(CmdPtr++) = 10;

			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 0;
			*(CmdPtr++) = 16;			// Send Get Status, as FT100 doesn't ack commands
		}
		else if	(PORT->PortType == NMEA)
		{	
			int Len;
			
			i = sprintf(CmdPtr, "$PICOA,90,%02x,RXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
			AddNMEAChecksum(CmdPtr);
			Len = i;
			i = sprintf(CmdPtr + Len, "$PICOA,90,%02x,TXF,%.6f*xx\r\n", RIG->RigAddr, Freq/1000000.);
			AddNMEAChecksum(CmdPtr + Len);
			Len += i;
			i = sprintf(CmdPtr + Len, "$PICOA,90,%02x,MODE,%s*xx\r\n", RIG->RigAddr, Mode);
			AddNMEAChecksum(CmdPtr + Len);
			FreqPtr[0]->Cmd1Len = Len + i;

			i = sprintf(RIG->Poll, "$PICOA,90,%02x,RXF*xx\r\n", RIG->RigAddr);
			AddNMEAChecksum(RIG->Poll);
			Len = i;
			i = sprintf(RIG->Poll + Len, "$PICOA,90,%02x,MODE*xx\r\n", RIG->RigAddr);
			AddNMEAChecksum(RIG->Poll + Len);
			RIG->PollLen = Len + i;


//		strcpy(RIG->PTTOn, "ZZTX1;");
//			RIG->PTTOnLen = 6;
//			strcpy(RIG->PTTOff, "ZZTX0;");
//			RIG->PTTOffLen = 6;
		}


		FreqPtr++;

		RIG->ScanCounter = 20;

		ptr = strtok_s(NULL, " \t\n\r", &Context);		// Next Freq
	}

	return RIG;
}

VOID SetupScanInterLockGroups(struct RIGINFO *RIG)
{
	struct PORTCONTROL * PortRecord;

	// See if other ports in same scan/interlock group

	PortRecord = GetPortTableEntryFromPortNum(RIG->PortNum);

	if (PortRecord->PORTINTERLOCK)		// Port has Interlock defined
	{
		// Find records in same Interlock Group

		int LockGroup = PortRecord->PORTINTERLOCK;
					
		PortRecord = PORTTABLE;
					
		while (PortRecord)
		{
			if (LockGroup == PortRecord->PORTINTERLOCK)
				RIG->BPQPort |=  (1 << PortRecord->PORTNUMBER);

			PortRecord = PortRecord->PORTPOINTER;
		}
	}
}


/*
int CRow;

HANDLE hComPort, hSpeed, hRigType, hButton, hAddr, hLabel, hTimes, hFreqs, hBPQPort;

VOID CreateRigConfigLine(HWND hDlg, struct RIGPORTINFO * PORT, struct RIGINFO * RIG)
{
	char Port[10];

	hButton =  CreateWindow(WC_BUTTON , "", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
					10, CRow+5, 10,10, hDlg, NULL, hInstance, NULL);

	if (PORT->PortType == ICOM)
	{
		char Addr[10];

		wsprintf(Addr, "%X", RIG->RigAddr);

		hAddr =  CreateWindow(WC_EDIT , Addr, WS_CHILD | WS_VISIBLE  | WS_TABSTOP | WS_BORDER,
                 305, CRow, 30,20, hDlg, NULL, hInstance, NULL);

	}
	hLabel =  CreateWindow(WC_EDIT , RIG->RigName, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
                 340, CRow, 60,20, hDlg, NULL, hInstance, NULL);

	wsprintf(Port, "%d", RIG->PortRecord->PORTCONTROL.PORTNUMBER);
	hBPQPort =  CreateWindow(WC_EDIT , Port, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 405, CRow, 20, 20, hDlg, NULL, hInstance, NULL);

	hTimes =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 430, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	hFreqs =  CreateWindow(WC_EDIT , RIG->FreqText, WS_CHILD | WS_VISIBLE| WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 535, CRow, 300, 20, hDlg, NULL, hInstance, NULL);

	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "0000:1159");
	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200:2359");
	SendMessage(hTimes, CB_SETCURSEL, 0, 0);

	CRow += 30;	

}

VOID CreatePortConfigLine(HWND hDlg, struct RIGPORTINFO * PORT)
{	
	char Port[20]; 
	int i;

	hComPort =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 30, CRow, 90, 160, hDlg, NULL, hInstance, NULL);

	for (i = 1; i < 256; i++)
	{
		wsprintf(Port, "COM%d", i);
		SendMessage(hComPort, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Port);
	}

	wsprintf(Port, "COM%d", PORT->IOBASE);

	i = SendMessage(hComPort, CB_FINDSTRINGEXACT, 0,(LPARAM) Port);

	SendMessage(hComPort, CB_SETCURSEL, i, 0);
	
	
	hSpeed =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 120, CRow, 75, 80, hDlg, NULL, hInstance, NULL);

	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "2400");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "4800");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "9600");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "19200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "38400");

	wsprintf(Port, "%d", PORT->SPEED);

	i = SendMessage(hSpeed, CB_FINDSTRINGEXACT, 0, (LPARAM)Port);

	SendMessage(hSpeed, CB_SETCURSEL, i, 0);

	hRigType =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP,
                 200, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "ICOM");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "YAESU");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "KENWOOD");

	SendMessage(hRigType, CB_SETCURSEL, PORT->PortType -1, 0);

}

INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct RIGPORTINFO * PORT;
		struct RIGINFO * RIG;
		int i, p;

		CRow = 40;

		for (p = 0; p < NumberofPorts; p++)
		{
			PORT = PORTInfo[p];

			CreatePortConfigLine(hDlg, PORT);
		
			for (i=0; i < PORT->ConfiguredRigs; i++)
			{
				RIG = &PORT->Rigs[i];
				CreateRigConfigLine(hDlg, PORT, RIG);
			}
		}



//	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
//                 90, Row, 40,20, hDlg, NULL, hInstance, NULL);
	
//	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
//                 135, Row, 100,20, hDlg, NULL, hInstance, NULL);

return TRUE; 
	}

	case WM_SIZING:
	{
		return TRUE;
	}

	case WM_ACTIVATE:

//		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:

		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

*/