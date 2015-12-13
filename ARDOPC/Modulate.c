//	Sample Creation routines (encode and filter) for ARDOP Modem

#include "ARDOPC.h"

FILE * fp1;

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// Function to generate the Two-tone leader and Frame Sync (used in all frame types) 

extern short Dummy;
void GetTwoToneLeaderWithSync(int intSymLen)
{
	// Generate a 100 baud (10 ms symbol time) 2 tone leader 
	// leader tones used are 1450 and 1550 Hz.  
  
	int intSign = 1;
	int i, j;
	short intSample;

    if ((intSymLen & 1) == 1) 
		intSign = -1;

	for (i = 0; i < intSymLen; i++)   //for the number of symbols needed (two symbols less than total leader length) 
	{
		for (j = 0; j < 120; j++)	// for 120 samples per symbol (100 baud) 
		{
           if (i != (intSymLen - 1)) 
			   intSample = intSign * intTwoToneLeaderTemplate[j];
		   else
			   intSample = -intSign * intTwoToneLeaderTemplate[j];
   
		   SampleSink(intSample);
		}
		intSign = -intSign;
	}
}

void SendLeaderAndSYNC(UCHAR * bytEncodedBytes, int intLeaderLen)
{
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, n;
	UCHAR bytMask;
	UCHAR bytSymToSend;
	short intSample;
	if (intLeaderLen == 0)
		intLeaderLenMS = LeaderLength;
	else
		intLeaderLenMS = intLeaderLen;

 	// Create the leader

	GetTwoToneLeaderWithSync(intLeaderLenMS / 10);
		       
	//Create the 8 symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
	// No reference needed for 4FSK

	// note revised To accomodate 1 parity symbol per byte (10 symbols total)

	for(j = 0; j < 2; j++)		 // for the 2 bytes of the frame type
	{              
		bytMask = 0xc0;
		
		for(k = 0; k < 5; k++)	 // for 5 symbols per byte (4 data + 1 parity)
		{
			if (k < 4)
				bytSymToSend = (bytMask & bytEncodedBytes[j]) >> (2 * (3 - k));
			else
				bytSymToSend = ComputeTypeParity(bytEncodedBytes[0]);

			for(n = 0; n < 240; n++)
			{
				if (((5 * j + k) & 1 ) == 0)
					intSample = intFSK50bdCarTemplate[bytSymToSend][n];
				else
					intSample = -intFSK50bdCarTemplate[bytSymToSend][n]; // -sign insures no phase discontinuity at symbol boundaries

				SampleSink(intSample);	
			}
			bytMask = bytMask >> 2;
		}
	}
}


