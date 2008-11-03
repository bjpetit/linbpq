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

//	5/12/99 - Add DDLNAME Param for ext driver

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

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>



int tnctypes(int i, char value[],char rec[]);
int do_kiss (char value[],char rec[]);
int dec_byte(i, value, rec);

extern int __cdecl AnyKey();
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
extern int __cdecl dotext(int i,char *key_word);
extern int __cdecl doBText(int i,char *key_word);
extern int __cdecl routes(int i);
extern int __cdecl ports(int i);
extern int __cdecl tncports(int i);
extern int __cdecl dedports(int i);
extern int __cdecl index(char *s,char *t);
extern int __cdecl verify(char *s,char c);
extern int __cdecl fputi(int i,struct _iobuf *fp);
extern int __cdecl strip(char *rec);
extern int __cdecl call_check(char *callsign);
extern int __cdecl callstring(int i,char *value,char *rec);
extern int __cdecl decode_port_rec(char *rec);
extern int __cdecl doid(int i,char *value,char *rec);
extern int __cdecl dodll(int i,char *value,char *rec);
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

//int i;
//char value[];
//char rec[];



#define PARAMLIM 71
#define MAXLINE 250
#define FILEVERSION 22

char inputname[250]="bpqcfg.txt";
char outputname[250]="bpqcfg.bin";
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
"BTEXT:", "IPGATEWAY"
};           /* parameter keywords */

static int offset[] =
{
40, 42, 44, 46, 48, 50,
52, 54, 56, 58, 64, 68,
69, 10, 30, 0, 20,
384, 512, 1024, 1536, 2560, 72,
74, 76, 78, 96, 100,
102, 103, 105, 107, 120, 118,
256, 66, 2048, 71, 70, 67,
3000, 98, 0, 109, 110,
0,10,20,30,
40,50,60,70,
80,90,100,110,
120,130,140,150,
160,162,164,166,
168,170,172,174,
121, 112							// BTEXT was UNPROTO+1
};		/* offset for corresponding data in config file */

static int routine[] = 
{
1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 2,
2, 0, 0, 0, 0,
3, 4, 4, 5, 6, 1,
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
15, 2
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
"TXPORT", "MHEARD", "CWIDTYPE", "MINQUAL", "MAXDIGIS", "PORTALIAS2", "DLLNAME", "BCALL", "DIGIMASK"
};           /* parameter keywords */

static int poffset[] =
{
2, 32, 34, 36, 38, 40, 42,
44, 46, 48, 50, 52, 54,
56, 58, 60, 62, 64, 
66, 80, 90, 100, 127, 256,
68, 70, 71 ,74, 128, 0,
76, 78, 110, 112, 114, 116,
118, 120, 121, 122, 123, 200, 210, 226, 72
};		/* offset for corresponding data in config file */

static int proutine[] = 
{
4, 5, 8, 3, 1, 1, 7,
6, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1,
1, 0, 0, 0, 9, 10,
1, 13, 13, 1, 11, 1,
1, 2, 2, 12, 1, 1,
1, 7, 7, 13, 13, 0, 14, 0, 1
};		/* routine to process parameter */


#define PPARAMLIM 45



static int fileoffset = 0;
static int portoffset = 2560;
static int tncportoffset = 384;
static int endport = 0;
static int portnum = 1;
static int porterror = 0;
static int tncporterror = 0;
static int dedporterror = 0;
static int kissflags = 0;

/************************************************************************/
/* Global variables							*/
/************************************************************************/

int paramok[PARAMLIM];		/* PARAMETER OK FLAG  */

FILE *fp1;			/* TEXT INPUT FILE    */
FILE *fp2;			/* BINARY OUTPUT FILE */

static char s1[80];
static char s2[80];
static char s3[80];
static char s4[80];
static char s5[80];
static char s6[80];
static char s7[80];
static char s8[80];

char spaces[]="                ";
char commas[]=",,,,,,,,";

char bbscall[11];
char bbsalias[11];
int bbsqual;

int AnyKey()
{
	int i=0;
	
	printf("Press any key to Exit");
	i = _getch();

	return 0;
}

/************************************************************************/
/*   MAIN PROGRAM							*/
/************************************************************************/

