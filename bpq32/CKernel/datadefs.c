
#pragma data_seg("_BPQDATA")

#include "compatbits.h"
#include "asmstrucs.h"

TRANSPORTENTRY * L4TABLE = 0;
UCHAR NEXTID = 55;
int MAXCIRCUITS = 50;
int L4DEFAULTWINDOW = 4;
int L4T1 = 60;
APPLCALLS APPLCALLTABLE[NumberofAppls];
char * APPLS;

int RFOnly = 0;

int MAXRTT = 9000;			// 90 secs
int MaxHops = 4;

int RTTInterval = 24;			// 4 Minutes

BOOL IPRequired = FALSE;

char HFCTEXT[81] = "";
int HFCTEXTLEN = 0;

char LOCATOR[80] = "";			// Locator for Reporting - may be Maidenhead or LAT:LON
char MAPCOMMENT[250] = "";		// Locator for Reporting - may be Maidenhead or LAT:LON
char LOC[7] = "";				// Maidenhead Locator for Reporting
char ReportDest[7];

UCHAR BPQDirectory[260] = ".";
UCHAR BPQProgramDirectory[260]="";

UCHAR WINMOR[7] = {'W'+'W','I'+'I','N'+'N','M'+'M','O'+'O','R'+'R'};		// WINMOR IN AX25
UCHAR PACTORCALL[7] = {'P'+'P','A'+'A','C'+'C','T'+'T','O'+'O','R'+'R'};	//PACTOR IN AX25

UCHAR L3RTT[7] = {'L'+'L','3'+'3','R'+'R','T'+'T','T'+'T',0x40, 0xe0};		// L3RTT IN AX25
UCHAR L3KEEP[7] = {'K'+'K','E'+'E','E'+'E','P'+'P','L'+'L','I'+'I', 0xe0};	//  KEEPLI IN AX25

char WL2KCall[10] = "";
char WL2KLoc[7] = "";

UCHAR ROUTEQUAL = 0;

UCHAR CWTABLE[64] = {0};