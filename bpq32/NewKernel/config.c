
//	July 2010

//	BPQ32 now reads bpqcfg.txt. This module converts it to the original binary format.

//	Based on the standalonw bpqcfg.c

/************************************************************************/
/*	CONFIG.C  Jonathan Naylor G4KLX, 19th November 1988		*/
/*									*/
/*	Program to produce configuration file for G8BPQ Network Switch	*/
/*	based on the original written in BASIC by G8BPQ.		*/
/*									*/
/*	Subsequently extended by G8BPQ					*/
/*									*/
/************************************************************************/
//
//	22/11/95 - Add second port alias for digipeating (for APRS)
//		 - Add PORTMAXDIGIS param

//	1999 - Win32 Version (but should also compile in 16 bit

//	5/12/99 - Add DLLNAME Param for ext driver

//	26/11/02 - Added AUTOSAVE

// Jan 2006

//		Add params for input and output names
//		Wait before exiting if error detected

// March 2006

//		Accept # as comment delimiter
//		Display input and output filenames
//		Wait before exit, even if ok

// March 2006 

//		Add L4APPL param

// Jan 2007

//		Remove UNPROTO
//		Add BTEXT
//		Add BCALL

// Nov 2007

//		Convert calls and APPLICATIONS string to upper case

// Jan 2008

//		Remove trailing space from UNPROTO
//		Don't warn BBSCALL etc missing if APPL1CALL etc present

// August 2008

//		Add IPGATEWAY Parameter
//		Add Port DIGIMASK Parameter

// December 2008

//		Add C_IS_CHAT Parameter

// March 2009

//		Add C style COmments (/* */ at start of line)

// August 2009

//		Add INP3 flag to locked routes

// November 2009

//		Add PROTOCOL=PACTOR or WINMOR option

//	December 2009

//		Add INP3 MAXRTT and MAXHOPS Commands

// March 2010

//		Add SIMPLE mode

// March 2010

//		Change SIMPLE mode default of Full_CTEXT to 1

// April 2010

//		Add NoKeepAlive ROUTE option

// Converted to intenal bpq32 process

// Spetember 2010

// Add option of embedded port configuration 



#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asmstrucs.h"
#include <ctype.h>
#include <conio.h>
#include <math.h>


#define DllExport	__declspec(dllexport)

struct WL2KInfo * DecodeWL2KReportLine(char *  buf);

extern HANDLE hInstance;

// Dummy file routines - write to buffer instead

char * Buffer;

char * PortConfig[35];
char * RigConfigMsg[35];
char * WL2KReportLine[35];

BOOL PortDefined[35];

char * fp2;

VOID bputc(int Ch, char * ptr)
{
	*fp2++ = Ch;
}

VOID bputi(int Ch, char * ptr);

VOID bputs(char * Val, char * ptr)
{
	int Len = strlen(Val);
	memcpy(fp2, Val, Len);
	fp2 += Len;
}

VOID bseek(char * ptr, int offset, int dummy)
{
	fp2 = Buffer + offset;
}

int btell(char * ptr)
{
	return fp2 - Buffer;
}

VOID * zalloc(int len);

int WritetoConsoleLocal(char * buff);

VOID __cdecl Consoleprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\n");
	WritetoConsoleLocal(Mess);

	return;
}

#pragma pack(1) 

struct CONFIGTABLE
{

//	CONFIGURATION DATA STRUCTURE

//	DEFINES LAYOUT OF CONFIG RECORD PRODUCED BY CONFIGURATION PROG

//	LAYOUT MUST MATCH THAT IN CONFIG.C SOURCE

	char C_NODECALL[10];		// OFFSET = 0 
	char C_NODEALIAS[10];		// OFFSET = 10
	char C_BBSCALL[10];			// OFFSET = 20
	char C_BBSALIAS[10];		// OFFSET = 30

	short C_OBSINIT;			// OFFSET = 40
	short C_OBSMIN;				// OFFSET = 42
	short C_NODESINTERVAL;		// OFFSET = 44
	short C_L3TIMETOLIVE;		// OFFSET = 46
	short C_L4RETRIES;			// OFFSET = 48
	short C_L4TIMEOUT;			// OFFSET = 50
	short C_BUFFERS;			// OFFSET = 52
	short C_PACLEN;				// OFFSET = 54
	short C_TRANSDELAY;			// OFFSET = 56
	short C_T3;					// OFFSET = 58
	short Spare1;				// OFFSET = 60
	short Spare2;				// OFFSET = 62
	short C_IDLETIME;			// OFFSET = 64
	UCHAR C_EMSFLAG;			// OFFSET = 66
	UCHAR C_LINKEDFLAG;			// OFFSET = 67
	UCHAR C_BBS;				// OFFSET = 68
	UCHAR C_NODE;				// OFFSET = 69
	UCHAR C_HOSTINTERRUPT;		// OFFSET = 70
	UCHAR C_DESQVIEW;			// OFFSET = 71
	short C_MAXLINKS;			// OFFSET = 72
	short C_MAXDESTS;
	short C_MAXNEIGHBOURS;
	short C_MAXCIRCUITS;		// 78
	UCHAR C_TNCPORTLIST[16];	// OFFSET = 80
	short C_IDINTERVAL;			// 96
	short C_FULLCTEXT;			// 98    ; SPARE (WAS DIGIFLAG)
	short C_MINQUAL;			// 100
	UCHAR C_HIDENODES;			// 102
	short C_L4DELAY;			// 103
	short C_L4WINDOW;			// 105
	short C_BTINTERVAL;			// 107
	UCHAR C_AUTOSAVE;			// 109
	UCHAR C_L4APPL;				// 110
	UCHAR C_C;					//  111 "C" = HOST Command Enabled
	UCHAR C_IP;					//  112 IP Enabled
	UCHAR C_MAXRTT;				// 113
	UCHAR C_MAXHOPS;			// 114
	UCHAR Spare[3];				// 115 NOW SPARE
	short C_BBSQUAL;			// 118
	UCHAR C_WASUNPROTO;
	UCHAR C_BTEXT[120];			// 121
	char C_VERSTRING[10];		// 241 Version String from Config File
	char Spare4[4];				// 251
	UCHAR C_VERSION;			// CONFIG PROG VERSION
};

struct PORTCONFIG
{
	short PORTNUM;
	char ID[30];			//2
	short TYPE;			    // 32,
	short PROTOCOL;			// 34,
	short IOADDR;			// 36,
	short INTLEVEL;			// 38,
	short SPEED;			// 40,
	short CHANNEL;			// 42,
	short BBSFLAG;			// 44, 
	short QUALITY;			// 46, 
	short MAXFRAME;			// 48,
	short TXDELAY;			// 50,
	short SLOTTIME;			// 52, 
	short PERSIST;			// 54,

	short FULLDUP;			// 56,
	short SOFTDCD;			// 58, 
	short FRACK;			// 60, 
	short RESPTIME;			// 62,
	short RETRIES;			// 64, 

	short PACLEN;			// 66,
	short QUALADJUST;		// 68,
	UCHAR DIGIFLAG;			// 70,
	UCHAR DIGIPORT;			// 71 
	short DIGIMASK;			// 72
	short USERS;			// 74,
	short TXTAIL;			// 76
	short ALIAS_IS_BBS;		// 78
	char CWID[10];			// 80,
	char PORTCALL[10];		//  90,
	char PORTALIAS[10];		// 100,
	short L3ONLY;			//  110,
	short KISSOPTIONS;		//"112,
	short INTERLOCK;		// 114,
	short NODESPACLEN;		//  116,
	short TXPORT;			// 118,
	UCHAR MHEARD;			// 120,
	UCHAR CWIDTYPE;			// 121,
	char MINQUAL;			// 122, 
	char MAXDIGIS;			//  123,
	char DefaultNoKeepAlives; // 124
	char UIONLY;			// 125,
	char Pad[2];			// 126-7,
	char UNPROTO[72];		//  128, 
	char PORTALIAS2[10];	//  200,
	char DLLNAME[16];		//  210,
	char BCALL[10];			// 226,
	unsigned long IPADDR;	// 236
	char Pad2[16];			// 240
	char VALIDCALLS[256];	//   256,
};

struct PORTCONFIG * PortRec;

#pragma pack()

int tnctypes(int i, char value[],char rec[]);
int do_kiss (char value[],char rec[]);
int dec_byte(i, value, rec);

extern char PWTEXT[];
extern char HFCTEXT[];
extern int HFCTEXTLEN;
extern char LOCATOR[];
extern char MAPCOMMENT[];
extern char LOC[];
extern int RFOnly = TRUE;

