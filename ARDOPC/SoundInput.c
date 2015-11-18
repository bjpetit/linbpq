//	ARDOP Modem Decode Sound Samples

#include "ARDOPC.h"


BOOL Capturing = TRUE;

BOOL blnLeaderFound = FALSE;
BOOL blnLeaderDetected;
int intRcvdSamplesRPtr = 0;

short intRcvdSamples[12000];		// 1 second. May need to optimise

double dblOffsetHz;
time_t dttLastLeaderDetect;

BOOL Connected = FALSE;
time_t dttLastGoodFrameTypeDecod;
time_t dttStartRmtLeaderMeasure;
time_t dttStartRTMeasure;

int intARQRTmeasuredMs;
int TuningRange;

double dblSNdBPwr, dblSNdBPwr_1, dblSNdBPwr_2;
double dblNCOFreq = 3000;	 // nominal NC) frequency
double dblNCOPhase = 0;
double dblNCOPhaseInc = 2 * M_PI * 3000 / 12000;  // was dblNCOFreq

int	intMixedSamplesLen = 0;	//size of intMixedSamples
int	intPriorMixedSamplesLen = 119;  // size of Prior sample buffer
int	intFilteredMixedSamplesLen = 0;	// size of FilteredMixedSamples array
int	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay

int RcvdSamplesLen = 0;				// Samples in RX buffer

BOOL SearchFor2ToneLeader(short * intNewSamples, int Ptr, int Length, double * dblOffsetHz);

void ResetSNPwrs()
{
	dblSNdBPwr = 0;
	dblSNdBPwr_1 = 0;
	dblSNdBPwr_2 = 0;
}

//  Subroutine to Initialize mixed samples

InitializeMixedSamples()
{
	// Measure the time from release of PTT to leader detection of reply.

	intARQRTmeasuredMs = min(10000, time(NULL) - dttStartRTMeasure); //?????? needs work

	intMixedSamplesLen = 0;	// ' Zero the intMixedSamples array
	intPriorMixedSamplesLen = 119;  // zero out prior samples in Prior sample buffer
	intFilteredMixedSamplesLen = 0;	// zero out the FilteredMixedSamples array
	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay
}

//	Subroutine to discard all sampled prior to current intRcvdSamplesRPtr

DiscardOldSamples()
{
	// This restructures the intRcvdSamples array discarding all samples prior to intRcvdSamplesRPtr
 
	if (RcvdSamplesLen - intRcvdSamplesRPtr <= 0)
		RcvdSamplesLen = intRcvdSamplesRPtr = 0;
	else
	{
		// This is rather slow. I'd prefer a cyclic buffer. Lets see....
		
		memmove(intRcvdSamples, &intRcvdSamples[intRcvdSamplesRPtr], (RcvdSamplesLen - intRcvdSamplesRPtr)* 2);
		RcvdSamplesLen -= intRcvdSamplesRPtr;
		intRcvdSamplesRPtr = 0;
	}
}


// Subroutine to process new samples as received from the sound card via Main.ProcessCapturedData
// Only called when not transmitting