main( int argc, char *argv[ ], char *envp[ ] )
{
	int i;
 	int heading = 0;

	char rec[MAXLINE];

	if (argc >1)
	{
		// Config file name is first param

		strcpy(inputname,argv[1]);
	}

	if (argc >2)
	{
		// Output file name is second param

		strcpy(outputname,argv[2]);
	}

	if (argc >3)
	{
		// Output file name is second param

		strcpy(option,argv[3]);
	}

	/*#define THOR*/  

#ifdef THOR

	puts("TheNode AX.25 Networking Software");

#else

	puts("G8BPQ AX.25 Networking Software");

#endif

	puts("Configuration file Preprocessor for Version 4.10f November 2008.\n");

#ifdef THOR

	if ((fp1 = fopen("CONFIG.TXT","r")) == NULL)
	{
	   puts("Could not open file CONFIG.TXT");

	   AnyKey();		// Give user time to read msg

	   exit(1);

	}

	if ((fp2 = fopen("CONFIG.DAT","wb")) == NULL)
	{
	   puts("Could not open file CONFIG.DAT");

	   AnyKey();		// Give user time to read msg

	   exit(1);
	}


#else

	if ((fp1 = fopen(inputname,"r")) == NULL)
	{
	   printf("Could not open file %s\n",inputname);

	   AnyKey();		// Give user time to read msg

	   exit(1);
	}

	if ((fp2 = fopen(outputname,"wb")) == NULL)
	{
	   printf("Could not open file %s\n",outputname);

	   AnyKey();		// Give user time to read msg

	   exit(1);
	}

#endif


	printf("Source: %s, Destination=%s\n\n",inputname,outputname);

	for (i=0; i<2560; i++)
	   fputc('\0',fp2);

	strip(rec);

	while (!feof(fp1))
	{
	   decode_rec(rec);

	   strip(rec);
	}

	paramok[17]=1;          /* dont need TNCPORTS */

	paramok[32]=1;          /* dont need UNPROTO */

	paramok[40]=1;			/* or DEDHOST */

	paramok[42]=1;			/* or SIMPLE */

	paramok[43]=1;			/* or AUTOSAVE */

	paramok[44]=1;			/* or L4APPL */

	if (paramok[45]==1)
	{
		paramok[16]=1;	//  APPL1CALL overrides BBSCALL
		fseek(fp2,(long) 20,SEEK_SET);

		for (i=0; i<10; i++)
		{
			if (bbscall[i] == 0)
				
				fputc(32,fp2);
			else
				fputc(bbscall[i],fp2);
		}
			
	}
	
	if (paramok[53]==1)
	{
		paramok[14]=1;	//  APPL1ALIAS overrides BBSALIAS
		fseek(fp2,(long) 30,SEEK_SET);

		for (i=0; i<10; i++)
		{
			if (bbsalias[i] == 0)
				
				fputc(32,fp2);
			else
				fputc(bbsalias[i],fp2);

		}
			
	}

	if (paramok[61]==1) 
	{
		paramok[33]=1;	//  APPL1QUAL overrides BBSQUAL
		fseek(fp2,(long) 118,SEEK_SET);
		fputi(bbsqual,fp2);
	}
			
	
	for (i=0;i<24;i++)
		
		paramok[45+i]=1;	/* or APPLCALLS, APPLALIASS APPLQUAL */

	paramok[69]=1;			// BText optional
	paramok[70]=1;			// IPGateway optional

	for (i=0; i < PARAMLIM; i++)
	{
	   if (paramok[i] == 0)
	   {
	      if (heading == 0)
	      {
		 puts("\nThe following parameters were not correctly specified\n");
         	 heading = 1;
      	      }
	      puts(keywords[i]);
   	}
	}

        if (portnum == 1)
	{
	   puts("No valid radio ports defined\n");
	   heading = 1;
	}

	fseek(fp2,(long) 255,SEEK_SET);
	fputc(FILEVERSION,fp2);

	fclose(fp1);
	fclose(fp2);

	if (heading == 0)
	{
	   puts("Conversion (probably) successful");
	   if (strcmp(option,"-T") != 0) AnyKey();
	}
	else
	{
   	   puts("\nConversion failed");
	   AnyKey();
	}
  	
	return 0;
}