extern int __cdecl main(int argc,char **argv,char **envp);
extern int __cdecl decode_rec(char *rec);
extern int __cdecl applcallsign(int i,char *value,char *rec);
extern int __cdecl appl_qual(int i,char *value,char *rec);
extern int __cdecl callsign(int i,char *value,char *rec);
extern int __cdecl int_value(int i,char *value,char *rec);
extern int __cdecl hex_value(int i,char *value,char *rec);
extern int __cdecl bin_switch(int i,char *value,char *rec);
extern int __cdecl dec_switch(int i,char *value,char *rec);
extern int __cdecl numbers(int i,char *value,char *rec);
extern int __cdecl applstrings(int i,char *value,char *rec);
extern int __cdecl dotext(int i,char *key_word, int max);
extern int __cdecl doBText(int i,char *key_word);
int routes(int i);
int ports(int i);
extern int __cdecl tncports(int i);
extern int __cdecl dedports(int i);
extern int __cdecl index(char *s,char *t);
extern int __cdecl verify(char *s,char c);
int GetNextLine(char * rec);
int call_check(char *callsign);
int call_check_internal(char * callsign);
extern int __cdecl callstring(int i,char *value,char *rec);
int decode_port_rec(char *rec);
extern int __cdecl doid(int i,char *value,char *rec);
extern int __cdecl dodll(int i,char *value,char *rec);
extern int __cdecl doDriver(int i,char *value,char *rec);
extern int __cdecl hwtypes(int i,char *value,char *rec);
extern int __cdecl protocols(int i,char *value,char *rec);
extern int __cdecl bbsflag(int i,char *value,char *rec);
extern int __cdecl channel(int i,char *value,char *rec);
extern int __cdecl validcalls(int i,char *value,char *rec);
extern int __cdecl kissoptions(int i,char *value,char *rec);
extern int __cdecl decode_tnc_rec(char *rec);
extern int __cdecl tnctypes(int i,char *value,char *rec);
extern int __cdecl dec_byte(int i,char *value,char *rec);
extern int __cdecl do_kiss(char *value,char *rec);
extern int __cdecl decode_ded_rec(char *rec);
extern int __cdecl simple(int i);

BOOL ProcessAPPLDef(char * rec);
BOOL ToLOC(double Lat, double Lon , char * Locator);

//int i;
//char value[];
//char rec[];

int FIRSTAPPL=1;
BOOL Comment = FALSE;

#define PARAMLIM 74
#define MAXLINE 512
#define FILEVERSION 22

extern UCHAR BPQDirectory[MAX_PATH];


char inputname[250]="bpqcfg.txt";
char option[250];

/************************************************************************/
/*      STATIC VARIABLES                                                */
/************************************************************************/

static char *keywords[] = 
{
"OBSINIT", "OBSMIN", "NODESINTERVAL", "L3TIMETOLIVE", "L4RETRIES", "L4TIMEOUT",
"BUFFERS", "PACLEN", "TRANSDELAY", "T3", "IDLETIME", "BBS",
"NODE", "NODEALIAS", "BBSALIAS", "NODECALL", "BBSCALL",
"TNCPORT", "IDMSG:", "INFOMSG:", "ROUTES:", "PORT",  "MAXLINKS",
"MAXNODES", "MAXROUTES", "MAXCIRCUITS", "IDINTERVAL", "MINQUAL",
"HIDENODES", "L4DELAY", "L4WINDOW", "BTINTERVAL", "UNPROTO", "BBSQUAL",
"APPLICATIONS", "EMS", "CTEXT:", "DESQVIEW", "HOSTINTERRUPT", "ENABLE_LINKED",
"DEDHOST", "FULL_CTEXT", "SIMPLE", "AUTOSAVE" , "L4APPL",
"APPL1CALL", "APPL2CALL", "APPL3CALL", "APPL4CALL",
"APPL5CALL", "APPL6CALL", "APPL7CALL", "APPL8CALL",
"APPL1ALIAS", "APPL2ALIAS", "APPL3ALIAS", "APPL4ALIAS",
"APPL5ALIAS", "APPL6ALIAS", "APPL7ALIAS", "APPL8ALIAS",
"APPL1QUAL", "APPL2QUAL", "APPL3QUAL", "APPL4QUAL",
"APPL5QUAL", "APPL6QUAL", "APPL7QUAL", "APPL8QUAL",
"BTEXT:", "NETROMCALL", "C_IS_CHAT", "MAXRTT", "MAXHOPS"		// IPGATEWAY= no longer allowed
};           /* parameter keywords */

static int offset[] =
{
40, 42, 44, 46, 48, 50,
52, 54, 56, 58, 64, 68,
69, 10, 30, 0, 20,
384, 512, InfoOffset, 1536, 2560, 72,
74, 76, 78, 96, 100,
102, 103, 105, 107, 120, 118,
ApplOffset, 66, 2048, 71, 70, 67,
3000, 98, 0, 109, 110,
0,10,20,30,
40,50,60,70,
80,90,100,110,
120,130,140,150,
160,162,164,166,
168,170,172,174,
121, 256, 111, 113, 114						// BTEXT was UNPROTO+1
};		/* offset for corresponding data in config file */

static int routine[] = 
{
1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 2,
2, 0, 0, 0, 0,
3, 4, 20, 5, 6, 1,
1, 1, 1, 1, 1,
2, 1, 1, 1, 7, 1,
8, 2, 4, 2, 9, 10,
11, 1, 12, 2 , 1,
13, 13, 13, 13,
13, 13 ,13, 13,
13, 13, 13, 13,
13, 13 ,13, 13,
14, 14, 14, 14,
14, 14 ,14, 14,
15, 0, 2, 9, 9
} ;			// Routine to process param

static char eof_message[] = "Unexpected end of file on input\n";

static char *pkeywords[] = 
{
"ID", "TYPE", "PROTOCOL", "IOADDR", "INTLEVEL", "SPEED", "CHANNEL",
"BBSFLAG", "QUALITY", "MAXFRAME", "TXDELAY", "SLOTTIME", "PERSIST",
"FULLDUP", "SOFTDCD", "FRACK", "RESPTIME", "RETRIES",
"PACLEN", "CWID", "PORTCALL", "PORTALIAS", "ENDPORT", "VALIDCALLS",
"QUALADJUST", "DIGIFLAG", "DIGIPORT", "USERS" ,"UNPROTO", "PORTNUM",
"TXTAIL", "ALIAS_IS_BBS", "L3ONLY", "KISSOPTIONS", "INTERLOCK", "NODESPACLEN",
"TXPORT", "MHEARD", "CWIDTYPE", "MINQUAL", "MAXDIGIS", "PORTALIAS2", "DLLNAME",
"BCALL", "DIGIMASK", "NOKEEPALIVES", "COMPORT", "DRIVER", "WL2KREPORT", "UIONLY",
"UDPPORT", "IPADDR"};           /* parameter keywords */

static int poffset[] =
{
2, 32, 34, 36, 38, 40, 42,
44, 46, 48, 50, 52, 54,
56, 58, 60, 62, 64, 
66, 80, 90, 100, 127, 256,
68, 70, 71 ,74, 128, 0,
76, 78, 110, 112, 114, 116,
118, 120, 121, 122, 123, 200, 210,
226, 72, 124, 36, 210, 512, 125,
36, 236};						/* offset for corresponding data in config file */

static int proutine[] = 
{
4, 5, 8, 3, 1, 1, 7,
6, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1,
1, 0, 0, 0, 9, 10,
1, 13, 13, 1, 11, 1,
1, 2, 2, 12, 1, 1,
1, 7, 7, 13, 13, 0, 14,
0, 1, 2, 1, 15, 16, 2,
1, 17};							/* routine to process parameter */

#define PPARAMLIM 52

static int fileoffset = 0;
static int portoffset = 2560;
static int tncportoffset = 384;
static int endport = 0;
static int portnum = 1;
static int porterror = 0;
static int tncporterror = 0;
static int dedporterror = 0;
static int kissflags = 0;
static int NextAppl = 0;


/************************************************************************/
/* Global variables							*/
/************************************************************************/

int paramok[PARAMLIM];		/* PARAMETER OK FLAG  */

FILE *fp1;			/* TEXT INPUT FILE    */

static char s1[80];
static char s2[80];
static char s3[80];
static char s4[80];
static char s5[80];
static char s6[80];
static char s7[80];
static char s8[80];

char commas[]=",,,,,,,,,,,,,,,,";

char bbscall[11];
char bbsalias[11];
int bbsqual;

BOOL LocSpecified = FALSE;

/************************************************************************/
/*   MAIN PROGRAM							*/
/************************************************************************/
 
VOID WarnThread();

int heading = 0;

