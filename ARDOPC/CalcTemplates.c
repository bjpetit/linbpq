#include "ARDOPC.h"

static int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 

short intTwoToneLeaderTemplate[120];  // holds just 1 symbol (10 ms) of the leader

short intPSK100bdCarTemplate[9][4][120];	// The actual templates over 9 carriers for 4 phase values and 120 samples
    //   (only positive Phase values are in the table, sign reversal is used to get the negative phase values) This reduces the table size from 7680 to 3840 integers
short intPSK200bdCarTemplate[9][4][72];		// Templates for 200 bd with cyclic prefix
short intFSK25bdCarTemplate[16][480];		// Template for 16FSK carriers spaced at 25 Hz, 25 baud
short intFSK50bdCarTemplate[4][240];		// Template for 4FSK carriers spaced at 50 Hz, 50 baud
short intFSK100bdCarTemplate[20][120];		// Template for 4FSK carriers spaced at 100 Hz, 100 baud
short intFSK600bdCarTemplate[4][20];		// Template for 4FSK carriers spaced at 600 Hz, 600 baud  (used for FM only)


void GenerateTwoToneLeaderTemplate()
{
	// to create leader alternate these template samples reversing sign on each adjacent symbol
    
	int i;
	float x, y, z;
	int line = 0;

	FILE * fp1;

	char msg[80];
	int len;

	fp1 = fopen("s:\leadercoeffs.txt", "wb");

	for (i = 0; i < 120; i++)
	{
		y = (sin(((1500.0 - 50) / 1500) * (i / 8.0 * 2 * M_PI)));
		z = (sin(((1500.0 + 50) / 1500) * (i / 8.0 * 2 * M_PI)));

		x = intAmp * 0.6 * (y - z);
		intTwoToneLeaderTemplate[i] = (short)x + 0.5;

		if ((i - line) == 9)
		{
			// print the last 10 values

			len = sprintf(msg, "\t%d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
				intTwoToneLeaderTemplate[line],
				intTwoToneLeaderTemplate[line + 1],
				intTwoToneLeaderTemplate[line + 2],
				intTwoToneLeaderTemplate[line + 3],
				intTwoToneLeaderTemplate[line + 4],
				intTwoToneLeaderTemplate[line + 5],
				intTwoToneLeaderTemplate[line + 6],
				intTwoToneLeaderTemplate[line + 7],
				intTwoToneLeaderTemplate[line + 8],
				intTwoToneLeaderTemplate[line + 9]);

			line = i + 1;

			fwrite(msg, 1, len, fp1);
		}
	}		
	fclose(fp1);
}

// Subroutine to create the FSK symbol templates

void GenerateFSKTemplates()
{
	// Generate templates of 240 samples (each symbol template = 20 ms) for each of the 4 possible carriers used in 200 Hz BW FSK modulation.
	// Generate templates of 120 samples (each symbol template = 10 ms) for each of the 20 possible carriers used in 500, 1000 and 2000 Hz BW 4FSK modulation.
	//Used to speed up computation of FSK frames and reduce use of Sin functions.
	//50 baud Tone values 

	// the possible carrier frequencies in Hz ' note gaps for groups of 4 at 900, 1400, and 1900 Hz improved isolation between simultaneous carriers

	float dblCarFreq[] = {1425, 1475, 1525, 1575, 600, 700, 800, 900, 1100, 1200, 1300, 1400, 1600, 1700, 1800, 1900, 2100, 2200, 2300, 2400};

	float dblAngle;		// Angle in radians
	float dblCarPhaseInc[20]; 
	int i, k;

	char msg[80];
	int len;
	int line = 0;
	FILE * fp1;

	fp1 = fopen("s:\\fskcoeffs.txt", "wb");


	// Compute the phase inc per sample

    for (i = 0; i < 4; i++) 
	{
		dblCarPhaseInc[i] = 2 * M_PI * dblCarFreq[i] / 12000;
	}
	
	// Now compute the templates: (960 32 bit values total) 

	len = sprintf(msg, "%s\n", "// FSK 50");
	fwrite(msg, 1, len, fp1);
	
	for (i = 0; i < 4; i++)			// across the 4 tones for 50 baud frequencies
	{
		dblAngle = 0;
		// 50 baud template

		line = 0;

		for (k = 0; k < 240; k++)	// for 240 samples (one 50 baud symbol)
		{
	//		intFSK50bdCarTemplate[i][k] = intAmp * 1.1 * sin(dblAngle);  // with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
			dblAngle += dblCarPhaseInc[i];

			if (dblAngle >= 2 * M_PI)
				dblAngle -= 2 * M_PI;
		
			if ((k - line) == 9)
			{
				// print the last 10 values

				len = sprintf(msg, "\t%d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
				intFSK50bdCarTemplate[i][line],
				intFSK50bdCarTemplate[i][line + 1],
				intFSK50bdCarTemplate[i][line + 2],
				intFSK50bdCarTemplate[i][line + 3],
				intFSK50bdCarTemplate[i][line + 4],
				intFSK50bdCarTemplate[i][line + 5],
				intFSK50bdCarTemplate[i][line + 6],
				intFSK50bdCarTemplate[i][line + 7],
				intFSK50bdCarTemplate[i][line + 8],
				intFSK50bdCarTemplate[i][line + 9]);

				line = k + 1;

				fwrite(msg, 1, len, fp1);
			}
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
	fclose(fp1);
}

//	 Subroutine to initialize valid frame types 

const int SamplesToComplete[256];

void InitValidFrameTypes()
{
	int i;
	
	bytValidFrameTypesLength = 0;
	
	for (i = 0; i < 256; i++)
	{
		if (IsValidFrameType(i))
			bytValidFrameTypes[bytValidFrameTypesLength++] = i;

		if (IsValidFrameType(i) && (SamplesToComplete[i] == -1))
			break;

	}
}