void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK, create
	// the 16 bit samples and send to sound interface

	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	int intSample;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;

	float dblCarScalingFactor;
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "4FSK") != 0)
		return;

	StopCapture();

	if (intBaud == 50)
		initFilter(200);
	else if (intNumCar == 1)
		initFilter(500);
	else if (intNumCar == 2)
		initFilter(1000);
	else if (intNumCar == 4)
		initFilter(2000);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If


	if (intLeaderLen == 0)
		intLeaderLenMS = LeaderLength;
	else
		intLeaderLenMS = intLeaderLen;

    switch(intBaud)
	{		
	case 50:
		
		intSampPerSym = 240;
		break;
                
	case 100:
		
		intSampPerSym = 120;
	}
		
	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below
    
	SendLeaderAndSYNC(bytEncodedBytes, intLeaderLen);

	intDataPtr = 2;

	switch(intNumCar)
	{
	case 1:			 // use carriers 0-3
		
		dblCarScalingFactor = 1.0; //  (scaling factors determined emperically to minimize crest factor) 

		for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data
		{
			bytMask = 0xC0;		 // Initialize mask each new data byte
			
			for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
			{
				bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr]) >> (2 * (3 - k)); // Values 0-3

				for (n = 0; n < intSampPerSym; n++)	 // Sum for all the samples of a symbols 
				{
					if((k & 1) == 0)
					{
						if(intBaud == 50)
							intSample = intFSK50bdCarTemplate[bytSymToSend][n];
						else
							intSample = intFSK100bdCarTemplate[bytSymToSend][n];
							
						SampleSink(intSample);
					}
					else
 					{
						if(intBaud == 50)
							intSample = -intFSK50bdCarTemplate[bytSymToSend][n];
						else
							intSample = -intFSK100bdCarTemplate[bytSymToSend][n];
							
						SampleSink(intSample);	
					}
				}

				bytMask = bytMask >> 2;
			}
			intDataPtr += 1;
		}

		SoundFlush();

		break;

	case 2:			// use carriers 8-15 (100 baud only)

		dblCarScalingFactor = 0.51f; //  (scaling factors determined emperically to minimize crest factor)

		for (m = 0; m < intDataBytesPerCar; m++)	  // For each byte of input data 
		{
			bytMask = 0xC0;	// Initialize mask each new data byte
                        			
			for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
			{
				for (n = 0; n < intSampPerSym; n++)	 // for all the samples of a symbol for 2 carriers
				{
					//' First carrier
                      
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
					intSample = intFSK100bdCarTemplate[8 + bytSymToSend][n];
					// Second carrier
                    
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr + intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSample = dblCarScalingFactor * (intSample + intFSK100bdCarTemplate[12 + bytSymToSend][n]);
			
					SampleSink(intSample);
				}
				bytMask = bytMask >> 2;
			}
			intDataPtr += 1;
		}
             
		SoundFlush();

		break;

	case 4:		 // use carriers 4-19 (100 baud only)

 		dblCarScalingFactor = 0.27f; //  (scaling factors determined emperically to minimize crest factor)

		for (m = 0; m < intDataBytesPerCar; m++)	  // For each byte of input data 
		{
			bytMask = 0xC0;	// Initialize mask each new data byte
                        			
			for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
			{
				for (n = 0; n < intSampPerSym; n++)	 // for all the samples of a symbol for 2 carriers
				{
					//' First carrier
                      
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
					intSample = intFSK100bdCarTemplate[4 + bytSymToSend][n];
					// Second carrier
                    
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr + intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSample = intSample + intFSK100bdCarTemplate[8 + bytSymToSend][n];
			
					//' Third carrier
					
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr + 2 * intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSample = intSample + intFSK100bdCarTemplate[12 + bytSymToSend][n];

					// ' Fourth carrier
   
					bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr + 3 * intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSample = dblCarScalingFactor * (intSample + intFSK100bdCarTemplate[16 + bytSymToSend][n]);

					SampleSink(intSample);
				}
				bytMask = bytMask >> 2;
			}
			intDataPtr += 1;
		}       
		SoundFlush();
		break;
	}
}

// Function to Modulate encoded data to 8FSK and send to sound interface

void Mod8FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 8FSK, create
	// the 16 bit samples and send to sound interface

	int intBaud, intDataLen, intRSLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;
	int intNumCar;

	short intSample;
	unsigned int intThreeBytes = 0;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMinQualThresh;
	int intMask = 0;
	int k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "8FSK") != 0)
		return;

	initFilter(200);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If


	intSampPerSym = 240;
	
	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below
    
	SendLeaderAndSYNC(bytEncodedBytes, intLeaderLen);

	intSampPerSym = 480;			// 25 Baud

	intDataPtr = 2;

 	for (m = 0; m < intDataBytesPerCar; m += 3)  // For each byte of input data
	{
		intThreeBytes = bytEncodedBytes[intDataPtr++];
		intThreeBytes = (intThreeBytes << 8) + bytEncodedBytes[intDataPtr++];
		intThreeBytes = (intThreeBytes << 8) + bytEncodedBytes[intDataPtr++];
		intMask = 0xE00000;
                 
		for (k = 0; k < 8; k++)
		{
			bytSymToSend = (intMask & intThreeBytes) >> (3 * (7 - k));

			// note value of "+ 4" below allows using 16FSK template for 8FSK using only the "inner" 8 tones around 1500

			for (n = 0; n < intSampPerSym; n++)	 // Sum for all the samples of a symbols 
			{
				if((k & 1) == 0)
					intSample = intFSK25bdCarTemplate[bytSymToSend + 4][n]; // Symbol vlaues 4- 11 (surrounding 1500 Hz)  
				else
					intSample = -intFSK25bdCarTemplate[bytSymToSend + 4][n]; // Symbol vlaues 4- 11 (surrounding 1500 Hz)  

				SampleSink(intSample);
			}
			intMask = intMask >> 3;
		}
	}
	SoundFlush();
}