DllExport BOOL ProcessConfig()
{
	int i;
	char rec[MAXLINE];
	int Cfglen = sizeof(struct CONFIGTABLE);
	struct APPLCONFIG * App;

 	heading = 0;
	fileoffset = 0;
	portoffset = 2560;
	tncportoffset = 384;
	portnum = 1;
	NextAppl = 0;
	LOCATOR[0] = 0;
	MAPCOMMENT[0] = 0;

	for (i = 0; i < 35; i++)
	{
		if (PortConfig[i])
		{
			free(PortConfig[i]);
			PortConfig[i] = NULL;
		}
		if (RigConfigMsg[i])
		{
			free(RigConfigMsg[i]);
			RigConfigMsg[i] = NULL;
		}
		PortDefined[i] = FALSE;
	}

	Consoleprintf("Configuration file Preprocessor.");

	if (BPQDirectory[0] == 0)
	{
		strcpy(inputname, "bpq32.cfg");
	}
		else
	{
		strcpy(inputname,BPQDirectory);
		strcat(inputname,"\\");
		strcat(inputname, "bpq32.cfg");
	}

	if ((fp1 = fopen(inputname,"r")) == NULL)
	{
		Consoleprintf("Could not open file %s",inputname);
		return FALSE;
	}

	Consoleprintf("Using Configuration file %s",inputname);

	Buffer = malloc(100000);

	memset(Buffer, 0, 100000);

	App = (struct APPLCONFIG *)&Buffer[ApplOffset];

	for (i=0; i < NumberofAppls; i++)
	{
		memset(App->Command, ' ', 12);
		memset(App->CommandAlias, ' ', 48);
		memset(App->ApplCall, ' ', 10);
		memset(App->ApplAlias, ' ', 10);

		App++;
	}

	fp2 = Buffer;

	GetNextLine(rec);

	while (rec[0])
	{
	   decode_rec(rec);

	   GetNextLine(rec);
	}

	paramok[8]=1;          /* dont need TRANSDELAY */
	paramok[17]=1;          /* dont need TNCPORTS */

	paramok[32]=1;          /* dont need UNPROTO */

	paramok[35]=1;          /* dont need EMS */
	paramok[37]=1;          /* dont need DESQVIEW */
	paramok[38]=1;          /* dont need HOSTINTERRUPT */

	paramok[40]=1;			/* or DEDHOST */

	paramok[42]=1;			/* or SIMPLE */

	paramok[43]=1;			/* or AUTOSAVE */

	paramok[44]=1;			/* or L4APPL */


	paramok[16]=1;	//  BBSCALL
	paramok[14]=1;	//  BBSALIAS
	paramok[33]=1;	//  BBSQUAL
	paramok[34]=1;	//  APPLICATIONS

	if (paramok[45]==1)
	{
		paramok[16]=1;	//  APPL1CALL overrides BBSCALL
		bseek(fp2,(long) 20,SEEK_SET);

		for (i=0; i<10; i++)
		{
			if (bbscall[i] == 0)
				
				bputc(32,fp2);
			else
				bputc(bbscall[i],fp2);
		}
			
	}
	
	if (paramok[53]==1)
	{
		paramok[14]=1;	//  APPL1ALIAS overrides BBSALIAS
		bseek(fp2,(long) 30,SEEK_SET);

		for (i=0; i<10; i++)
		{
			if (bbsalias[i] == 0)
				
				bputc(32,fp2);
			else
				bputc(bbsalias[i],fp2);

		}
			
	}

	if (paramok[61]==1) 
	{
		paramok[33]=1;	//  APPL1QUAL overrides BBSQUAL
		bseek(fp2,(long) 118,SEEK_SET);
		bputi(bbsqual,fp2);
	}
			
	
	for (i=0;i<24;i++)
		
		paramok[45+i]=1;	/* or APPLCALLS, APPLALIASS APPLQUAL */

	paramok[69]=1;			// BText optional
	paramok[70]=1;			// IPGateway optional
	paramok[71]=1;			// C_IS_CHAT optional

	paramok[72]=1;			// MAXRTT optional
	paramok[73]=1;			// MAXHOPS optional

	for (i=0; i < PARAMLIM; i++)
	{
	   if (paramok[i] == 0)
	   {
	      if (heading == 0)
	      {
		 Consoleprintf("\r\nThe following parameters were not correctly specified");
         	 heading = 1;
      	      }
	      Consoleprintf(keywords[i]);
   	}
	}

        if (portnum == 1)
	{
	   Consoleprintf("No valid radio ports defined");
	   heading = 1;
	}

	bseek(fp2,(long) 255,SEEK_SET);
	bputc(FILEVERSION,fp2);

	if (Comment)
	{
		Consoleprintf("\nUnterminated Comment (Missing */)");
		heading = 1;
	}

	fclose(fp1);

	if (LOCATOR[0] == 0 && LocSpecified == 0 && RFOnly == 0)
	{
		Consoleprintf("");
		Consoleprintf("Please enter a LOCATOR statment in your BPQ32.cfg");
		Consoleprintf("If you really don't want to be on the Node Map you cam enter LOCATOR=NONE");
		Consoleprintf("");

//		_beginthread(WarnThread, 0, 0);
	}

	if (heading == 0)
	{
	   Consoleprintf("Conversion (probably) successful");
	   Consoleprintf("");
	}
	else
	{
   	   Consoleprintf("Conversion failed");
   	   return FALSE;
	}
/*
	{
		char fn[200];
		HANDLE handle;
		int cnt;

		wsprintf(fn,"%s\\CFDUMP1",BPQDirectory);

		handle = CreateFile(fn,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	
		WriteFile(handle,Buffer,100000,&cnt,NULL);

	 	CloseHandle(handle);
	}
  */	
	return TRUE;
}

static WNDPROC wpOrigWarnProc; 


LRESULT APIENTRY WarnProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	return DefWindowProc(hwnd, uMsg, wParam, lParam); 

} 


VOID WarnThread()
{
	HWND hWnd = CreateWindow("STATIC",
		"Please enter a LOCATOR statment in your BPQ32.cfg\rIf you really don't want to be on the Node Map you can enter LOCATOR=NONE",
		0, 100, 100, 550, 100, NULL, NULL, hInstance, NULL);

	wpOrigWarnProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LONG)WarnProc);

	ShowWindow(hWnd, SW_SHOWNORMAL);
	InvalidateRect(hWnd, NULL, TRUE);

	Sleep(30000);
	DestroyWindow(hWnd);
}


/************************************************************************/
/*	Decode PARAM=							*/
/************************************************************************/

decode_rec(rec)
char rec[];
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */

	char key_word[20] = "";
	char value[300] = "";

	if (_memicmp(rec, "IPGATEWAY", 9) == 0 && rec[9] != '=')	// IPGATEWAY, not IPGATEWAY=
	{
		// Create Embedded IPGateway Config

		// Copy all subsequent lines up to **** to a memory buffer

		char * ptr;
		struct CONFIGTABLE * cfg = (struct CONFIGTABLE * )Buffer;
		
		PortConfig[33] = ptr = malloc(50000);

		*ptr = 0;

		GetNextLine(rec);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "****", 3) == 0)
			{
				PortConfig[33] = realloc(PortConfig[33], (strlen(ptr) + 1));
				cfg->C_IP = 1;
				return 0;
			}

			strcat(ptr, rec);
			strcat(ptr, "\r\n");
			GetNextLine(rec);
		}

		Consoleprintf("Missing **** for IPGateway Config %d", portnum);
		heading = 1;

		return 0;
	}

	if (_memicmp(rec, "APRSDIGI", 8) == 0)
	{		
		// Create Embedded APRS Config

		// Copy all subsequent lines up to **** to a memory buffer

		char * ptr;
		struct CONFIGTABLE * cfg = (struct CONFIGTABLE * )Buffer;
		
		PortConfig[34] = ptr = malloc(50000);

		*ptr = 0;

		// Don't use GetNextLine - we need to keep ; in messages
		
		fgets(rec,MAXLINE,fp1);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "****", 3) == 0)
			{
				PortConfig[34] = realloc(PortConfig[34], (strlen(ptr) + 1));
				return 0;
			}
			if (strlen(rec) > 1)
			{
				if (strstr(rec, "/*"))
				{
					Comment = TRUE;
					goto NextAPRS;
				}
				else if (strstr(rec, "*/"))
				{
					Comment = FALSE;
					goto NextAPRS;
				}
			}
			
			if (Comment)
				goto NextAPRS;

			strcat(ptr, rec);
			strcat(ptr, "\r\n");
