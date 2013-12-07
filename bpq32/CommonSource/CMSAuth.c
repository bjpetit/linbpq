#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#ifdef LINBPQ

#include "compatbits.h"

#define APIENTRY
#define VOID void

#else
#include <windows.h>
#endif

char * strlop(char * buf, char delim);

VOID APIENTRY md5 (char *arg, unsigned char * checksum);

// Implementation of the WinLink password challenge/response protocol
  
unsigned char seed [] = {77, 197, 101, 206, 190, 249,
     93, 200, 51, 243, 93, 237,
     71, 94, 239, 138, 68, 108,
     70, 185, 225, 137, 217, 16,
     51, 122, 193, 48, 194, 195,
     198, 175, 172, 169, 70, 84, 
     61, 62, 104, 186, 114, 52,
     61, 168, 66, 129, 192, 208,
     187, 249, 232, 193, 41, 113,
     41, 45, 240, 16, 29, 228,
	 208, 228, 61, 20, 0};

/*
 Calculate the challenge password response as follows:
 - Concatenate the challenge phrase, password, and supplied secret value (i.e. the salt)
 - Generate an MD5 hash of the result
 - Convert the first 4 bytes of the hash to an integer (big endian) and return it
*/

int GetCMSHash(char * Challenge, char * Password)
{
	unsigned char Hash[16];
	unsigned char Phrase[100];

	strlop(Challenge, 13);
	strlop(Password, 13);

	strcpy(Phrase, Challenge);
	strcat(Phrase, Password);
	strcat(Phrase, seed);
	md5(Phrase, Hash);

	return ((Hash[3] & 0x3f) << 24) + (Hash[2] << 16) + (Hash[1] << 8) + Hash[0];
}
