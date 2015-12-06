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

void SendLeaderAndSYNC(UCHAR * bytEncodedData, int intLeaderLen)
{
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;
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
				bytSymToSend = (bytMask & bytEncodedData[j]) >> (2 * (3 - k));
			else
				bytSymToSend = ComputeTypeParity(bytEncodedData[0]);

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


void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK, create
	// the 16 bit samples and send to sound interface

	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	short intSample;

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
    
	SendLeaderAndSYNC(bytEncodedData, intLeaderLen);

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
				bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (2 * (3 - k)); // Values 0-3

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
                      
					bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
					intSample = intFSK100bdCarTemplate[8 + bytSymToSend][n];
					// Second carrier
                    
					bytSymToSend = (bytMask & bytEncodedData[intDataPtr + intDataBytesPerCar]) >> (2 * (3 - k));	// Values 0-3
					intSample = dblCarScalingFactor * (intSample + intFSK100bdCarTemplate[12 + bytSymToSend][n]);
			
					SampleSink(intSample);
				}
				bytMask = bytMask >> 2;
			}
			intDataPtr += 1;
		}
             
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

// Function to Modulate encoded data to 8FSK and send to sound interface

void Mod8FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 8FSK, create
	// the 16 bit samples and send to sound interface

	int intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;
	int intNumCar;

	short intSample;
	unsigned int intThreeBytes = 0;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "8FSK") != 0)
		return;

	StopCapture();

	initFilter(200);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If


	intSampPerSym = 240;
	
	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below
    
	SendLeaderAndSYNC(bytEncodedData, intLeaderLen);

	intSampPerSym = 480;			// 25 Baud

	intDataPtr = 2;

 	for (m = 0; m < intDataBytesPerCar; m += 3)  // For each byte of input data
	{
		intThreeBytes = bytEncodedData[intDataPtr++];
		intThreeBytes = (intThreeBytes << 8) + bytEncodedData[intDataPtr++];
		intThreeBytes = (intThreeBytes << 8) + bytEncodedData[intDataPtr++];
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

void Mod16FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 16FSK, create
	// the 16 bit samples and send to sound interface

	int intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;
	int intNumCar;

	short intSample;
	unsigned int intThreeBytes = 0;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;

	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;

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

	SendLeaderAndSYNC(bytEncodedData, intLeaderLen);

	intDataPtr = 2;

	for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data 
	{
		bytMask = 0xF0;	 // Initialize mask each new data byte
		for (k = 0; k < 2; k++)	// for 2 symbol values per byte of data
		{
			bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (4 * (1 - k)); // Values 0 - 15

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



void Mod4FSK600BdDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK, create
	// the 16 bit samples and send to sound interface

	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	short intSample;

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

	initFilter(2000);

//	If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
 //               strLastWavStream = strType
  //          End If

	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below

	intSampPerSym = 12000 / intBaud;
    
	SendLeaderAndSYNC(bytEncodedData, intLeaderLen);

	intDataPtr = 2;

	for (m = 0; m < intDataBytesPerCar; m++)  // For each byte of input data
	{
		bytMask = 0xC0;		 // Initialize mask each new data byte			
		for (k = 0; k < 4; k++)		// for 4 symbol values per byte of data
		{
			bytSymToSend = (bytMask & bytEncodedData[intDataPtr]) >> (2 * (3 - k)); // Values 0-3
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


UCHAR GetSym8PSK(int intDataPtr, int k, int intCar, UCHAR * bytEncodedData, int intDataBytesPerCar)
{
	int int3Bytes = bytEncodedData[intDataPtr + intCar * intDataBytesPerCar];
//	int intMask  = 7;
	int intSym;
	UCHAR bytSym;

	int3Bytes = int3Bytes << 8;
	int3Bytes += bytEncodedData[intDataPtr + intCar * intDataBytesPerCar + 1];
	int3Bytes = int3Bytes << 8;
	int3Bytes += bytEncodedData[intDataPtr + intCar * intDataBytesPerCar + 2];  // now have 3 bytes, 24 bits or 8 8PSK symbols 
//	intMask = intMask << (3 * (7 - k));
	intSym = int3Bytes >> (3 * (7 - k));
	bytSym = intSym & 7;	//(intMask && int3Bytes) >> (3 * (7 - k));

	return bytSym;
}



/// Function to Modulate data encoded for PSK, create
// the 16 bit samples and send to sound interface
   
 
void ModPSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	int intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	short intSample;
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
	UCHAR bytLastSym[8]; // Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 
 

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	StopCapture();

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

		if (strcmp(strMod, "4PSK") == 0)
			//	add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym for initial reference phase
			intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * (4 * (intDataLen + intRSLen + 3));
		else
		if (strcmp(strMod, "8PSK") == 0)
			intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * ((8 * (intDataLen + intRSLen + 3)) / 3);

		break;
                
	case 167:
		
		intSampPerSym = 72; // the total number of samples per symbol 
         
		if (strcmp(strMod, "4PSK") == 0)
			intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * (4 * (intDataLen + intRSLen + 3)) ;
			// add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym for initial reference phase
 		else
		if (strcmp(strMod, "8PSK") == 0)
			intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + +intSampPerSym * ((8 * (intDataLen + intRSLen + 3)) / 3) ;

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

	GetTwoToneLeaderWithSync(intLeaderLenMS / 10);

	// Create the 10 symbols (20 bit) 50 baud 4FSK frame type with Implied SessionID and 2 symbol parit
	// No reference needed for 4FSK
         
	for (j = 0; j < 2; j++)	// for the 2 bytes of the frame type
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
					intSample = intFSK50bdCarTemplate[bytSymToSend][n];
				else
					intSample = -intFSK50bdCarTemplate[bytSymToSend][n]; // -sign insures no phase discontinuity at symbol boundaries
					SampleSink(intSample);	
			}
			bytMask = bytMask >> 2;
		}
	}

	intPeakAmp = 0;
	intCarIndex = intCarStartIndex;  // initialize to correct starting carrier
            
	// Now create a reference symbol for each carrier
      
	//	We have to do each carrier for each sample, as we write
	//	the sample immediately - this is ok for 1 carrier


	for (i = 0; i < intNumCar; i++)	// across all carriers
	{
		bytSymToSend = 0;  //  using non 0 causes error on first data byte 12/8/2014   ...Values 0-3  not important (carries no data).   (Possible chance for Crest Factor reduction?)
                
		bytLastSym[intCarIndex] = bytSymToSend;

		for (n = 0; n < intSampPerSym; n++)  // Sum for all the samples of a symbols 
		{
			intSample = 0;

			if (intBaud == 100)
			{
				if (bytSymToSend < 2)
					intSample += intPSK100bdCarTemplate[intCarIndex][bytSymToSend * 2][n];  // double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
				else
					intSample -= intPSK100bdCarTemplate[intCarIndex][2 * (bytSymToSend - 2)][n]; // subtract 2 from the symbol value before doubling and subtract value of table 

				if (i == intNumCar -1)
					intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
			}
			else if (intBaud == 167)
			{
				if (bytSymToSend < 2) // This uses the symmetry of the symbols to reduce the table size by a factor of 2
					intSample += intPSK200bdCarTemplate[intCarIndex][bytSymToSend * 2][n];  // double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
				else
					intSample -= intPSK200bdCarTemplate[intCarIndex][2 * (bytSymToSend - 2)][n]; // subtract 2 from the symbol value before doubling and subtract value of table 

				if (i == intNumCar - 1)
					intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
			}
			
	// need attention for multiple carriers

			SampleSink(intSample);
		}

	
		intCarIndex += 1;
		if (intCarIndex == 4)
			intCarIndex += 1;	// skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
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
				intCarIndex = intCarStartIndex; // initialize the carrrier index
                 
				for (i = 0; i < intNumCar ; i++) // across all carriers
				{
					bytSym = (bytMask & bytEncodedData[intDataPtr + i * intDataBytesPerCar]) >> (2 * (3 - k));
					bytSymToSend = ((bytLastSym[intCarIndex] + bytSym) & 3);  // Values 0-3

					for (n = 0; n < intSampPerSym; n++)  // Sum for all the samples of a symbols 
					{
						intSample = 0;

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
						if (i == intNumCar - 1)
							intSample = intSample * dblCarScalingFactor; // on the last carrier rescale value based on # of carriers to bound output
					
						bytLastSym[intCarIndex] = bytSymToSend;
						SampleSink(intSample);
		
					}
					intCarIndex += 1;
					if (intCarIndex == 4)
						intCarIndex += 1;	// skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
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
				intCarIndex = intCarStartIndex; // initialize the carrrier index

				for (i = 0; i < intNumCar; i++)
				{
					bytSym = GetSym8PSK(intDataPtr, k, i, bytEncodedData, intDataBytesPerCar);
					bytSymToSend = ((bytLastSym[intCarIndex] + bytSym) & 7);	// mod 8
					for (n = 0; n < intSampPerSym; n++)	//  Sum for all the samples of a symbols 
					{
						intSample = 0;

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
						if (i == intNumCar - 1)
							intSample = intSample * dblCarScalingFactor; // Need to examine clipping and scaling here 
						else
						{
									/*
									// need work ofr multi carier

                                        ' In testing

                                        If bytSymToSend < 4 Then  ' This uses the symmetry of the symbols to reduce the table size by a factor of 2
                                            intSamples(intSamplePtr + n) += intPSK200bdCarTemplate(intCarIndex, bytSymToSend, n)
                                        Else ' untested 
                                            intSamples(intSamplePtr + n) -= intPSK200bdCarTemplate(intCarIndex, (bytSymToSend - 4), n)
                                        End If
                                        If i = intNumCar - 1 Then
                                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor
                                        End If
                                    End If
									*/
						}
					
						bytLastSym[intCarIndex] = bytSymToSend;
						SampleSink(intSample);
					}
				
					intCarIndex += 1;
					if (intCarIndex == 4)
						intCarIndex += 1;  // skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
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
/*
// Function to apply 200 Hz filter for transmit  

void FSXmtFilter200_1500Hz(short * intNewSamples, int Length)
{
	// Used for PSK 200 Hz modulation XMIT filter  
	// assumes sample rate of 12000
	// implements 3 100 Hz wide sections centered on 1500 Hz  (~200 Hz wide @ - 30dB centered on 1500 Hz)

	// FSF (Frequency Selective Filter) variables

	static float dblR = 0.9995;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static float dblRn;

	static float dblR2;
	static float dblCoef[19] = {0.0};			// the coefficients
	float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	float dblZout_0[19] = {0.0};	// resonator outputs
	float dblZout_1[19] = {0.0};	// resonator outputs delayed one sample
	float dblZout_2[19] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j, x, xold;

	float intFilteredSample = 0;			//  Filtered sample
	float largest = 0;

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);

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

		x = xold = 0;
		
		if (i < intN)
			x  = intNewSamples[i];
		else if (i < Length)
		{
			x = intNewSamples[i];
			xold = intNewSamples[i - intN];
		}
		else
			xold = intNewSamples[i - intN];

		dblZin = x  -dblRn * xold;
	
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

//			intNewSamples[i - intFilLen] = (short)intFilteredSample; // & 0xfff0;
			xDummy[i - intFilLen] = (short)intFilteredSample; // & 0xfff0;
			largest = MAX(largest, intFilteredSample);
		}
		fprintf(fp1, "i %d s %d s_old %d dblZin %f out %f\r\n",
			i, x, xold, dblZin, intFilteredSample);

	}
}

// Function to apply 500 Hz filter for transmit 
void FSXmtFilter500_1500Hz(short * intNewSamples, int Length)
{
	// Used for FSK modulation XMIT filter  
	// assumes sample rate of 12000
	// implements 7 100 Hz wide sections centered on 1500 Hz  (~500 Hz wide @ - 30dB centered on 1500 Hz)
	// FSF (Frequency Selective Filter) variables
 
	static float dblR = 0.9995f;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static float dblRn;

	static float dblR2;
	static float dblCoef[19] = {0.0};			// the coefficients
	float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	float dblZout_0[19] = {0.0};	// resonator outputs
	float dblZout_1[19] = {0.0};	// resonator outputs delayed one sample
	float dblZout_2[19] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j;

	float intFilteredSample = 0;			//  Filtered sample

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);

	AddTrailer();  // add the trailer before filtering

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

        Static dblR As float = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As float = dblR ^ intN
        Static dblR2 As float = dblR ^ 2
        Static dblCoef(20) As float 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As float  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(20) As float ' resonator outputs
        Dim dblZout_1(20) As float ' resonator outputs delayed one sample
        Dim dblZout_2(20) As float  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As float 'for debug wave plotting
        'Dim dblFilteredSamples(intNewSamples.Length - 1) As float ' for debug wave plotting
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
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = FALSE Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered" & strFilename, 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered" & strFilename, 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[Filters.FSXmtFilterFSK500_1500Hz] Exception: " & ex.ToString)
            return Nothing
        End Try
        return intFilteredSamples
    End Function ' FSXmtFilter1000_1500Hz

    ' Function to apply 2000 Hz filter for transmit 
    Public Function FSXmtFilter2000_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for FSK modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 21 100 Hz wide sections centered on 1500 Hz  (~2000 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables

        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As float = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As float = dblR ^ intN
        Static dblR2 As float = dblR ^ 2
        Static dblCoef(25) As float 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As float  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(25) As float ' resonator outputs
        Dim dblZout_1(25) As float ' resonator outputs delayed one sample
        Dim dblZout_2(25) As float  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As float 'for debug wave plotting
        ' Dim dblFilteredSamples(intNewSamples.Length - 1) As float ' for debug wave plotting
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
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = FALSE Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered2000Hz.wav", 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered2000Hz.wav", 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.FSXmitFilterFSK2000_1500Hz] Exception: " & ex.ToString)
            return Nothing
        End Try
        return intFilteredSamples
    End Function  'FSXmtFilter2000_1500Hz
*/

