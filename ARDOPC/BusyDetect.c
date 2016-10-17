
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#endif

#include "ARDOPC.h"

VOID SortSignals(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin);

int intLastStart, intLastStop;

int LastBusyOn;
int LastBusyOff;

BOOL blnLastBusy = FALSE;

float dblAvgStoNSlowNarrow;
float dblAvgStoNFastNarrow;
float dblAvgStoNSlowWide;
float dblAvgStoNFastWide;
int intLastStart = 0;
int intLastStop  = 0;
int intBusyOnCnt  = 0;  // used to filter Busy ON detections 
int intBusyOffCnt  = 0; // used to filter Busy OFF detections 


// Function to implement a busy detector based on 1024 point FFT
BOOL BusyDetect2(float * dblMag, int intStart, int intStop)        // this only called while searching for leader ...once leader detected, no longer called.
{
	// each bin is about 12000/1024 or 11.72 Hz
	// this only called while searching for leader ...once leader detected, no longer called.
	// First sort signals and look at highes signals:baseline ratio..

	float dblAVGSignalPerBinNarrow, dblAVGSignalPerBinWide, dblAVGBaselineNarrow, dblAVGBaselineWide;
	float dblFastAlpha = 0.4f;
	float dblSlowAlpha = 0.2f;
	float dblAvgStoNNarrow, dblAvgStoNWide;
	int intNarrow = 8;  // 8 x 11.72 Hz about 94 z
	int intWide = ((intStop - intStart) * 2) / 3; //* 0.66);
	int blnBusy = FALSE;
	float dblAvgStoNSlowNarrow = 0;
	float dblAvgStoNFastNarrow = 0;
	float dblAvgStoNSlowWide = 0;
	float dblAvgStoNFastWide = 0;

	// First narrow band (~94Hz)

	SortSignals(dblMag, intStart, intStop, intNarrow, &dblAVGSignalPerBinNarrow, &dblAVGBaselineNarrow);

	if (intLastStart == intStart && intLastStop == intStop)
	{
		dblAvgStoNSlowNarrow = (1 - dblSlowAlpha) * dblAvgStoNSlowNarrow + dblSlowAlpha * dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
		dblAvgStoNFastNarrow = (1 - dblFastAlpha) * dblAvgStoNFastNarrow + dblFastAlpha * dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
	}
	else
	{
		dblAvgStoNSlowNarrow = dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
		dblAvgStoNFastNarrow = dblAVGSignalPerBinNarrow / dblAVGBaselineNarrow;
		intLastStart = intStart;
		intLastStop = intStop;
	}
	
	dblAvgStoNNarrow = max(dblAvgStoNSlowNarrow, dblAvgStoNFastNarrow); // computes fast attack, slow release

	// Wide band (66% ofr current bandwidth) 

	SortSignals(dblMag, intStart, intStop, intWide, &dblAVGSignalPerBinWide, &dblAVGBaselineWide);

	if (intLastStart == intStart && intLastStop == intStop)
	{
		dblAvgStoNSlowWide = (1 - dblSlowAlpha) * dblAvgStoNSlowWide + dblSlowAlpha * dblAVGSignalPerBinWide / dblAVGBaselineWide;
		dblAvgStoNFastWide = (1 - dblFastAlpha) * dblAvgStoNFastWide + dblFastAlpha * dblAVGSignalPerBinWide / dblAVGBaselineWide;
	}
	else
	{
		dblAvgStoNSlowWide = dblAVGSignalPerBinWide / dblAVGBaselineWide;
		dblAvgStoNFastWide = dblAVGSignalPerBinWide / dblAVGBaselineWide;
		intLastStart = intStart;
		intLastStop = intStop;
	}

	dblAvgStoNNarrow = max(dblAvgStoNSlowNarrow, dblAvgStoNFastNarrow); // computes fast attack, slow release
	dblAvgStoNWide = max(dblAvgStoNSlowWide, dblAvgStoNFastWide); // computes fast attack, slow release

	// Preliminary calibration...future a function of bandwidth and BusyDet.
   
	switch (ARQBandwidth)
	{
	case B200MAX:
	case B200FORCED:
		if (dblAvgStoNNarrow > 1.5 * BusyDet|| dblAvgStoNWide > 2.5 * BusyDet)
			blnBusy = True;
		break;

	case B500MAX:
	case B500FORCED:
		if (dblAvgStoNNarrow > 1.5 * BusyDet || dblAvgStoNWide > 2.5 * BusyDet)
			blnBusy = True;
		break;

	case B1000MAX:
	case B1000FORCED:

		if (dblAvgStoNNarrow > 1.4 * BusyDet || dblAvgStoNWide > 2 * BusyDet)
			blnBusy = True;
		break;

	case B2000MAX:
	case B2000FORCED:
		if (dblAvgStoNNarrow > 1.4 * BusyDet || dblAvgStoNWide > 2 * BusyDet)
			blnBusy = True;
	}

	if (blnBusy) // This used to skip over one call busy nuisance trips. Busy must be present at least 2 consecutive times to be reported
	{
		intBusyOnCnt += 1;
		intBusyOffCnt = 0;
        if (intBusyOnCnt > 1)
			blnBusy = True;
		else if (!blnBusy)
		{
			intBusyOffCnt += 1;
			intBusyOnCnt = 0;
			if (intBusyOffCnt > 3)
				blnBusy = False;
		}
	}
	if (blnLastBusy == False && blnBusy)
	{
		int x = round(dblAvgStoNNarrow);	// odd, but PI doesnt print floats properly 
		int y = round(dblAvgStoNWide);
		
		blnLastBusy = True;
		LastBusyOn = Now;
#ifdef __ARM_ARCH
		WriteDebugLog("[BusyDetect2: BUSY ON  StoN Narrow = %d StoN Wide %d", x, y);
#else
		WriteDebugLog("[BusyDetect2: BUSY ON  StoN Narrow = %f StoN Wide %f", dblAvgStoNNarrow, dblAvgStoNWide);
#endif
	}
	else if (blnLastBusy == True && !blnBusy)
	{
		int x = round(dblAvgStoNNarrow);	// odd, but PI doesnt print floats properly 
		int y = round(dblAvgStoNWide);

		blnLastBusy = False;
		LastBusyOff = Now;
#ifdef __ARM_ARCH
		WriteDebugLog("[BusyDetect2: BUSY OFF StoN Narrow = %d StoN Wide %d", x, y);
#else
		WriteDebugLog("[BusyDetect2: BUSY OFF StoN Narrow = %f StoN Wide %f", dblAvgStoNNarrow, dblAvgStoNWide);
#endif
	}

	return blnLastBusy;
}