void ProcessNewSamples(short * Samples, int nSamples)
{
	int intRcvdSamplesWPtr = 0;
 //       Dim stcStatus As Status = Nothing
	BOOL blnFrameDecodedOK = FALSE;
	
//	Dim bytData(-1) As Byte
/*
            intRcvdSamplesWPtr = intRcvdSamples.Length
            ReDim Preserve intRcvdSamples(intRcvdSamples.Length + bytSamples.Length \ 2 - 1)
            For i As Integer = 0 To bytSamples.Length \ 2 - 1
                intRcvdSamples(intRcvdSamplesWPtr) = System.BitConverter.ToInt16(bytSamples, 2 * i)
                intRcvdSamplesWPtr += 1
            Next i
			*/
	if (ProtocolState == FECSend)
		return;

	// Searching for leader

	if (State == SearchingForLeader)
	{
		// Search for leader as long as 960 samples (8  symbols) available

		while (State == SearchingForLeader && (intRcvdSamplesRPtr + 960) <= nSamples)
		{
			ARDOPState = SearchFor2ToneLeader(Samples, intRcvdSamplesRPtr, nSamples, &dblOffsetHz);
			if (blnLeaderFound)
			{
				dttLastLeaderDetect = time(NULL);
                //        stcStatus.ControlName = "lblOffset"
                //        stcStatus.Text = "Offset: " & (Format(dblOffsetHz, "#0.0").PadLeft(6)) & " Hz"
                 //       queTNCStatus.Enqueue(stcStatus)

				InitializeMixedSamples();
				State = AcquireSymbolSync;
				ResetSNPwrs();
			}
		}
		if (State == SearchingForLeader)
		{
			DiscardOldSamples();
			return;
		}
	}

	// Got leader

	// Acquire Symbol Sync 
/*
    if (State == AcquireSymbolSync)
		Then
                objMain.objDemod.MixNCOFilter(intRcvdSamples, intRcvdSamplesRPtr, dblOffsetHz) ' Mix and filter new samples (Mixing consumes all intRcvdSamples)
                If (objMain.objDemod.intFilteredMixedSamples.Length - objMain.objDemod.intMFSReadPtr) > 500 Then
                    blnSymbolSyncFound = objMain.objDemod.Acquire2ToneLeaderSymbolFraming() ' adjust the pointer to the nominal symbol start based on phase
                    If blnSymbolSyncFound Then
                        State = ReceiveState.AcquireFrameSync
                    Else
                        DiscardOldSamples()
                        objMain.objDemod.ClearAllMixedSamples()
                        State = ReceiveState.SearchingForLeader
                        Exit Sub
                    End If
                End If
            End If

            ' Acquire Frame Sync
            If State = ReceiveState.AcquireFrameSync Then
                objMain.objDemod.MixNCOFilter(intRcvdSamples, intRcvdSamplesRPtr, dblOffsetHz) ' Mix and filter new samples
                blnFrameSyncFound = objMain.objDemod.AcquireFrameSyncRSB()
                If blnFrameSyncFound Then
                    State = ReceiveState.AcquireFrameType
                ElseIf objMain.objDemod.intMFSReadPtr > 12000 Then ' no Frame sync within 1000 ms (may want to make this limit a funciton of Mode and leaders)
                    DiscardOldSamples()
                    objMain.objDemod.ClearAllMixedSamples()
                    State = ReceiveState.SearchingForLeader
                End If
            End If

            ' Acquire Frame Type
            If State = ReceiveState.AcquireFrameType Then
                objMain.objDemod.MixNCOFilter(intRcvdSamples, intRcvdSamplesRPtr, dblOffsetHz) ' Mix and filter new samples
                intFrameType = objMain.objDemod.Acquire4FSKFrameType()
                If intFrameType = -2 Then
                    Exit Sub ' isufficient samples 
                ElseIf intFrameType = -1 Then  ' poor decode quality (large decode distance)
                    State = ReceiveState.SearchingForLeader
                    objMain.objDemod.ClearAllMixedSamples()
                    DiscardOldSamples()
                    stcStatus.BackColor = SystemColors.Control
                    stcStatus.Text = ""
                    stcStatus.ControlName = "lblRcvFrame"
                    queTNCStatus.Enqueue(stcStatus)
                Else
                    strCurrentFrame = objFrameInfo.Name(intFrameType)
                    If Not objFrameInfo.IsShortControlFrame(intFrameType) Then
                        stcStatus.BackColor = Color.Khaki
                        stcStatus.Text = strCurrentFrame
                        stcStatus.ControlName = "lblRcvFrame"
                        queTNCStatus.Enqueue(stcStatus)
                    End If

                    intSamplesToCompleteFrame = objFrameInfo.SamplesToComplete(intFrameType)
                    State = ReceiveState.AcquireFrame
                    If MCB.ProtocolMode = "FEC" And objFrameInfo.IsDataFrame(intFrameType) And GetARDOPProtocolState <> ProtocolState.FECSend Then
                        SetARDOPProtocolState(ProtocolState.FECRcv)
                    End If
                End If
            End If

            ' Acquire Frame
            If State = ReceiveState.AcquireFrame Then
                objMain.objDemod.MixNCOFilter(intRcvdSamples, intRcvdSamplesRPtr, dblOffsetHz) ' Mix and filter new samples
                If (objMain.objDemod.intFilteredMixedSamples.Length - objMain.objDemod.intMFSReadPtr) >= intSamplesToCompleteFrame + 240 Then
                    blnFrameDecodedOK = objMain.objDemod.DecodeFrame(intFrameType, bytData)
                    If blnFrameDecodedOK Then
                        If MCB.ProtocolMode = "FEC" Then
                            If objFrameInfo.IsDataFrame(intFrameType) Then ' check to see if a data frame
                                ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK)
                            ElseIf intFrameType = &H30 Then
                                AddTagToDataAndSendToHost(bytData, "IDF")
                            ElseIf intFrameType >= &H31 And intFrameType <= &H38 Then
                                ProcessUnconnectedConReqFrame(intFrameType, bytData)
                            End If
                        ElseIf MCB.ProtocolMode = "ARQ" Then
                            If Not blnTimeoutTriggered Then ProcessRcvdARQFrame(intFrameType, bytData, blnFrameDecodedOK) ' Process connected ARQ frames here 
                            If GetARDOPProtocolState = ProtocolState.DISC Then ' allows ARQ mode to operate like FEC when not connected
                                If intFrameType = &H30 Then
                                    AddTagToDataAndSendToHost(bytData, "IDF")
                                ElseIf intFrameType >= &H31 And intFrameType <= &H38 Then
                                    ProcessUnconnectedConReqFrame(intFrameType, bytData)
                                ElseIf objFrameInfo.IsDataFrame(intFrameType) Then ' check to see if a data frame
                                    ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK)
                                End If
                            End If
                        End If
                    Else
                        If MCB.ProtocolMode = "FEC" Then
                            If objFrameInfo.IsDataFrame(intFrameType) Then ' check to see if a data frame
                                ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK)
                            ElseIf intFrameType = &H30 Then ' If ID frame
                                AddTagToDataAndSendToHost(bytData, "ERR")
                            End If
                        ElseIf MCB.ProtocolMode = "ARQ" Then
                            If GetARDOPProtocolState = ProtocolState.DISC Then
                                If objFrameInfo.IsDataFrame(intFrameType) Then ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK)
                                If intFrameType = &H30 Then AddTagToDataAndSendToHost(bytData, "ERR")
                            End If
                            If Not blnTimeoutTriggered Then ProcessRcvdARQFrame(intFrameType, bytData, blnFrameDecodedOK)
                        End If
                    End If
                    If MCB.ProtocolMode = "FEC" And GetARDOPProtocolState <> ProtocolState.FECSend Then
                        SetARDOPProtocolState(ProtocolState.DISC) : InitializeConnection()
                    End If
                    State = ReceiveState.SearchingForLeader
                    objMain.objDemod.ClearAllMixedSamples()
                    DiscardOldSamples()
                    Exit Sub
                End If
            End If
        Catch ex As Exception
            Logs.Exception("[ARDOPprotocol.ProcessNewSamples]  Err: " & ex.ToString)
        End Try
    End Sub 'ProcessNewSamples
*/
}