// Function to Modulate encoded data to 16FSK and send to sound interface

void Mod16FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 16FSK, create
	// the 16 bit samples and send to sound interface

	int intBaud, intDataLen, intRSLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;
	int intNumCar;

	short intSample;
	unsigned int intThreeBytes = 0;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;

	int intMask = 0;
	int k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "16FSK") != 0)
		return;

	StopCapture();

	initFilter(500);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If

	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below    
	intSampPerSym = 480;			// 25 Baud

	SendLeaderAndSYNC(bytEncodedBytes, intLeaderLen);

	intDataPtr = 2;

	for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data 
	{
		bytMask = 0xF0;	 // Initialize mask each new data byte
		for (k = 0; k < 2; k++)	// for 2 symbol values per byte of data
		{
			bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr]) >> (4 * (1 - k)); // Values 0 - 15

			for (n = 0; n < intSampPerSym; n++)	 // Sum for all the samples of a symbols 
			{
				if((k & 1) == 0)
					intSample = intFSK25bdCarTemplate[bytSymToSend][n];
				else
					intSample = -intFSK25bdCarTemplate[bytSymToSend][n];

				SampleSink(intSample);
			}
			bytMask = bytMask >> 4;
		}
		intDataPtr++;
	}
	SoundFlush();
}

//	Function to Modulate data encoded for 4FSK High baud rate and create the integer array of 32 bit samples suitable for playing 