/************************************************************************/
/*	Decode PARAM=							*/
/************************************************************************/

decode_rec(rec)
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

	for (i=0;  i < PARAMLIM && _stricmp(keywords[i],key_word) != 0 ; i++)
	   ;

	if (i == PARAMLIM)
	   printf("Source record not recognised - Ignored:\n%s\n",rec);
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
             	cn = dotext(i,key_word);             /* TEXT PARMS */
		break;

             case 5:
             	cn = routes(i);                      /* ROUTES TO LOCK IN */
		break;

             case 6:
              	cn = ports(i);                       /* PORTS DEFINITION */
		break;

             case 7:
				 printf("Obsolete Record %s ignored\n",rec);
				 printf("UNPROTO address should now be specified in PORT definition\n\n");

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

int FIRSTAPPL=1;

/************************************************************************/
/*   CALLSIGNS								*/
/************************************************************************/
int applcallsign(i, value, rec)
int i;
char value[];
char rec[];
{
	if (i==45) strcpy(bbscall,value);
	if (i==53) strcpy(bbsalias,value);

	if (FIRSTAPPL)
	{
		FIRSTAPPL=0;
		
		fseek(fp2,(long) portoffset,SEEK_SET);

		for (i=0; i<160; i++)
			fputc(' ',fp2);

		for (i=0; i<16; i++)
			fputc(0,fp2);

		fseek(fp2,(long) portoffset,SEEK_SET);

	}

	fileoffset=fileoffset+portoffset;

	return callsign (i,value,rec);
}

int appl_qual(i, value, rec)
int i;
char value[];
char rec[];
{
	int j,k;
	
	if (FIRSTAPPL)
	{
		FIRSTAPPL=0;
		
		fseek(fp2,(long) portoffset,SEEK_SET);

		for (j=0; j<160; j++)
			fputc(' ',fp2);

		for (j=0; j<16; j++)
			fputc(0,fp2);

		fseek(fp2,(long) portoffset,SEEK_SET);

	}

	fileoffset=fileoffset+portoffset;


	fseek(fp2,(long) fileoffset,SEEK_SET);

	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   puts("Invalid numerical value \n");
	   printf("%s\n\n",rec);
	   return(0);
	}
	
	if (i==61) bbsqual=j;

	fputi(j,fp2);
	return(1);
}