NextAPRS:
			fgets(rec,MAXLINE,fp1);
		}

		if (_memicmp(rec, "****", 3) == 0)
			return 0;						// No Newline after ***

		Consoleprintf("Missing **** for APRS Config %d", portnum);
		heading = 1;

		return 0;
	}

	if (_memicmp(rec, "PASSWORD", 8) == 0)
	{
		// SYSOP Password

		if (strlen(rec) > 88) rec[88] = 0;

		_strupr(rec);
		
		strcpy(PWTEXT, &rec[9]);
		return 0;
	}

	if (_memicmp(rec, "HFCTEXT", 7) == 0)
	{
		// HF only CTEXT (normlly short to reduce traffic)

		if (strlen(rec) > 87) rec[87] = 0;
		strcpy(HFCTEXT, &rec[8]);
		HFCTEXTLEN = strlen(HFCTEXT);
		HFCTEXT[HFCTEXTLEN - 1] = '\r';
		return 0;
	}

	if (_memicmp(rec, "LOCATOR", 7) == 0)
	{
		// Station Maidenhead Locator or Lat/Long

		char * Context;		
		char * ptr1 = strtok_s(&rec[7], " ,=\t\n\r:", &Context);
		char * ptr2 = strtok_s(NULL, " ,=\t\n\r:", &Context);

		LocSpecified = TRUE;

		if (_memicmp(&rec[8], "NONE", 4) == 0)
			return 0;

		if (ptr1)
		{
			strcpy(LOCATOR, ptr1);
			if (ptr2)
			{
				strcat(LOCATOR, ":");
				strcat(LOCATOR, ptr2);
				ToLOC(atof(ptr1), atof(ptr2), LOC);
			}
			else
			{
				if (strlen(ptr1) == 6)
					strcpy(LOC, ptr1);
			}
		}
		return 0;
	}

	if (_memicmp(rec, "MAPCOMMENT", 10) == 0)
	{
		// Additional Info for Node Map

		char * ptr1 = &rec[11];
		char * ptr2 = MAPCOMMENT;

		while (*ptr1)
		{
			if (*ptr1 == ',')
			{
				*ptr2++ = '&';
				*ptr2++ = '#';
				*ptr2++ = '4';
				*ptr2++ = '4';
				*ptr2++ = ';';
			}
			else
				*(ptr2++) = *ptr1;

			ptr1++;

			if ((ptr2 - MAPCOMMENT) > 248)
				break;
		}

		*ptr2 = 0;

		return 0;
	}

	if (_memicmp(rec, "APPLICATION ", 12) == 0 || _memicmp(rec, "APPLICATION=", 12) == 0)
	{
		// New Style APPLICATION Definition

		char save[300];

		strcpy(save, rec);			// Save in case error
		
		if (!ProcessAPPLDef(&rec[12]))
		{
			Consoleprintf("Invalid Record %s", save);
			heading = 1;
		}
		else
				paramok[34]=1;		// Got APPLICATIONS

		return 0;
	}

	if (index(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0;  i < PARAMLIM && _stricmp(keywords[i],key_word) != 0 ; i++)
	   ;

	if (i == PARAMLIM)
	   Consoleprintf("Source record not recognised - Ignored: %s",rec);
	else
	{
	   fileoffset = offset[i];
	   switch (routine[i])
           {
             case 0:
		cn = callsign(i,value,rec);          /* CALLSIGNS */
         	break;

             case 1:
		cn = int_value(i,value,rec);	     /* INTEGER VALUES */
		break;

             case 2:
              	cn = bin_switch(i,value,rec);        /* 0/1 SWITCHES */
		break;

             case 3:
		cn = tncports(i);	             /* VIRTUAL COMBIOS PORTS */
		break;

             case 4:
             	cn = dotext(i,key_word, 510);             /* TEXT PARMS */
		break;

             case 20:
             	cn = dotext(i,key_word, InfoMax);         /* INFO TEXT PARM */
		break;

			 case 5:
             	cn = routes(i);                      /* ROUTES TO LOCK IN */
		break;

             case 6:
              	cn = ports(i);                       /* PORTS DEFINITION */
		break;

             case 7:
				 Consoleprintf("Obsolete Record %s ignored",rec);
				 Consoleprintf("UNPROTO address should now be specified in PORT definition");

				 break;

             case 8:
              	cn = applstrings(i,value,rec);        /* APPLICATIONS LIST */
		break;

             case 9:
              	cn = dec_switch(i,value,rec);        /* 0/9 SWITCHES */
		break;

             case 10:
             	cn = channel(i,value,rec);	     /* SINGLE CHAR  */	
		break;

             case 11:
		cn = dedports(i);	             /* VIRTUAL COMBIOS PORTS */
		break;

	     case 12:
		cn = simple(i);			   /* Set up basic L2 system*/
		break;

	   case 13:

		   cn = applcallsign(i,value,rec);          /* CALLSIGNS */
		   break;

       case 14:

		   cn = appl_qual(i,value,rec);	     /* INTEGER VALUES */
		   break;


	   case 15:   
		  cn = doBText(i,key_word);             /* BTEXT */
		  break;
 
	     }

	   paramok[i] = cn;
	}

	return 0;
}

/************************************************************************/
/*   CALLSIGNS								*/
/************************************************************************/
int applcallsign(i, value, rec)
int i;
char value[];
char rec[];
{
	int Appl;
	struct APPLCONFIG * App;

	// Appl1Call is 0, Appl1Alias 800, 10 byte values 
	
	Appl = fileoffset/10;

	if (call_check_internal(value))
	{
		// Invalid

		return 0;
	}

	App = (struct APPLCONFIG *)&Buffer[ApplOffset + Appl * sizeof(struct APPLCONFIG) ];	if (Appl > 7)
	{
		// Aliases

		App -= 8;
		memcpy(App->ApplAlias, value, 10);
	}
	else
	{
		memcpy(App->ApplCall, value, 10);
	}

	if (i==45)
		strcpy(bbscall,value);
	if (i==53)
		strcpy(bbsalias,value);

	return 1;
}

int appl_qual(i, value, rec)
int i;
char value[];
char rec[];
{
	int j, k, Appl;
	struct APPLCONFIG * App;

	// Appl1Qual is 160, 2 byte values 
	
	Appl = (fileoffset - 160) / 2;

	App = (struct APPLCONFIG *)&Buffer[ApplOffset + Appl * sizeof(struct APPLCONFIG) ];

	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   Consoleprintf("Invalid numerical value ");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
	
	if (i==61) bbsqual=j;

	App->ApplQual = j;
	return(1);
}


int callsign(i, value, rec)
int i;
char value[];
char rec[];
{
	bseek(fp2,(long) fileoffset,SEEK_SET);

	if (call_check(value) == 1)
	{
	   Consoleprintf("%s",rec);
	   return(0);
	}

	return(1);
}


/************************************************************************/
/*   VALIDATE INT VALUES						*/
/************************************************************************/

int int_value(i, value, rec)
int i;
char value[];
char rec[];
{
	int j,k;

	bseek(fp2,(long) fileoffset,SEEK_SET);

	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   Consoleprintf("Invalid numerical value ");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}

	bputi(j,fp2);
	return(1);
}


/************************************************************************/
/*   VALIDATE HEX INT VALUES						*/
/************************************************************************/

int hex_value(i, value, rec)
int i;
char value[];
char rec[];
{
	int j = -1;
	int k = 0;
	bseek(fp2,(long) fileoffset,SEEK_SET);

	k = sscanf(value," %xH",&j);

	if (j < 0)
	{
	   Consoleprintf("Bad Hex Value");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}

	bputi(j,fp2);
	return(1);
}


/************************************************************************/
/*   VALIDATE BINARY SWITCH DATA AND WRITE TO FILE			*/
/************************************************************************/