void Mod4FSK600BdDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK, create
	// the 16 bit samples and send to sound interface

	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	short intSample;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;

	int intMask = 0;
	int k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "4FSK") != 0)
		return;

	StopCapture();

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If

	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below

	intSampPerSym = 12000 / intBaud;
    
	SendLeaderAndSYNC(bytEncodedBytes, intLeaderLen);

	intDataPtr = 2;

	for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data
	{
		bytMask = 0xC0;		 // Initialize mask each new data byte			
		for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
		{
			bytSymToSend = (bytMask & bytEncodedBytes[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
			for (n = 0; n < intSampPerSym; n++)	 // Sum for all the samples of a symbols 
			{
    			intSample = intFSK600bdCarTemplate[bytSymToSend][n];
				SampleSink(intSample);
			}
			bytMask = bytMask >> 2;
		}
		intDataPtr += 1;
	}
	SoundFlush();
}


// Function to extract an 8PSK symbol from an encoded data array


UCHAR GetSym8PSK(int intDataPtr, int k, int intCar, UCHAR * bytEncodedBytes, int intDataBytesPerCar)
{
	int int3Bytes = bytEncodedBytes[intDataPtr + intCar * intDataBytesPerCar];
//	int intMask  = 7;
	int intSym;
	UCHAR bytSym;

	int3Bytes = int3Bytes << 8;
	int3Bytes += bytEncodedBytes[intDataPtr + intCar * intDataBytesPerCar + 1];
	int3Bytes = int3Bytes << 8;
	int3Bytes += bytEncodedBytes[intDataPtr + intCar * intDataBytesPerCar + 2];  // now have 3 bytes, 24 bits or 8 8PSK symbols 
//	intMask = intMask << (3 * (7 - k));
	intSym = int3Bytes >> (3 * (7 - k));
	bytSym = intSym & 7;	//(intMask && int3Bytes) >> (3 * (7 - k));

	return bytSym;
}

// Function to Modulate data encoded for PSK, create
// the 16 bit samples and send to sound interface
   

void ModPSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen)
{
	int intNumCar, intBaud, intDataLen, intRSLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	int intSample;
    char strType[16] = "";
    char strMod[16] = "";
	UCHAR bytSym, bytSymToSend, bytMask, bytMinQualThresh;
	float dblCarScalingFactor;
	int intMask = 0;
	int intLeaderLenMS;
	int i, j, k, m, n;
	int intCarStartIndex;
	int intPeakAmp;
	int intCarIndex;

	UCHAR bytLastSym[9]; // = {0}; // Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 
 
	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (intNumCar == 1)
		initFilter(200);
	else if (intNumCar == 2)
		initFilter(500);
	else if (intNumCar == 4)
		initFilter(1000);
	else if (intNumCar == 8)
		initFilter(2000);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
//               strLastWavStream = strType
//          End If

	if (intLeaderLen == 0)
		intLeaderLenMS = LeaderLength;
	else
		intLeaderLenMS = intLeaderLen;

	switch(intBaud)
	{		
	case 100:
		
		intSampPerSym = 120;
		break;
                
	case 167:
		
		intSampPerSym = 72; // the total number of samples per symbol 
		break;
	}
	
	// Create the leader
	
	switch(intNumCar)
	{		
	case 1:
		intCarStartIndex = 4;
		dblCarScalingFactor = 1.0f; // Starting at 1500 Hz  (scaling factors determined emperically to minimize crest factor)  TODO:  needs verification
		break;
	case 2:
		intCarStartIndex = 3;
		dblCarScalingFactor = 0.53f; // Starting at 1400 Hz
		break;
	case 4:
		intCarStartIndex = 2;
		dblCarScalingFactor = 0.29f; // Starting at 1200 Hz
		break;
	case 8:
		intCarStartIndex = 0;
		dblCarScalingFactor = 0.17f; // Starting at 800 Hz
	} 

	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below

	// Create the leader

	SendLeaderAndSYNC(bytEncodedBytes, intLeaderLen);

	intPeakAmp = 0;
            
	// Now create a reference symbol for each carrier
      
	//	We have to do each carrier for each sample, as we write
	//	the sample immediately 


	for (n = 0; n < intSampPerSym; n++)  // Sum for all the samples of a symbols 
	{
		intSample = 0;
		intCarIndex = intCarStartIndex;  // initialize to correct starting carrier

		for (i = 0; i < intNumCar; i++)	// across all carriers
		{
			bytSymToSend = 0;  //  using non 0 causes error on first data byte 12/8/2014   ...Values 0-3  not important (carries no data).   (Possible chance for Crest Factor reduction?)
                
			bytLastSym[intCarIndex] = bytSymToSend;

			if (intBaud == 100)
				intSample += intPSK100bdCarTemplate[intCarIndex][0][n];  // double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
			else
				intSample -= intPSK200bdCarTemplate[intCarIndex][0][n]; // subtract 2 from the symbol value before doubling and subtract value of table 

			intCarIndex += 1;
			if (intCarIndex == 4)
				intCarIndex += 1;	// skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
		}
		intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
		SampleSink(intSample);
	}
      
	// End of reference phase generation 

	intDataPtr = 2;  // initialize pointer to start of data.
		
	if (strcmp(strMod, "4PSK") == 0)
	{
		for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data (all carriers) 
		{
			bytMask = 0xC0; // Initialize mask each new data byte
                        
			for (k = 0; k < 4; k++)  // for 4 symbol values per byte of data
			{                 
				for (n = 0; n < intSampPerSym; n++)  // Sum for all the samples of a symbols 
				{
					intSample = 0;
					intCarIndex = intCarStartIndex; // initialize the carrrier index
	
					for (i = 0; i < intNumCar ; i++) // across all carriers
					{
						bytSym = (bytMask & bytEncodedBytes[intDataPtr + i * intDataBytesPerCar]) >> (2 * (3 - k));
						bytSymToSend = ((bytLastSym[intCarIndex] + bytSym) & 3);  // Values 0-3
			
						if (intBaud == 100)
						{
							if (bytSymToSend < 2)
								intSample += intPSK100bdCarTemplate[intCarIndex][bytSymToSend * 2][n];  // double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
							else
								intSample -= intPSK100bdCarTemplate[intCarIndex][2 * (bytSymToSend - 2)][n]; // subtract 2 from the symbol value before doubling and subtract value of table 
						}
						else
						{
							if (bytSymToSend < 2)
								intSample += intPSK200bdCarTemplate[intCarIndex][bytSymToSend * 2][n];  // double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
							else
								intSample -= intPSK200bdCarTemplate[intCarIndex][2 * (bytSymToSend - 2)][n]; // subtract 2 from the symbol value before doubling and subtract value of table 
						}
						if (n == intSampPerSym - 1)		// Last sample?
							bytLastSym[intCarIndex] = bytSymToSend;

						intCarIndex += 1;
						if (intCarIndex == 4)
							intCarIndex += 1;	// skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
					}
					intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
					if (intSample > 32767 || intSample < -32767)
						printf("too big");
					
					SampleSink(intSample);		
				}       
				bytMask = bytMask >> 2;
			}
			intDataPtr += 1;
		}
	}
	else if (strcmp(strMod, "8PSK") == 0)
	{
		// More complex ...must go through data in 3 byte chunks creating 8 Three bit symbols for each 3 bytes of data. 
     
		for (m = 0; m < intDataBytesPerCar / 3; m++)
		{
			for (k = 0; k < 8; k++) // for 8 symbols in 24 bits of int3Bytes
			{
				for (n = 0; n < intSampPerSym; n++)	//  Sum for all the samples of a symbols 
				{
					intSample = 0;
		
					// We have to sum all samples for all carriers

					intCarIndex = intCarStartIndex;
				
					for (i = 0; i < intNumCar; i++)
					{
						bytSym = GetSym8PSK(intDataPtr, k, i, bytEncodedBytes, intDataBytesPerCar);
						bytSymToSend = ((bytLastSym[intCarIndex] + bytSym) & 7);	// mod 8
				
						if (intBaud == 100)
						{
							if (bytSymToSend < 4) // This uses the symmetry of the symbols to reduce the table size by a factor of 2
								intSample += intPSK100bdCarTemplate[intCarIndex][bytSymToSend][n]; // positive phase values template lookup for 8PSK.
							else
								intSample -= intPSK100bdCarTemplate[intCarIndex][bytSymToSend - 4][n]; // negative phase values,  subtract value of table 
						}
						else
						{
							if (bytSymToSend < 4) // This uses the symmetry of the symbols to reduce the table size by a factor of 2
								intSample += intPSK100bdCarTemplate[intCarIndex][bytSymToSend][n]; // positive phase values template lookup for 8PSK.
							else
								intSample -= intPSK100bdCarTemplate[intCarIndex][bytSymToSend - 4][n]; // negative phase values,  subtract value of table 
						}

						if (n == intSampPerSym - 1)		// Last sample?
							bytLastSym[intCarIndex] = bytSymToSend;

						intCarIndex += 1;
						if (intCarIndex == 4)
							intCarIndex += 1;  // skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
					}
					intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
					if (intSample > 32767 || intSample < -32767)
						printf("too big");
					
					SampleSink(intSample);		
				}
			}
			intDataPtr += 3;
		}
	}
	SoundFlush();
}


// Subroutine to add trailer before filtering

void AddTrailer()
{
	int intAddedSymbols = 1 + TrailerLength / 10; // add 1 symbol + 1 per each 10 ms of MCB.Trailer
	int i, k;

	for (i = 1; i <= intAddedSymbols; i++)
	{
		for (k = 0; k < 120; k++)
		{
			SampleSink(intPSK100bdCarTemplate[4][0][k]);
		}
	}
}

//	Resends the last frame

void RemodulateLastFrame()
{	
	int intNumCar, intBaud, intDataLen, intRSLen;
	int bytMinQualThresh;
	BOOL blnOdd;

	char strType[16] = "";
    char strMod[16] = "";

	if (!FrameInfo(bytEncodedBytes[0], &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "4FSK") == 0)
	{
		if (bytEncodedBytes[0] >= 0x7A && bytEncodedBytes[0] <= 0x7D)
			Mod4FSK600BdDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		else
			Mod4FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 

		return;
	}
	if (strcmp(strMod, "16FSK") == 0)
	{
		Mod16FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		return;
	}
	if (strcmp(strMod, "8FSK") == 0)
	{
		Mod8FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		return;
	}
	ModPSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
}
	