int callsign(i, value, rec)
int i;
char value[];
char rec[];
{
	fseek(fp2,(long) fileoffset,SEEK_SET);

	if (call_check(value) == 1)
	{
	   printf("%s\n\n",rec);
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

	fseek(fp2,(long) fileoffset,SEEK_SET);

	k = sscanf(value," %d",&j);

	if (k != 1)
	{
	   puts("Invalid numerical value \n");
	   printf("%s\n\n",rec);
	   return(0);
	}

	fputi(j,fp2);
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
	fseek(fp2,(long) fileoffset,SEEK_SET);

	k = sscanf(value," %xH",&j);

	if (j < 0)
	{
	   puts("Bad Hex Value\n");
	   printf("%s\n\n",rec);
	   return(0);
	}

	fputi(j,fp2);
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
	   fseek(fp2,(long) fileoffset,SEEK_SET);
	   fputc(value_int,fp2);
	   return(1);
	}
	else
	{
	   puts("Invalid switch value, must be either 0 or 1\n");
	   printf("%s\n\n",rec);
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
	   fseek(fp2,(long) fileoffset,SEEK_SET);
	   fputc(value_int,fp2);
	   return(1);
	}
	else
	{
	   puts("Invalid value, must be between 0 and 255\n");
	   printf("%s\n\n",rec);
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
	
        fseek(fp2,(long) fileoffset,SEEK_SET);

	num = sscanf(value," %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",&j1,&j2,&j3,&j4,&j5,&j6,&j7,&j8,&j9,&j10,&j11,&j12,&j13,&j14,&j15,&j16);

	fputc(j1,fp2);
	fputc(j2,fp2);
	fputc(j3,fp2);
	fputc(j4,fp2);
	fputc(j5,fp2);
	fputc(j6,fp2);
	fputc(j7,fp2);
	fputc(j8,fp2);
	fputc(j9,fp2);
	fputc(j10,fp2);
	fputc(j11,fp2);
	fputc(j12,fp2);
	fputc(j13,fp2);
	fputc(j14,fp2);
	fputc(j15,fp2);
	fputc(j16,fp2);

	return(1);
}

int applstrings(i, value, rec)
int i;
char value[];
char rec[];
{

   char appl[20];

   char * ptr1;
   char * ptr2;

   int num,j;

   fseek(fp2,(long) fileoffset,SEEK_SET);

   strcat(rec,commas);		// Ensure 8 commas

   ptr1=&rec[13];		// skip APPLICATIONS=
   num=0;

   while (num < 8)
  
     {
       strcpy(appl,spaces);
       ptr2=appl;
 		
       j = *ptr1++;

       while (j != ',')
         {
            *(ptr2++) = toupper(j);
            j = *ptr1++;
          }
    
        fputs(appl,fp2);
        num++;
     }


	return(1);
}


/************************************************************************/
/*    USE FOR FREE FORM TEXT IN MESSAGES				*/
/************************************************************************/

int dotext(i, key_word)
int i;
char key_word[];
{
	char rec[MAXLINE];

        fseek(fp2,(long) fileoffset,SEEK_SET);

	strip(rec);

	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   rec[strlen(rec) - 1] = '\r';
	   fputs(rec,fp2);

	   fgets(rec,MAXLINE,fp1);
 	}

	fputc('\0',fp2);

	if (ftell(fp2) > fileoffset+510)
	{
           printf("Text too long: %s - file may be corrupt\n",key_word);
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

        fseek(fp2,(long) fileoffset,SEEK_SET);

	strip(rec);

	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   rec[strlen(rec) - 1] = '\r';
	   fputs(rec,fp2);
	   fgets(rec,MAXLINE,fp1);
 	}

	fputc('\0',fp2);

	if (ftell(fp2) > fileoffset+120)
	{
		printf("Text too long: %s - (max 120 bytes) file may be corrupt\n",key_word);
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

	fseek(fp2,(long) fileoffset,SEEK_SET);

	strip(rec);

	while (index(rec,"***") != 0 && !feof(fp1))
	{
	   pwind=0;
	   pfrack=0;
	   ppacl=0;

	   sscanf(rec,"%[^,],%d,%d,%d,%d,%d",callsign,&quality,&port,&pwind,&pfrack,&ppacl);

	   err_flag = call_check(callsign);

	   if (quality < 0 || quality > 255)
	   {
	      puts("Quality must be between 0 and 255\n");
		  printf("%s\n\n",rec);

	      err_flag = 1;
	   }
	   fputc(quality,fp2);

	   if (port < 1 || port > 16)
	   {
			puts("Port number must be between 1 and 16\n");
			printf("%s\n\n",rec);
			err_flag = 1;
	   }
	   fputc(port,fp2);
	   fputc(pwind,fp2);
	   fputi(pfrack,fp2);
	   fputc(ppacl,fp2);

	   if (err_flag == 1)
	   {
	      printf("%s\n\n",rec);
	      main_err = 1;
	      err_flag = 0;
	   }

	   strip(rec);
	}

	fputc('\0',fp2);

	if (ftell(fp2) > fileoffset+510)
	{
	   puts("Route information too long - file may be corrupt");
	   main_err = 1;
	}

	if (feof(fp1))
	{
	   puts(eof_message);
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

int ports(i)
int i;
{
	char rec[MAXLINE];
	endport=0;
	porterror=0;
	kissflags=0;

	poffset[23]=256;

	fseek(fp2,(long) portoffset,SEEK_SET);

	for (i=0; i<512; i++)
	   fputc('\0',fp2);

	fseek(fp2,(long) portoffset,SEEK_SET);
        fputi(portnum,fp2);

	while (endport == 0 && !feof(fp1))
	{
	   strip(rec);
	   decode_port_rec(rec);
	}
	if (porterror != 0)
	{
	   puts("Error in port definition");
	   return(0);
	} 

	fseek(fp2,(long) portoffset+112,SEEK_SET);
        fputi(kissflags,fp2);

   	portoffset = portoffset + 512;
	portnum = portnum +1;
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

	fseek(fp2,(long) tncportoffset+5,SEEK_SET);
	fputc(6,fp2);

	while (endport == 0 && !feof(fp1))
	{
	   strip(rec);
	   decode_tnc_rec(rec);
	}
	if (tncporterror != 0)
	{
	   puts("Error in TNC PORT definition");
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

	fseek(fp2,(long) tncportoffset+5,SEEK_SET);
	fputc(6,fp2);

	while (endport == 0 && !feof(fp1))
	{
	   strip(rec);
	   decode_ded_rec(rec);
	}
	if (dedporterror != 0)
	{
	   puts("Error in DEDHOST definition");
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

fputi(i, fp)
int i;
FILE *fp;
{
	int high;
	int low;

	high = i / 256;
	low = i % 256;

	fputc(low,fp);
	fputc(high,fp);

	return 0;
}


/************************************************************************/
/*   GET NEXT LINE THAT ISN'T BLANK OR IS A COMMENT LINE		*/
/************************************************************************/

strip(rec)
char rec[];
{
	int i, j;

	do
	{
	   fgets(rec,MAXLINE,fp1);

	   for (i=0; rec[i] != '\0'; i++)
	      if (rec[i] == '\t' || rec[i] == '\n')
		 rec[i] = ' ';

	   if (!feof(fp1))
	   {
	      j = verify(rec,' ');

	      if (j > 0)
	      {
		 for (i=0; rec[j] != '\0'; i++, j++)
	  	    rec[i] = rec[j];

		 rec[i] = '\0';
	      }

	      j = index(rec,";");

	      if (j != -1)
		 rec[j] = '\0';

//		  j = index(rec,"#");

//	      if (j != -1)
//		 rec[j] = '\0';
	   
	   }
	}
	while (verify(rec,' ') == -1 && !feof(fp1));

	return 0;
}


/************************************************************************/
/*   TEST VALIDITY OF CALLSIGN						*/
/************************************************************************/

int call_check(callsign)
char callsign[];
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
		  puts("Callsign too long, 6 characters before SSID\n");
	  	  printf("%s\n\n",callsign);
	      err_flag = 1;
	   }
	   if (ssid < 0 || ssid > 15)
	   {
	      puts("SSID out of range, must be between 0 and 15\n");
	      printf("%s\n\n",callsign);
		  err_flag = 1;
	   }
	}
	else				/* No SSID field */
	{
	   if (strlen(callsign) > 6)
	   {
		  puts("Callsign too long, 6 characters maximum\n");
		  printf("%s\n\n",callsign);
	      err_flag = 1;
	   }
	}  

	strcat(callsign,"          ");
	callsign[10] = '\0';
	for (i=0; i< 10; i++)
		callsign[i]=toupper(callsign[i]);

	fputs(callsign,fp2);

	return(err_flag);
}


/* Process  UNPROTO string allowing VIA */

char workstring[80];

int callstring(i, value, rec)
int i;
char value[];
char rec[];
{
	unsigned int j;

	fseek(fp2,(long) fileoffset,SEEK_SET);

	for (j = 0;( j < strlen(value)); j++)
	    fputc(value[j],fp2);

        return(1);
}

/*
		RADIO PORT PROCESSING 
*/
decode_port_rec(rec)
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

	for (i=0; _stricmp(pkeywords[i],key_word) != 0 && i < PPARAMLIM; i++)
	   ;

	if (i == PPARAMLIM)
	   printf("Source record not recognised - Ignored:\n%s\n",rec);
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
            cn = dodll(i,value,rec);               /* ID PARMS */
			break;

			
			
			case 9:
              	cn = 1;
		endport=1;

	        fseek(fp2,(long) portoffset+255,SEEK_SET);
	        fputc('\0',fp2);

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

	if (j > 33)
	{
	   puts("Port description too long - Truncated");
	   printf("%s\n\n",rec);
	}
	strcat(workstring,"                             ");
	workstring[30] = '\0';

	fseek(fp2,(long) fileoffset,SEEK_SET);
        fputs(workstring,fp2);
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
	   puts("DLL name too long - Truncated");
	   printf("%s\n\n",rec);
	}
	strcat(workstring,"                ");
	workstring[16] = '\0';

	fseek(fp2,(long) fileoffset,SEEK_SET);
        fputs(workstring,fp2);
        return(1);
}



int hwtypes(i, value, rec)
int i;
char value[];
char rec[];
{
	int hw;

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

	fseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   puts("Invalid Hardware Type (not DRSI PC120 TOSH ASYNC QUAD)");
	   printf("%s\n\n",rec);
	   return (0);
	}
	else
           fputi(hw,fp2);

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
	if (_stricmp(value,"HDLC") == 0)
	   hw = 6;
	if (_stricmp(value,"COMBIOS") == 0)
	   hw = 8;

	fseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   puts("Invalid Protocol (not KISS NETROM COMBIOS HDLC)");
	   printf("%s\n\n",rec);
	   return (0);
	}
	else
           fputi(hw,fp2);

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
	   puts("BBS Flag must be NOBBS, BBSOK, or null\n");
	   printf("%s\n\n",rec);
	   return(0);
	}

	fseek(fp2,(long) fileoffset,SEEK_SET);

        fputi(hw,fp2);
        return(1);
}