int bin_switch(i, value, rec)
int i;
char value[];
char rec[];
{
	int value_int;

	value_int = atoi(value);

	if (value_int == 0 || value_int == 1)
	{
	   bseek(fp2,(long) fileoffset,SEEK_SET);
	   bputc(value_int,fp2);
	   return(1);
	}
	else
	{
	   Consoleprintf("Invalid switch value, must be either 0 or 1");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
}
/*
;	single byte decimal
*/
int dec_switch(i, value, rec)
int i;
char value[];
char rec[];
{
	int value_int;

	value_int = atoi(value);

	if (value_int < 256 )
	{
	   bseek(fp2,(long) fileoffset,SEEK_SET);
	   bputc(value_int,fp2);
	   return(1);
	}
	else
	{
	   Consoleprintf("Invalid value, must be between 0 and 255");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}
}


/************************************************************************/
/*    VALIDATE AND CONVERT NUMBERS TO BINARY				*/
/************************************************************************/

int numbers(i, value, rec)
int i;
char value[];
char rec[];
{
	int j1 = 0;
	int j2 = 0;
	int j3 = 0;
	int j4 = 0;
	int j5 = 0;
	int j6 = 0;
	int j7 = 0;
	int j8 = 0;
	int j9 = 0;
	int j10 = 0;
	int j11 = 0;
	int j12 = 0;
	int j13 = 0;
	int j14 = 0;
	int j15 = 0;
	int j16 = 0;
	int num;
	
        bseek(fp2,(long) fileoffset,SEEK_SET);

	num = sscanf(value," %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",&j1,&j2,&j3,&j4,&j5,&j6,&j7,&j8,&j9,&j10,&j11,&j12,&j13,&j14,&j15,&j16);

	bputc(j1,fp2);
	bputc(j2,fp2);
	bputc(j3,fp2);
	bputc(j4,fp2);
	bputc(j5,fp2);
	bputc(j6,fp2);
	bputc(j7,fp2);
	bputc(j8,fp2);
	bputc(j9,fp2);
	bputc(j10,fp2);
	bputc(j11,fp2);
	bputc(j12,fp2);
	bputc(j13,fp2);
	bputc(j14,fp2);
	bputc(j15,fp2);
	bputc(j16,fp2);

	return(1);
}

int applstrings(i, value, rec)
int i;
char value[];
char rec[];
{
	char appl[250];	// In case trailing spaces
	struct CONFIGTABLE * cfg = (struct CONFIGTABLE *)Buffer;
	char * ptr1;
	char * ptr2;
	struct APPLCONFIG * App;
	int j;

 //  strcat(rec,commas);		// Ensure 16 commas

   ptr1=&rec[13];		// skip APPLICATIONS=

   while (NextAppl < NumberofAppls)
   {
	   App = (struct APPLCONFIG *)&Buffer[ApplOffset + NextAppl++ * sizeof(struct APPLCONFIG) ];

       memset(appl, ' ', 249);
	   appl[249] = 0;

	   ptr2=appl;
 		
       j = *ptr1++;

       while (j != ',' && j)
       {
		   *(ptr2++) = toupper(j);
		   j = *ptr1++;
	   }

	   ptr2 = strchr(appl, '/');

	   if (ptr2)
	   {
		   // Command has an Alias

		   *ptr2++ = 0;
		   memcpy(App->CommandAlias, ptr2, 48);
		   strcat(appl, "            ");
	   }
	   
	   memcpy(App->Command, appl, 12);
	   cfg->C_BBS = 1;

	   
	   if (*(ptr1 - 1) == 0)
		   return 1;
   }

	return(1);
}


/************************************************************************/
/*    USE FOR FREE FORM TEXT IN MESSAGES				*/
/************************************************************************/

int dotext(int i, char * key_word, int max)
{
	int len;
	
	char rec[MAXLINE];

        bseek(fp2,(long) fileoffset,SEEK_SET);

	GetNextLine(rec);

	if (index(rec,"***") == 0)
		bputc('\r',fp2);


	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   rec[strlen(rec) - 1] = '\r';
	   bputs(rec,fp2);

	   fgets(rec,MAXLINE,fp1);
 	}

	bputc('\0',fp2);

	len = btell(fp2) - fileoffset;

	if (btell(fp2) > fileoffset+max)
	{
           Consoleprintf("Text too long: %s - file may be corrupt\r\n",key_word);
	   return(0);
	}

	if (feof(fp1))
	   return(0);
	else
	   return(1);
}

int doBText(int i, char key_word[])
{
	char rec[MAXLINE];

        bseek(fp2,(long) fileoffset,SEEK_SET);

	GetNextLine(rec);

	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   rec[strlen(rec) - 1] = '\r';
	   bputs(rec,fp2);
	   fgets(rec,MAXLINE,fp1);
 	}

	bputc('\0',fp2);

	if (btell(fp2) > fileoffset+120)
	{
		Consoleprintf("Text too long: %s - (max 120 bytes) file may be corrupt",key_word);
		return(0);
	}

	if (feof(fp1))
	   return(0);
	else
	   return(1);
}


/************************************************************************/
/*     CONVERT PRE-SET ROUTES PARAMETERS TO BINARY			*/
/************************************************************************/

int routes(i)
int i;
{
	int err_flag = 0;
	int main_err = 0;

	char rec[MAXLINE];

	char callsign[30];
	int quality;
	int port;
	int pwind;
	int pfrack;
	int ppacl;	
	int inp3;	

	bseek(fp2,(long) fileoffset,SEEK_SET);

	GetNextLine(rec);

	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   pwind=0;
	   pfrack=0;
	   ppacl=0;
	   inp3 = 0;

	   sscanf(rec,"%[^,],%d,%d,%d,%d,%d,%d",callsign,&quality,&port,&pwind,&pfrack,&ppacl, &inp3);

	   err_flag = call_check(callsign);

	   if (quality < 0 || quality > 255)
	   {
	      Consoleprintf("Quality must be between 0 and 255");
		  Consoleprintf("%s\r\n",rec);

	      err_flag = 1;
	   }
	   bputc(quality,fp2);

	   if (port < 1 || port > 32)
	   {
			Consoleprintf("Port number must be between 1 and 32");
			Consoleprintf("%s\r\n",rec);
			err_flag = 1;
	   }
	   bputc(port,fp2);

	   // Use top bit of window as INP3 Flag, next as NoKeepAlive

	   if (inp3 & 1)
		   pwind |= 0x80;

	   if (inp3 & 2)
		   pwind |= 0x40;

	   bputc(pwind, fp2);

	   bputi(pfrack,fp2);
	   bputc(ppacl,fp2);

	   if (err_flag == 1)
	   {
	      Consoleprintf("%s\r\n",rec);
	      main_err = 1;
	      err_flag = 0;
	   }

	   GetNextLine(rec);
	}

	bputc('\0',fp2);

	if (btell(fp2) > fileoffset+510)
	{
	   Consoleprintf("Route information too long - file may be corrupt");
	   main_err = 1;
	}

	if (feof(fp1))
	{
	   Consoleprintf(eof_message);
	   return(0);
	}	

	if (main_err == 1)
	   return(0);
	else
	   return(1);
}


/************************************************************************/
/*     CONVERT PORT DEFINITIONS TO BINARY				*/
/************************************************************************/
int hw;		// Hardware type
int LogicalPortNum;				// As set by PORTNUM

int ports(int i)
{
	char rec[MAXLINE];
	endport=0;
	porterror=0;
	kissflags=0;

	poffset[23]=256;

	bseek(fp2,(long) portoffset,SEEK_SET);

	PortRec = (struct PORTCONFIG *)fp2;

	bseek(fp2,(long) portoffset,SEEK_SET);
        bputi(portnum,fp2);

	LogicalPortNum = portnum;

	while (endport == 0 && !feof(fp1))
	{
	   GetNextLine(rec);
	   decode_port_rec(rec);
	}
	if (porterror != 0)
	{
	   Consoleprintf("Error in port definition");
	   return(0);
	}

	if (PortDefined[LogicalPortNum]) // Already defined?
	{
		Consoleprintf("Port %d already defined", LogicalPortNum);
		heading = 1;
	}

	PortDefined[LogicalPortNum] = TRUE;

	bseek(fp2,(long) portoffset+112,SEEK_SET);
        bputi(kissflags,fp2);

	portoffset = portoffset + 1024;
	portnum++;
	
	return(1); 

}


int tncports(i)
int i;
{
	char rec[MAXLINE];
	endport=0;
	tncporterror=0;
/*
	Set default APPLFLAGS to 6
*/

	bseek(fp2,(long) tncportoffset+5,SEEK_SET);
	bputc(6,fp2);

	while (endport == 0 && !feof(fp1))
	{
	   GetNextLine(rec);
	   decode_tnc_rec(rec);
	}
	if (tncporterror != 0)
	{
	   Consoleprintf("Error in TNC PORT definition");
	   return(0);
	} 
   	tncportoffset = tncportoffset + 8;
	return(1); 


}



int dedports(i)
int i;
{
	char rec[MAXLINE];
	endport=0;
	dedporterror=0;
/*
	Set default APPLFLAGS to 6
*/

	bseek(fp2,(long) tncportoffset+5,SEEK_SET);
	bputc(6,fp2);

	while (endport == 0 && !feof(fp1))
	{
	   GetNextLine(rec);
	   decode_ded_rec(rec);
	}
	if (dedporterror != 0)
	{
	   Consoleprintf("Error in DEDHOST definition");
	   return(0);
	} 
   	tncportoffset = tncportoffset + 8;
	return(1); 


}





/************************************************************************/
/*   MISC FUNCTIONS							*/
/************************************************************************/

/************************************************************************/
/*   FIND OCCURENCE OF ONE STRING WITHIN ANOTHER			*/
/************************************************************************/

int index(s, t)
char s[], t[];
{
	int i, j ,k;

	for (i=0; s[i] != '\0'; i++)
	{
	   for (j=i, k=0; t[k] != '\0' && s[i] == t[k]; j++, k++)
	      ;
	   if (t[k] == '\0')
		return(i);
	}
	return(-1);
}


/************************************************************************/
/*   FIND FIRST OCCURENCE OF A CHARACTER THAT IS NOT c			*/
/************************************************************************/

int verify(s, c)
char s[], c;
{
	int i;

	for (i = 0; s[i] != '\0'; i++)
	   if (s[i] != c)
	      return(i);

	return(-1);
}


