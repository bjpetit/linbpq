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

void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen)
{
	// Function to Modulate data encoded for 4FSK and create the integer array of 32 bit samples suitable for playing 
	// Function works for 1, 2 or 4 simultaneous carriers 

	int intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intDataPtr, intSampPerSym, intDataBytesPerCar;
	BOOL blnOdd;

	short intSample;

    char strType[16] = "";
    char strMod[16] = "";

	UCHAR bytSymToSend, bytMask, bytMinQualThresh;
	UCHAR bytLastSym[8];	// Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 

	float dblCarScalingFactor;
	int intMask = 0;
	int intLeaderLenMS;
	int j, k, m, n;

	if (!FrameInfo(Type, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
		return;

	if (strcmp(strMod, "4FSK") != 0)
		return;

	initFilter(30000);


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
        // For FEC transmission the computed leader length = MCB.Leader length    
		intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (Len - 2) / intNumCar);

		GetTwoToneLeaderWithSync(intLeaderLenMS / 10);

		break;
                
	case 100:
		
		intSampPerSym = 120;
		intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (Len - 2) / intNumCar);
		GetTwoToneLeaderWithSync(intLeaderLenMS / 10);
	}
	
	// Create the leader
	
	intDataBytesPerCar = (Len - 2) / intNumCar;		// We queue the samples here, so dont copy below
    
	//Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
     

	//Create the 8 symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
	// No reference needed for 4FSK

	// note revised To accomodate 1 parity symbol per byte (10 symbols total)

	for(j = 0; j < 2; j++)		 // for the 2 bytes of the frame type
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

	//' No reference needed for 4FSK

	intDataPtr = 2;

	switch(intNumCar)
	{
	case 1:			 // use carriers 0-4
		
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

		dblCarScalingFactor = 0.51; //  (scaling factors determined emperically to minimize crest factor)

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
				}

				bytMask = bytMask >> 2;
				SampleSink(intSample);
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
*/	
// Function to apply 500 Hz filter for transmit 
void FSXmtFilter500_1500Hz(short * intNewSamples, int Length)
{
	// Used for FSK modulation XMIT filter  
	// assumes sample rate of 12000
	// implements 7 100 Hz wide sections centered on 1500 Hz  (~500 Hz wide @ - 30dB centered on 1500 Hz)
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
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered" & strFilename, 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered" & strFilename, 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[Filters.FSXmtFilterFSK500_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
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
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered2000Hz.wav", 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered2000Hz.wav", 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.FSXmitFilterFSK2000_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
    End Function  'FSXmtFilter2000_1500Hz
*/