void ProcessCapturedData(short * Samples, int nSamples)
{
  //      Dim bytCaptureData(-1) As Byte
	//int intReadPos;
	//int intCapturePos;

  //      Dim stcStatus As Status = Nothing
	static int intRcvPeak = 0;
	static int intGraphicsCtr = 0;
	static int intRecoveryCnt;
	static int intPkRcvLevelCnt = 0;


	if (nSamples < 2)
		return;

 //intRcvPeak = Math.Max(intRcvPeak, Math.Abs(intSample))
      
	if (!Capturing)
		return;

	ProcessNewSamples(Samples, nSamples);


  //          intNextCaptureOffset += bytCaptureData.Length
   //         intNextCaptureOffset = intNextCaptureOffset Mod intCaptureBufferSize ' Circular buffer
}

// Subroutine to compute Goertzel algorithm and return Real and Imag components for a single frequency bin

GoertzelRealImag(short intRealIn[], int intPtr, int N, double m, double * dblReal, double * dblImag)
{
	// intRealIn is a buffer at least intPtr + N in length
	// N need not be a power of 2
	// m need not be an integer
	// Computes the Real and Imaginary Freq values for bin m
	// Verified to = FFT results for at least 10 significant digits
	// Timings for 1024 Point on Laptop (64 bit Core Duo 2.2 Ghz)
	//        GoertzelRealImag .015 ms   Normal FFT (.5 ms)
	//  assuming Goertzel is proportional to N and FFT time proportional to Nlog2N
	//  FFT:Goertzel time  ratio ~ 3.3 Log2(N)

	//  Sanity check

	//if (intPtr < 0 Or (intRealIn.Length - intPtr) < N Then
    //        dblReal = 0 : dblImag = 0 : Exit Sub
     //   End If

	double dblZ_1 = 0.0, dblZ_2, dblW;
	double dblCoeff = 2 * cos(2 * M_PI * m / N);
	int i;

	for (i = 0; i <= N; i++)
	{
		if (i == N)
			dblW = dblZ_1 * dblCoeff - dblZ_2;
		else
			dblW = intRealIn[intPtr] + dblZ_1 * dblCoeff - dblZ_2;

		dblZ_2 = dblZ_1;
		dblZ_1 = dblW;
		intPtr += 1;
	}
	*dblReal = 2 * (dblW - cos(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2
	*dblImag = 2 * (sin(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2   (this sign agrees with Scope DSP phase values) 
}

// Function to interpolate spectrum peak using Quinn algorithm

double QuinnSpectralPeakLocator(double XkM1Re, double XkM1Im, double XkRe, double XkIm, double XkP1Re, double XkP1Im)
{
	// based on the Quinn algorithm in Streamlining Digital Processing page 139
	// Alpha1 = Re(Xk-1/Xk)
	// Alpha2 = Re(Xk+1/Xk)
	//Delta1 = Alpha1/(1 - Alpha1)
	//'Delta2 = Alpha2/(1 - Alpha2)
	// if Delta1 > 0 and Delta2 > 0 then Delta = Delta2 else Delta = Delta1
	// should be within .1 bin for S:N > 2 dB

	double dblDenom = pow(XkRe, 2) + pow(XkIm, 2);
	double dblAlpha1;
	double dblAlpha2;
	double dblDelta1;
	double dblDelta2;

	dblAlpha1 = ((XkM1Re * XkRe) + (XkM1Im * XkIm)) / dblDenom;
	dblAlpha2 = ((XkP1Re * XkRe) + (XkP1Im * XkIm)) / dblDenom;
	dblDelta1 = dblAlpha1 / (1 - dblAlpha1);
	dblDelta2 = dblAlpha2 / (1 - dblAlpha2);

	if (dblDelta1 > 0 &&  dblDelta2 > 0)
		return dblDelta2;
	else
		return dblDelta1;
}



// Function to detect and tune the 2 tone leader (for all bandwidths) 

BOOL SearchFor2ToneLeader(short * intNewSamples, int Ptr, int Length, double * dblOffsetHz)
{
	//' Status July 6, 2015 Good performance down to MPP_5dB. Operation over 4 search ranges confirmed (50, 100, 150, 200 Hz) 
	//       Optimized for July 6, 2015
	// search through the samples looking for the telltail 2 tone pattern (nominal tones 1450, 1550 Hz)
	// Find the offset in Hz (due to missmatch in transmitter - receiver tuning
	//'  Finds the power ratio of the tones 1450 and 1550 ratioed to 1350, 1400, 1500, 1600, and 1650

	double dblGoertzelReal[57];
	double dblGoertzelImag[57];
	double dblMag[57];
	double dblPower;
	double dblMaxPeak = 0.0, dblMaxPeakSN, dblInterpM, dblBinAdj, dblBinAdj1450, dblBinAdj1550;
	int intInterpCnt = 0;  // the count 0 to 3 of the interpolations that were < +/- .5 bin
	int  intIatMaxPeak = 0;
	double dblAlpha = 0.3;  // Works well possibly some room for optimization Changed from .5 to .3 on Rev 0.1.5.3
	double dblInterpretThreshold= 1.5; // Good results June 6, 2014 (was .4)  ' Works well possibly some room for optimization
	double dblFilteredMaxPeak = 0;
	int intStartBin, intStopBin;
	double dblLeftCar, dblRightCar, dblBinInterpLeft, dblBinInterpRight, dblCtrR, dblCtrI, dblCtrP, dblLeftP, dblRightP;
	double dblLeftR[2], dblLeftI[2], dblRightR[2], dblRightI[2];
	time_t Now = time(NULL);
	int i;

	if ((Length - Ptr) < 960)
		return FALSE;		// insure there are at least 960 samples (8 symbols of 120 samples)

	// Compute the start and stop bins based on the tuning range Each bin is 12000/960 or 12.5 Hz/bin
 
	if (Connected && (Now - dttLastGoodFrameTypeDecod < 15) || TuningRange == 0)
	{
		dblLeftCar = 120 - 4 + *dblOffsetHz / 12.5;  // the nominal positions of the two tone carriers based on the last computerd dblOffsetHz
		dblRightCar = 120 + 4 + *dblOffsetHz / 12.5;
		// Calculate 4 bins total for Noise value in S/N computation
		
		GoertzelRealImag(intNewSamples, Ptr, 960, 121 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // nominal center + 12.5 Hz
		dblCtrP = pow(dblCtrR, 2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 119 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); //  center - 12.5 Hz
		dblCtrP += pow(dblCtrR, 2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 127 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // Center + 87.5 Hz
		dblCtrP += pow(dblCtrR,2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 113 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // center - 87.5 Hz
		dblCtrP += pow(dblCtrR,2) + pow(dblCtrI, 2);

		// Calculate one bin above and below the two nominal 2 tone positions for Quinn Spectral Peak locator
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar - 1, &dblLeftR[0], &dblLeftI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar, &dblLeftR[1], &dblLeftI[1]);
		dblLeftP = pow(dblLeftR[1], 2) + pow(dblLeftI[1],  2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar + 1, &dblLeftR[2], &dblLeftI[2]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar - 1, &dblRightR[0], &dblRightI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar, &dblRightR[1], &dblRightI[1]);
		dblRightP = pow(dblRightR[1], 2) + pow(dblRightI[1], 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar + 1, &dblRightR[2], &dblRightI[2]);
		// Calculate the total power in the two tones (use product vs sum to help reject a single carrier).
		dblPower = sqrt(dblLeftP * dblRightP); // sqrt converts back to units of power 
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * log(dblPower / (0.25 * dblCtrP)); // Power S:N
                
		if (dblSNdBPwr > 22 && dblSNdBPwr_1 > 19)
		{
			// Calculate the interpolation based on the left of the two tones

			dblBinInterpLeft = QuinnSpectralPeakLocator(dblLeftR[0], dblLeftI[0], dblLeftR[1], dblLeftI[1], dblLeftR[2], dblLeftI[2]);
			// And the right of the two tones
			dblBinInterpRight = QuinnSpectralPeakLocator(dblRightR[0], dblRightI[0], dblRightR[1], dblRightI[1], dblRightR[2], dblRightI[2]); 

			if (abs(dblBinInterpLeft + dblBinInterpRight) < 1.2)
			{
				// sanity check for the interpolators

				if (dblBinInterpLeft + dblBinInterpLeft > 0)
					*dblOffsetHz = *dblOffsetHz + min((dblBinInterpLeft + dblBinInterpRight) * 6.25, 3);  // average left and right, adjustment bounded to +/- 3Hz max
				else
					*dblOffsetHz = *dblOffsetHz + max((dblBinInterpLeft + dblBinInterpRight) * 6.25, -3);
                    
				//strDecodeCapture = "Interp Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: ";
				dttStartRmtLeaderMeasure = Now;
				blnLeaderDetected = TRUE;

				//if (AccumulateStats)
				//{
				//	With stcTuningStats
                //     .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
                //     .intLeaderDetects += 1
                 //           End With
				//}
				return TRUE;
			}
		}
		Ptr += 480;
		blnLeaderDetected = FALSE;
		return FALSE;
	}
	else
	{
		// this is the full search over the full tuning range selected.  Uses more CPU time and with possibly larger deviation once connected. 
		
		intStartBin = ((200 - TuningRange) / 12.5);
		//intStopBin = ((dblMag.Length - 1) - ((200 - TuningRange) / 12.5));
		intStopBin = ((57 - 1) - ((200 - TuningRange) / 12.5));
		// Generate the Power magnitudes for up to 46 12.5 Hz bins (a function of MCB.TuningRange) 
      
		for (i = intStartBin; i <= intStopBin; i++)
		{
			GoertzelRealImag(intNewSamples, Ptr, 960, i + 92, &dblGoertzelReal[i], &dblGoertzelImag[i]);
			dblMag[i] = pow(dblGoertzelReal[i], 2) + pow(dblGoertzelImag[i], 2); // dblMag(i) in units of power (V^2)
		}
		
		//Search for the bins for the max power in the two tone signal.  

		for (i = intStartBin ; i <= intStopBin - 24; i++)	// ' +/- MCB.TuningRange from nominal 
		{
			dblPower = sqrt(dblMag[i + 8] * dblMag[i + 16]); // using the product to minimize sensitivity to one strong carrier vs the two tone
			// sqrt coonverts back to units of power from Power ^2
			if (dblPower > dblMaxPeak)
			{
				dblMaxPeak = dblPower;
				intIatMaxPeak = i + 104;
			}
		}

		// Now compute the max peak:Noise (power)  using the product of the 2 tone bins @ 1450 and 1550 Hz (nominal...spaced at 100 Hz)
		// Divided by the Noise power at nominal bins 1350, 1400, 1500, 1600, 1650 Hz
		// Denominator uses average of 5 "noise" bins to reduce variation. Factor of .2 adjusts for the #Bins in the  Denom 
		// Note sum vs product appeared more consistant with multipath poor. 

		dblMaxPeakSN = dblMaxPeak / (0.2 * (dblMag[intIatMaxPeak - 104] + dblMag[intIatMaxPeak - 100] +
               dblMag[intIatMaxPeak - 92] + dblMag[intIatMaxPeak - 84] + dblMag[intIatMaxPeak - 80]));
               // 'dblMaxPeakSN = dblMaxPeak / (0.3333 * (dblMag(intIatMaxPeak - 100) + dblMag(intIatMaxPeak - 92) + dblMag(intIatMaxPeak - 84)))
		dblSNdBPwr_2 = dblSNdBPwr_1;
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * log(dblMaxPeakSN);
		if (dblSNdBPwr > 29 && dblSNdBPwr_1 > 24) // These values selected during optimizatin tests 7/5/2015 @ mpp -5 dB S:N
		{
			// Do the interpolation based on the two carriers at nominal 1450 and 1550Hz
    
			if ((intIatMaxPeak - 97) >= intStartBin && (intIatMaxPeak - 87) <= intStopBin) // check to insure no index errors
			{
				// Interpolate the adjacent bins using QuinnSpectralPeakLocator
					
					dblBinAdj1450 = QuinnSpectralPeakLocator(dblGoertzelReal[intIatMaxPeak - 97], dblGoertzelImag[intIatMaxPeak - 97],
							dblGoertzelReal[intIatMaxPeak - 96], dblGoertzelImag[intIatMaxPeak - 96],
							dblGoertzelReal[intIatMaxPeak - 95], dblGoertzelImag[intIatMaxPeak - 95]);
				if (dblBinAdj1450 < dblInterpretThreshold && dblBinAdj1450 > -dblInterpretThreshold)
				{
					dblBinAdj = dblBinAdj1450;
					intInterpCnt += 1;
				}

				dblBinAdj1550 = QuinnSpectralPeakLocator(dblGoertzelReal[intIatMaxPeak - 89], dblGoertzelImag[intIatMaxPeak - 89], 
						dblGoertzelReal[intIatMaxPeak - 88], dblGoertzelImag[intIatMaxPeak - 88], 
						dblGoertzelReal[intIatMaxPeak - 87], dblGoertzelImag[intIatMaxPeak - 87]);

				if (dblBinAdj1550 < dblInterpretThreshold && dblBinAdj1550 > -dblInterpretThreshold)
				{
					dblBinAdj += dblBinAdj1550;
					intInterpCnt += 1;
				}
				if (intInterpCnt == 0)
				{
					Ptr += 480;
					blnLeaderDetected = FALSE;
					return FALSE;
				}
				else
				{
					dblBinAdj = dblBinAdj / intInterpCnt; // average the offsets that are within .5 bin
					dblInterpM = intIatMaxPeak + dblBinAdj;
				}
			//	else
				{
					Ptr += 480; // ' ..reduces CPU loading
					return FALSE;
				}
				// update the offsetHz and setup the NCO new freq and Phase inc. Note no change to current NCOphase

				*dblOffsetHz = 12.5 * (dblInterpM - 120); // compute the tuning offset in Hz using dblInterpM
		}
		else
		{
			Ptr += 480;  //  Ptr += 240 ' evaluate if this is OK ..reduces CPU loading
			blnLeaderDetected = FALSE;
			return FALSE;
		}
     
		dblNCOFreq = 3000 + *dblOffsetHz;  //Set the NCO frequency and phase inc for mixing 
		dblNCOPhaseInc = 2 * M_PI * dblNCOFreq / 12000;
		Ptr = Ptr + 480; // advance 4 symbols to avoid any noise in start ' optimize?
		State = AcquireSymbolSync;
		blnLeaderDetected = TRUE;
        //        If MCB.AccumulateStats Then
         //           With stcTuningStats
        //                .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
         //               .intLeaderDetects += 1
         //           End With
         //       End If

		//strDecodeCapture = "Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: ";
		dttStartRmtLeaderMeasure = Now;
		return TRUE;
		}
	}
	return FALSE;
}