/************************************************************************/
/*   PUT A TWO BYTE INTEGER ONTO THE FILE				*/
/************************************************************************/

VOID bputi(int i, char * ptr)
{
	int high;
	int low;

	high = i / 256;
	low = i % 256;

	bputc(low, ptr);
	bputc(high, ptr);

	return;
}


/************************************************************************/
/*   GET NEXT LINE THAT ISN'T BLANK OR IS A COMMENT LINE		*/
/************************************************************************/

int GetNextLine(char *rec)
{
	int i, j;
	char * ret;

	do
	{
		ret = fgets(rec,MAXLINE,fp1);

		if (ret == NULL)
		{
			rec[0] = 0;
			return 0;
		}

		for (i=0; rec[i] != '\0'; i++)
			if (rec[i] == '\t' || rec[i] == '\n')
				rec[i] = ' ';

		j = verify(rec,' ');

		if (j > 0)
		{
			// Remove Leading Spaces
				
			for (i=0; rec[j] != '\0'; i++, j++)
				rec[i] = rec[j];

			rec[i] = '\0';
		}
			
		j = index(rec,";");

		if (j != -1)
			rec[j] = '\0';				// Chop at comment

		if (strlen(rec) > 1)
			if (memcmp(rec, "/*",2) == 0)
				Comment = TRUE;
			else
				if (memcmp(rec, "*/",2) == 0)
				{
					rec[0] = 32;
					rec[1] = 0;
					Comment = FALSE;
				}

		if (Comment)
		{
			rec[0] = 32;
			rec[1] = 0;
			continue;
		}

//		  j = index(rec,"#");

//	      if (j != -1)
//		 rec[j] = '\0';

	} while (verify(rec,' ') == -1);

	return 0;
}


/************************************************************************/
/*   TEST VALIDITY OF CALLSIGN						*/
/************************************************************************/

int call_check_internal(char * callsign)
{
	char call[20];
	int ssid;
	int err_flag = 0;
	int i;

	if (index(callsign,"-") > 0)	/* There is an SSID field */
	{
	   sscanf(callsign,"%[^-]-%d",call,&ssid);
	   if (strlen(call) > 6)
	   {
		  Consoleprintf("Callsign too long, 6 characters before SSID");
	  	  Consoleprintf("%s\r\n",callsign);
	      err_flag = 1;
	   }
	   if (ssid < 0 || ssid > 15)
	   {
	      Consoleprintf("SSID out of range, must be between 0 and 15");
	      Consoleprintf("%s\r\n",callsign);
		  err_flag = 1;
	   }
	}
	else				/* No SSID field */
	{
	   if (strlen(callsign) > 6)
	   {
		  Consoleprintf("Callsign too long, 6 characters maximum");
		  Consoleprintf("%s\r\n",callsign);
	      err_flag = 1;
	   }
	}  

	strcat(callsign,"          ");
	callsign[10] = '\0';
	for (i=0; i< 10; i++)
		callsign[i]=toupper(callsign[i]);

	return(err_flag);
}

int call_check(char * callsign)
{
	int err = call_check_internal(callsign);
	bputs(callsign,fp2);
	return err;
}


/* Process  UNPROTO string allowing VIA */

char workstring[80];

int callstring(i, value, rec)
int i;
char value[];
char rec[];
{
	unsigned int j;

	bseek(fp2,(long) fileoffset,SEEK_SET);

	for (j = 0;( j < strlen(value)); j++)
	    bputc(value[j],fp2);

        return(1);
}

/*
		RADIO PORT PROCESSING 
*/


decode_port_rec(char * rec)
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */
	unsigned long IPADDR;
	WSADATA	WsaData;	// receives data from WSAStartupproblem

	char key_word[20]="";
	char value[300]="";

	struct WL2KInfo * WL2K;

	if (_memicmp(rec, "CONFIG", 6) == 0)
	{
		// Create Embedded PORT Config

		// Copy all subseuent lines up to ENDPORT to a memory buffer

		char * ptr;
		int i;

		if (LogicalPortNum > 32)
		{
			Consoleprintf("Portnum %d is invalid", LogicalPortNum);
			LogicalPortNum = 0;
		}

		PortConfig[LogicalPortNum] = ptr = malloc(50000);
		*ptr = 0;
	
		GetNextLine(rec);

		while (!feof(fp1))
		{
			if (_memicmp(rec, "ENDPORT", 7) == 0)
			{
				PortConfig[LogicalPortNum] = realloc(PortConfig[LogicalPortNum], (strlen(ptr) + 1));		
				endport = 1;
				return 0;
			}

			i = strlen(rec);
			i--;

			while(i > 1)
			{
				if (rec[i] == ' ')
					rec[i] = 0;				// Remove trailing spaces
				else
					break;

				i--;
			}

			// Pick out RIGCONFIG Records

			if (_memicmp(rec, "RIGCONTROL", 10) == 0)
			{
				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				if (strlen(rec) > 15)
					RigConfigMsg[LogicalPortNum] = _strdup(rec);
				else
				{
					// Multiline config, ending in ****

					char * rptr;
					
					RigConfigMsg[LogicalPortNum] = rptr = zalloc(50000);

					strcpy(rptr, "RIGCONTROL ");

					GetNextLine(rec);

					while(!feof(fp1))
					{
						if (memcmp(rec, "***", 3) == 0)
						{
							RigConfigMsg[LogicalPortNum] = realloc(RigConfigMsg[LogicalPortNum], (strlen(rptr) + 1));		
							break;
						}
						strcat(rptr, rec);
						GetNextLine(rec);
					}
				}
			}
			else
			{
				strcat(ptr, rec);
				strcat(ptr, "\r\n");
			}

			GetNextLine(rec);
		}

		Consoleprintf("Missing ENDPORT for Port %d", portnum);
		heading = 1;

		return 0;
	}

	if (index(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

	if (_stricmp(key_word, "portnum") == 0)
	{
		// Save as LogicalPortNum

		LogicalPortNum = atoi(value);
	}

/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0; i < PPARAMLIM && _stricmp(pkeywords[i],key_word) != 0  ; i++)
	   ;

	if (i == PPARAMLIM)
	   Consoleprintf("Source record not recognised - Ignored:%s\r\n",rec);
	else
	{
	   fileoffset = poffset[i] + portoffset;

	   switch (proutine[i])
	   {
	   
	   case 0:
		   cn = callsign(i,value,rec);          /* CALLSIGNS */
		   break;

             case 1:
		cn = int_value(i,value,rec);	     /* INTEGER VALUES */
		break;

             case 2:
              	cn = bin_switch(i,value,rec);        /* 0/1 SWITCHES */
		break;

             case 3:
		cn = hex_value(i,value,rec);         /* HEX NUMBERS */
		break;

             case 4:
             	cn = doid(i,value,rec);               /* ID PARMS */
		break;

             case 5:
             	cn = hwtypes(i,value,rec);              /* HARDWARE TYPES */
		break;

             case 6:
             	cn = bbsflag(i,value,rec);
		break;

             case 7:
             	cn = channel(i,value,rec);
		break;

	    case 8:
		cn = protocols(i,value,rec);
		break;

	    case 10:
		cn = validcalls(i,value,rec);
		break;

	    case 11:
		cn = callstring(i,value,rec);
		break;

	    case 12:
		cn = kissoptions(i,value,rec);
		break;

        case 13:
	        cn = dec_switch(i,value,rec);        /* 0/9 SWITCHES */
			break;

        case 14:
            cn = dodll(i,value,rec);               /* DLL PARMS */
			break;

        case 15:
            cn = doDriver(i,value,rec);               /* DLL PARMS */
			break;

        case 16:

			WL2K = DecodeWL2KReportLine(rec);
			memcpy(&Buffer[fileoffset], &WL2K, 4);
			break;
		
		case 17:

			// IP Address for KISS over UDP
			
			WSAStartup(MAKEWORD(2, 0), &WsaData);
			IPADDR = inet_addr(&rec[7]);
			memcpy(&Buffer[fileoffset], &IPADDR, 4);
			WSACleanup();

			break;

		case 9:
			
			cn = 1;
			endport=1;

	        bseek(fp2,(long) portoffset+255,SEEK_SET);
	        bputc('\0',fp2);

		break;
	   }
	}
	if (cn == 0) porterror=1;

	return 0;
}