VOID SortSignals(float * dblMag, int intStartBin, int intStopBin, int intNumBins, float *  dblAVGSignalPerBin, float *  dblAVGBaselinePerBin)
{
	// puts the top intNumber of bins between intStartBin and intStopBin into dblAVGSignalPerBin, the rest into dblAvgBaselinePerBin
    // for decent accuracy intNumBins should be < 75% of intStopBin-intStartBin)

	float dblAVGSignal[200] = {0};//intNumBins
	float dblAVGBaseline[200] = {0};//intStopBin - intStartBin - intNumBins

	float dblSigSum = 0;
	float dblTotalSum = 0;
	int intSigPtr = 0;
	int intBasePtr = 0;
	int i, j, k;

	for (i = 0; i <  intNumBins; i++)
	{
		for (j = intStartBin; j <= intStopBin; j++)
		{
			if (i == 0)
			{
				dblTotalSum += dblMag[j];
				if (dblMag[j] > dblAVGSignal[i])
					dblAVGSignal[i] = dblMag[j];
			}
			else
			{
				if (dblMag[j] > dblAVGSignal[i] && dblMag[j] < dblAVGSignal[i - 1])
					dblAVGSignal[i] = dblMag[j];
			}
		}
	}
	
	for(k = 0; k < intNumBins; k++)
	{
		dblSigSum += dblAVGSignal[k];
	}
	*dblAVGSignalPerBin = dblSigSum / intNumBins;
	*dblAVGBaselinePerBin = (dblTotalSum - dblSigSum) / (intStopBin - intStartBin - intNumBins + 1);
}
