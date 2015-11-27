
#include "ARDOPC.h"


int Ticks;


void printtick(char * msg)
{
	printf("%s %i\r\n", msg, Ticks);
}

#include "ecc.h"

unsigned char msg[] = "Nervously I loaded the twin ducks aboard the revolving pl\
atform.";
unsigned char codeword[256];

unsigned char revcodeword[256];
unsigned char xxcodeword[256];

/* Some debugging routines to introduce errors or erasures
   into a codeword.
   */

/* Introduce a byte error at LOC */
void
byte_err (int err, int loc, unsigned char *dst)
{
	printf("Adding Error at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] ^= err;
}

/* Pass in location of error (first byte position is
   labeled starting at 1, not 0), and the codeword.
*/
void
byte_erasure (int loc, unsigned char dst[], int cwsize, int erasures[])
{
	printf("Erasure at loc %d, data %#x\n", loc, dst[loc-1]);
  dst[loc-1] = 0;
}

int NPAR;

void xxmain ()
{

  int erasures[16];
  int nerasures = 0;
  int i, j;

  /* Initialization the ECC library */

	NPAR = 8;

  initialize_ecc ();

  /* ************** */

  /* Encode data into codeword, adding NPAR parity bytes */

  encode_data(msg, sizeof(msg), codeword);

  printf("Encoded data is: \"%s\" Len %d\n", codeword, sizeof (msg));

#define ML (sizeof (msg) + NPAR)


  /* Add one error and two erasures */
  
 
  byte_err(0x35, 3, codeword);

  byte_err(0x23, 17, codeword);
  byte_err(0x34, 19, codeword);


  printf("with some errors: \"%s\"\n", codeword);

  /* We need to indicate the position of the erasures.  Eraseure
     positions are indexed (1 based) from the end of the message... */

 // erasures[nerasures++] = ML-17;
//  erasures[nerasures++] = ML-19;


  /* Now decode -- encoded codeword size must be passed */

  // Reverse Data and Error Bits

  i = 0;

  for (j = strlen(msg); j >=0; j--)
  {
	  revcodeword[i++] = codeword[j];
  }

  for (j = strlen(msg) + NPAR; j >= strlen(msg); j--)
  {
	  revcodeword[i++] = codeword[j];
  }

  decode_data(revcodeword, ML);

  /* check if syndrome is all zeros */
  if (check_syndrome () != 0) {
    correct_errors_erasures (revcodeword,
			     ML,
			     nerasures,
			     erasures);

i=0;
	for (j = strlen(msg); j >=0; j--)
  {
	  xxcodeword[i++] = revcodeword[j];
	}
    printf("Corrected codeword: \"%s\"\n", xxcodeword);
  }

}