int doid(i, value, rec)
int i;
char value[];
char rec[];
{
	unsigned int j;
	for (j = 3;( j < strlen(rec)+1); j++)
	    
	workstring[j-3] = rec[j];

	// Remove trailing spaces before checking length

	i = strlen(workstring);
	i--;

	while(i > 1)
	{
		if (workstring[i] == ' ')
			workstring[i] = 0;				// Remove trailing spaces
		else
			break;

		i--;
	}

	if (i > 29)
	{
	   Consoleprintf("Port description too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	}
	strcat(workstring,"                             ");
	workstring[30] = '\0';

	bseek(fp2,(long) fileoffset,SEEK_SET);
	bputs(workstring,fp2);
	return(1);
}

int dodll(i, value, rec)
int i;
char value[];
char rec[];
{
	unsigned int j;
	for (j = 8;( j < strlen(rec)+1); j++)
	    workstring[j-8] = rec[j];

	if (j > 24)
	{
	   Consoleprintf("DLL name too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	}
		
	_strupr(workstring);
	strcat(workstring,"                ");

	memcpy(PortRec->DLLNAME, workstring, 16);
	PortRec->TYPE = 16;		// External

	if (strstr(PortRec->DLLNAME, "TELNET") || strstr(PortRec->DLLNAME, "AXIP"))
		RFOnly = FALSE;

	return(1);
}

int doDriver(int i, char * value, char * rec)
{
	unsigned int j;
	for (j = 7;( j < strlen(rec)+1); j++)
	    workstring[j-7] = rec[j];

	if (j > 23)
	{
	   Consoleprintf("Driver name too long - Truncated");
	   Consoleprintf("%s\r\n",rec);
	}

	_strupr(workstring);
	strcat(workstring,"                ");

	memcpy(PortRec->DLLNAME, workstring, 16);
	PortRec->TYPE = 16;				// External

	if (strstr(PortRec->DLLNAME, "TELNET") || strstr(PortRec->DLLNAME, "AXIP"))
		RFOnly = FALSE;

	return 1;
}



int hwtypes(i, value, rec)
int i;
char value[];
char rec[];
{
	hw = 255;
	if (_stricmp(value,"ASYNC") == 0)
	   hw = 0;
	if (_stricmp(value,"PC120") == 0)
	   hw = 2;
	if (_stricmp(value,"DRSI") == 0)
	   hw = 4;
	if (_stricmp(value,"DE56") == 0)
	   hw = 4;
	if (_stricmp(value,"TOSH") == 0)
	   hw = 6;
	if (_stricmp(value,"QUAD") == 0)
	   hw = 8;
	if (_stricmp(value,"RLC100") == 0)
	   hw = 10;
	if (_stricmp(value,"RLC400") == 0)
	   hw = 12;
	if (_stricmp(value,"INTERNAL") == 0)
	   hw = 14;
	if (_stricmp(value,"EXTERNAL") == 0)
	   hw = 16;
	if (_stricmp(value,"BAYCOM") == 0)
	   hw = 18;
	if (_stricmp(value,"PA0HZP") == 0)
	   hw = 20;

	bseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   Consoleprintf("Invalid Hardware Type (not DRSI PC120 INTERNAL EXTERNAL BAYCOM PA0HZP ASYNC QUAD)");
	   Consoleprintf("%s\r\n",rec);
	   return (0);
	}
	else
           bputi(hw,fp2);

	return(1);
}
int protocols(i, value, rec)
int i;
char value[];
char rec[];
{
	int hw;

	hw = 255;
	if (_stricmp(value,"KISS") == 0)
	   hw = 0;
	if (_stricmp(value,"NETROM") == 0)
	   hw = 2;
	if (_stricmp(value,"BPQKISS") == 0)
	   hw = 4;
	if (_stricmp(value,"HDLC") == 0)
	   hw = 6;
	if (_stricmp(value,"L2") == 0)
	   hw = 8;
	if (_stricmp(value,"PACTOR") == 0)
	   hw = 10;
	if (_stricmp(value,"WINMOR") == 0)
	   hw = 10;


	bseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   Consoleprintf("Invalid Protocol (not KISS NETROM COMBIOS HDLC)");
	   Consoleprintf("%s\r\n",rec);
	   return (0);
	}
	else
           bputi(hw,fp2);

	return(1);
}


int bbsflag(i, value, rec)
int i;
char value[];
char rec[];
{
	int hw=255;

	if (_stricmp(value,"NOBBS") == 0)
	   hw = 1;
	if (_stricmp(value,"BBSOK") == 0)
	   hw = 0;
	if (_stricmp(value,"") == 0)
	   hw = 0;

	if (hw==255)
	{
	   Consoleprintf("BBS Flag must be NOBBS, BBSOK, or null");
	   Consoleprintf("%s\r\n",rec);
	   return(0);
	}

	bseek(fp2,(long) fileoffset,SEEK_SET);

        bputi(hw,fp2);
        return(1);
}

int channel(i, value, rec)
int i;
char value[];
char rec[];
{
	bseek(fp2,(long) fileoffset,SEEK_SET);
	bputc(value[0],fp2);
        return(1);
}


int validcalls(i, value, rec)
int i;
char value[];
char rec[];
{
	int j;

	bseek(fp2,(long) fileoffset,SEEK_SET);

	for (j = 11;( rec[j] > ' '); j++)

	{
	    bputc(rec[j],fp2);
	    poffset[23]++;
	}

        return(1);
}


int kissoptions(i, value, rec)
int i;
char value[];
char rec[];
{
	int err=255;

	char opt1[12];
	char opt2[12];
	char opt3[12];
	char opt4[12];
	char opt5[12];

	opt1[1] = '\0';
	opt2[1] = '\0';
	opt3[1] = '\0';
	opt4[1] = '\0';
	opt5[1] = '\0';


	sscanf(value,"%[^,+],%[^,+],%[^,+],%[^,+],%[^,+]",opt1,opt2,opt3,opt4,opt5);

	if (opt1[1] != '\0') {do_kiss(opt1,rec);}
	if (opt2[1] != '\0') {do_kiss(opt2,rec);}
	if (opt3[1] != '\0') {do_kiss(opt3,rec);}
	if (opt4[1] != '\0') {do_kiss(opt4,rec);}
	if (opt5[1] != '\0') {do_kiss(opt5,rec);}

	return(1);
}



/*
		TNC PORT PROCESSING 
*/
static char *tkeywords[] = 
{
"COM", "TYPE", "APPLMASK", "KISSMASK", "APPLFLAGS", "ENDPORT"
};           /* parameter keywords */

static int toffset[] =
{
0, 1, 2, 3, 5, 8
};		/* offset for corresponding data in config file */

static int troutine[] = 
{
1, 5, 1, 3, 1, 9
};		/* routine to process parameter */

#define TPARAMLIM 6

decode_tnc_rec(rec)
char rec[];
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */

	char key_word[20];
	char value[300];

	if (index(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0; _stricmp(tkeywords[i],key_word) != 0 && i < TPARAMLIM; i++)
	   ;

	if (i == TPARAMLIM)
	   Consoleprintf("Source record not recognised - Ignored:%s\r\n",rec);
	else
	{
	   fileoffset = toffset[i] + tncportoffset;
	   switch (troutine[i])
           {

             case 1:
		cn = dec_byte(i,value,rec);	     /* INTEGER VALUES */
		break;

             case 2:
              	cn = bin_switch(i,value,rec);        /* 0/1 SWITCHES */
		break;

             case 3:
		cn = hex_value(i,value,rec);         /* HEX NUMBERS */
		break;

             case 5:
             	cn = tnctypes(i,value,rec);          /* PORT MODES */
		break;

             case 9:
              	cn = 1;
		endport=1;

		break;
           }
	}
	if (cn == 0) tncporterror=1;

	return 0;

}

int tnctypes(i, value, rec)
int i;
char value[];
char rec[];
{
	int hw;

	hw = 255;
	if (_stricmp(value,"KISS") == 0)
	   hw = 2;
	if (_stricmp(value,"TNC2") == 0)
	   hw = 0;
	if (_stricmp(value,"PK232/UFQ") == 0)
	   hw = 4;
	if (_stricmp(value,"PK232/AA4RE") == 0)
	   hw = 6;

	bseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   Consoleprintf("Invalid TNC Type (not TNC2 KISS PK232/UFQ PK232/AA4RE)");
	   Consoleprintf("%s\r\n",rec);
	   return (0);
	}
	else
           bputc(hw,fp2);

	return(1);
}

int dec_byte(i, value, rec)
int i;
char value[];
char rec[];
{
	int value_int;

	value_int = atoi(value);

	bseek(fp2,(long) fileoffset,SEEK_SET);
	bputc(value_int,fp2);
	return(1);
}

int do_kiss (char * value,char * rec)
{
	int err=255;

	if (_stricmp(value,"POLLED") == 0)
	{
		err=0;
		kissflags=kissflags | 2;
	}

	if (_stricmp(value,"CHECKSUM") == 0)
	{
		err=0;
		kissflags=kissflags | 1;
	}

	if (_stricmp(value,"D700") == 0)
	{
		err=0;
		kissflags=kissflags | 16;
	}
	if (_stricmp(value,"TNCX") == 0)
	{
		err=0;
		kissflags=kissflags | 32;
	}

	if (_stricmp(value,"ACKMODE") == 0)
	{
		err=0;
		kissflags=kissflags | 4;
	}

	if (_stricmp(value,"SLAVE") == 0)
	{
		err=0;
		kissflags=kissflags | 8;
	}

	if (err == 255)
	{
	   Consoleprintf("Invalid KISS Options (not POLLED ACKMODE CHECKSUM D700 SLAVE TNCX)");
	   Consoleprintf("%s\r\n",rec);
	}
	return (err);
}




/*
		DEDHOST PORT PROCESSING 
*/
static char *dkeywords[] = 
{
"INT", "STREAMS", "APPLMASK", "BUFFERPOOL", "CONNECTS", "ENDPORT"
};           /* parameter keywords */

static int doffset[] =
{
0, 1, 2, 3, 5, 8
};		/* offset for corresponding data in config file */

static int droutine[] = 
{
1, 5, 1, 3, 1, 9
};		/* routine to process parameter */

#define DPARAMLIM 6

decode_ded_rec(rec)
char rec[];
{
	int i;
	int cn = 1;			/* RETURN CODE FROM ROUTINES */

	char key_word[20];
	char value[300];

	if (index(rec,"=") >= 0)
	   sscanf(rec,"%[^=]=%s",key_word,value);
	else
	   sscanf(rec,"%s",key_word);

/************************************************************************/
/*      SEARCH FOR KEYWORD IN TABLE					*/
/************************************************************************/

	for (i=0; _stricmp(dkeywords[i],key_word) != 0 && i < DPARAMLIM; i++)
	   ;

	if (i == DPARAMLIM)
	   Consoleprintf("Source record not recognised - Ignored:%s\r\n",rec);
	else
	{
	   fileoffset = doffset[i] + tncportoffset;
	   switch (droutine[i])
           {

             case 1:
		cn = dec_byte(i,value,rec);	     /* INTEGER VALUES */
		break;

             case 2:
              	cn = bin_switch(i,value,rec);        /* 0/1 SWITCHES */
		break;

             case 3:
		cn = hex_value(i,value,rec);         /* HEX NUMBERS */
		break;

             case 5:
             	cn = tnctypes(i,value,rec);          /* PORT MODES */
		break;

             case 9:
              	cn = 1;
		endport=1;

		break;
           }
	}
	if (cn == 0) dedporterror=1;

	return 0;
}

/* default values for all params - used by 'simple' config mode */


static int defaults [] =
{
5,4,60,25,4,120,	/* OBSINIT OBSMIN NODESINT L3TTL L4RETRIES L4TIMEOUT */
 
255,120,1,180,900,0,    /* BUFFERS PACLEN TRANSDELAY T3 IDLETIME BBS */

1,-1,-1,-1,-1,		/* NODE NODEALIAS BBSALIAS NODECALL BBSCALL */

-1,-1,-1,-1,-1,30,	/* TNCPORT IDMSG: INFOMSG:  ROUTES: PORT MAXLINKS */

50,30,120,15,140,		/* MAXNODES MAXROUTES  MAXCIRCUITS IDINTERVAL MINQUAL */

0,10,4,0,-1,255,	/* HIDENODES L4DELAY L4WINDOW BTINTERVAL UNPROTO BBSQUAL */

-1,0,-1,1,127,0,	/* APPLS EMS CTEXT: DESQVIEW HOSTINTERRUPT ENABLE_LINKED */

-1,0,-1			/* DEDHOST FULL_CTEXT SIMPLE */

};



int simple(int i)
{

	// Set up the basic config header

	struct CONFIGTABLE Cfg;

	memset(&Cfg, 0, 256);

	Cfg.C_AUTOSAVE = 1;
	Cfg.C_BBS = 1;
	Cfg.C_BTINTERVAL = 60;
	Cfg.C_BUFFERS = 999;
	Cfg.C_C = 1;
	Cfg.C_DESQVIEW = 0;
	Cfg.C_EMSFLAG = 0;
	Cfg.C_FULLCTEXT = 1;
	Cfg.C_HIDENODES = 0;
	Cfg.C_HOSTINTERRUPT = 127;
	Cfg.C_IDINTERVAL = 10;
	Cfg.C_IDLETIME = 900;
	Cfg.C_IP = 0;
	Cfg.C_L3TIMETOLIVE = 25;
	Cfg.C_L4DELAY = 10;
	Cfg.C_L4RETRIES = 3;
	Cfg.C_L4TIMEOUT = 60;
	Cfg.C_L4WINDOW = 4;
	Cfg.C_LINKEDFLAG = 'A';
	Cfg.C_MAXCIRCUITS = 128;
	Cfg.C_MAXDESTS = 250;
	Cfg.C_MAXHOPS = 4;
	Cfg.C_MAXLINKS = 64;
	Cfg.C_MAXNEIGHBOURS = 64;
	Cfg.C_MAXRTT = 90;
	Cfg.C_MINQUAL = 150;
	Cfg.C_NODE = 1;
	Cfg.C_NODESINTERVAL = 30;
	Cfg.C_OBSINIT = 6;
	Cfg.C_OBSMIN = 5;
	Cfg.C_PACLEN = 236;
	Cfg.C_T3 = 180;
	Cfg.C_TRANSDELAY = 1;

	/* Set PARAMOK flags on all values that are defaulted */

	for (i=0; i < PARAMLIM; i++)
	   paramok[i]=1;

	bseek(fp2,(long) 0,SEEK_SET);

	memcpy(Buffer, &Cfg, sizeof(Cfg));

	paramok[15]=0;		/* Must have callsign */

	return(1);
}

DllExport VOID FreeConfig()
{
	free(Buffer);
}

/* END OF CODE */

BOOL ProcessAPPLDef(char * buf)
{
	// New Style APPL definition

	// APPL n,COMMAND,CMDALIAS,APPLCALL,APPLALIAS,APPLQUAL

	char * ptr;
	char * Context;
	int Appl;
	struct APPLCONFIG * App;
	struct CONFIGTABLE * cfg = (struct CONFIGTABLE * )Buffer;

	_strupr(buf);

	ptr = strtok_s(buf, ", /\n\r", &Context);

	if (ptr == 0) return FALSE;

	Appl = atoi(ptr);

	if (Appl < 1 || Appl > 32) return FALSE;

	App = (struct APPLCONFIG *)&Buffer[ApplOffset + (Appl - 1) * sizeof(struct APPLCONFIG) ];

	ptr = strchr(Context, ',');
	if (ptr == 0)
	{
		ptr = strchr(Context, ' ');				// There is a space on end of command
		if (ptr == 0)  return FALSE;			// No comma or space meeans no param
	}

	(*ptr++) = 0;

	if (strlen(Context) > 12) return FALSE;

	memcpy(App->Command, Context, strlen(Context));

	cfg->C_BBS = 1;

	Context = ptr;

	ptr = strchr(Context, ',');

	if (ptr == 0)
	{
		// No comma, so must be last param - this is ok
		
		if (strlen(Context) > 48) return FALSE;

		memcpy(App->CommandAlias, Context, strlen(Context));
		return TRUE;
	}

	(*ptr++) = 0;

	if (strlen(Context) > 48) return FALSE;

	memcpy(App->CommandAlias, Context, strlen(Context));

	Context = ptr;

	ptr = strtok_s(NULL, ", /\n\r", &Context);
	if (ptr == 0) return TRUE;						// Allow Missing Call
	if (strlen(ptr) > 10) return FALSE;

	memcpy(App->ApplCall, ptr, strlen(ptr));

	ptr = strtok_s(NULL, ", /\n\r", &Context);
	if (ptr == 0) return TRUE;						// Allow missing Alias
	if (strlen(ptr) > 10) return FALSE;

	memcpy(App->ApplAlias, ptr, strlen(ptr));

	ptr = strtok_s(NULL, ", /\n\r", &Context);
	if (ptr == 0) return TRUE;						// Allow missing Quality

	App->ApplQual = atoi(ptr);

	return TRUE;
}

 BOOL ToLOC(double Lat, double Lon , char * Locator)
 {
	int i;
	double S1, S2;

	Lon = Lon + 180;
	Lat = Lat + 90;

	S1 = fmod(Lon, 20);

	#pragma warning(push)
	#pragma warning(disable : 4244)
	
	i = Lon / 20;
	Locator[0] = 65 + i;

	S2 = fmod(S1, 2);

	i = S1 / 2;
	Locator[2] = 48 + i;

	i = S2 * 12;
	Locator[4] = 65 + i;

	S1 = fmod(Lat,10);

	i = Lat / 10;
	Locator[1] = 65 + i;

	S2 = fmod(S1,1);

	i = S1;
	Locator[3] = 48 + i;

	i = S2 * 24;
	Locator[5] = 65 + i;

	#pragma warning(pop)

	return TRUE;
 }