int channel(i, value, rec)
int i;
char value[];
char rec[];
{
	fseek(fp2,(long) fileoffset,SEEK_SET);
	fputc(value[0],fp2);
        return(1);
}


int validcalls(i, value, rec)
int i;
char value[];
char rec[];
{
	int j;

	fseek(fp2,(long) fileoffset,SEEK_SET);

	for (j = 11;( rec[j] > ' '); j++)

	{
	    fputc(rec[j],fp2);
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
	   printf("Source record not recognised - Ignored:\n%s\n",rec);
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

	fseek(fp2,(long) fileoffset,SEEK_SET);

	if (hw == 255)
	{
	   puts("Invalid TNC Type (not TNC2 KISS PK232/UFQ PK232/AA4RE)");
	   printf("%s\n\n",rec);
	   return (0);
	}
	else
           fputc(hw,fp2);

	return(1);
}

int dec_byte(i, value, rec)
int i;
char value[];
char rec[];
{
	int value_int;

	value_int = atoi(value);

	fseek(fp2,(long) fileoffset,SEEK_SET);
	fputc(value_int,fp2);
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

	if (_stricmp(value,"CRC") == 0)
	    {
		err=0;
		kissflags=kissflags | 16;
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
	   puts("Invalid KISS Options (not POLLED ACKMODE CHECKSUM CRC SLAVE)");
	   printf("%s\n\n",rec);
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
	   printf("Source record not recognised - Ignored:\n%s\n",rec);
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
5,4,120,25,4,120,	/* OBSINIT OBSMIN NODESINT L3TTL L4RETRIES L4TIMEOUT */
 
255,120,1,180,900,0,    /* BUFFERS PACLEN TRANSDELAY T3 IDLETIME BBS */

1,-1,-1,-1,-1,		/* NODE NODEALIAS BBSALIAS NODECALL BBSCALL */

-1,-1,-1,-1,-1,10,	/* TNCPORT IDMSG: INFOMSG:  ROUTES: PORT MAXLINKS /*

50,10,10,15,10,		/* MAXNODES MAXROUTES  MAXCIRCUITS IDINTERVAL MINQUAL */

0,10,4,0,-1,255,	/* HIDENODES L4DELAY L4WINDOW BTINTERVAL UNPROTO BBSQUAL */

-1,0,-1,1,127,0,	/* APPLS EMS CTEXT: DESQVIEW HOSTINTERRUPT ENABLE_LINKED */

-1,0,-1			/* DEDHOST FULL_CTEXT SIMPLE */

};



int simple(i)
int i;
{
	/* Set PARAMOK flags on all values that are defaulted */

	for (i=0; i < PARAMLIM; i++)
	   paramok[i]=1;

	fseek(fp2,(long) 69,SEEK_SET);

	paramok[15]=0;		/* Must have callsign */

	return(1);
}

/* END OF CODE */
