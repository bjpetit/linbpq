Imports System.Math
Imports System.IO
Public Class DemodulateDecode12K

    ' integers
    Public intMFSReadPtr As Int32 ' pointer to the current next to use sample of intMixedFilteredSamples
    Public intPriorTone As Int32 = 0 ' the prior tone since this is incremental FSK
    Public intLeaderRcvdMs As Int32 = 1000
    Public intRmtLeaderMeasure As Int32 = 0
    Public intLastRcvdFrameQuality As Int32
    Public blnLeaderDetected As Boolean = False ' Setup upon a leader detect....cleared after frame decode or failure to detect leader
    Private intPSKRefPhase As Int16 ' PSK Reference symbol in milliradians) (+/- 3142)

    ' Bytes
    Private bytLastDataFrameType As Byte = 0

    ' objects
    Private objRS8 As New ReedSoloman8
    Private objMain As Main
    Private objFrameInfo As New FrameInfo

    ' doubles
    Private dblFilEnvelopeOut() As Double
    Private dblFilEnvelopeIn() As Double
    Private dblNCOFreq As Double = 3000 ' nominal NC) frequency
    Private dblNCOPhase As Double = 0
    Private dblSNdBPwr, dblSNdBPwr_1, dblSNdBPwr_2 As Double
    Private dblNCOPhaseInc As Double = 2 * PI * dblNCOFreq / 12000
    Private dbl2Pi As Double = 2 * PI

    ' Date/Time
    Private dttStartRmtLeaderMeasure As Date
    Private dttLastGoodFrameTypeDecode As Date = Now.AddHours(-1)

    ' arrays
    Public intFilteredMixedSamples(-1) As Int32
    Public intMixedSamples(-1) As Int32
    Private intPriorMixedSamples(119) As Int32 ' a buffer of 120 samples to hold the prior samples used in the filter

    ' For testing a 2nd order Eliptic Biquad bandpass 1500 center, 200 Hz width, 1dB ripple, 30 db Stopband Attn (from Scope IIR) 
    Private dblBiQuad1() As Double = {1, -1.30689276639, 0.942654543778, 1, -0.888659442255, 1}
    Private dblBiQuad2() As Double = {1, -1.44171395977332, 0.9477488287543, 1, -1.71774001216554, 1}

    ' for plotting used for debugging for waveform capture and analysis
    Private dblAcquire2ToneLeaderSymbolFraming() As Double
    Private dblAcquirePSKFrameSyncRSB() As Double
    Private dblAcquire4FSKFrameType(240 * 8 - 1) As Double

    ' Subroutine to clear all mixed samples 
    Public Sub ClearAllMixedSamples()
        ReDim intFilteredMixedSamples(-1)
        ReDim intMixedSamples(-1)
        ReDim intPriorMixedSamples(intPriorMixedSamples.Length - 1)
        intMFSReadPtr = 0
    End Sub 'ClearAllMixedSamples

    ' Subroutine to reset the SNPower from SearchForTwoToneLeader
    Public Sub ResetSNPwrs()
        dblSNdBPwr = 0
        dblSNdBPwr_1 = 0
        dblSNdBPwr_2 = 0
    End Sub  '  ResetSNPwrs

    ' Subroutine to compute Goertzel algorithm and return Real and Imag components for a single frequency bin
    Public Sub GoertzelRealImag(ByRef intRealIn() As Int32, ByVal intPtr As Integer, ByVal N As Integer, ByVal m As Double, ByRef dblReal As Double, ByRef dblImag As Double)
        ' intRealIn is a buffer at least intPtr + N in length
        ' N need not be a power of 2
        ' m need not be an integer
        ' Computes the Real and Imaginary Freq values for bin m
        ' Verified to = FFT results for at least 10 significant digits
        ' Timings for 1024 Point on Laptop (64 bit Core Duo 2.2 Ghz)
        '       GoertzelRealImag .015 ms   Normal FFT (.5 ms)
        ' assuming Goertzel is proportional to N and FFT time proportional to Nlog2N
        ' FFT:Goertzel time  ratio ~ 3.3 Log2(N)

        ' Sanity check
        If intPtr < 0 Or (intRealIn.Length - intPtr) < N Then
            dblReal = 0 : dblImag = 0 : Exit Sub
        End If
        Dim dblZ_1, dblZ_2, dblW As Double
        Dim dblCoeff As Double = 2 * Cos(dbl2Pi * m / N)
        For i As Integer = 0 To N
            If i = N Then
                dblW = dblZ_1 * dblCoeff - dblZ_2
            Else
                dblW = intRealIn(intPtr) + dblZ_1 * dblCoeff - dblZ_2
            End If
            dblZ_2 = dblZ_1 : dblZ_1 = dblW : intPtr += 1
        Next i
        dblReal = 2 * (dblW - Cos(dbl2Pi * m / N) * dblZ_2) / N ' scale results by N/2
        dblImag = 2 * (Sin(dbl2Pi * m / N) * dblZ_2) / N  ' scale results by N/2   (this sign agrees with Scope DSP phase values) 
    End Sub ' GoertzelRealImag

    ' Function to detect and tune the 2 tone leader (for all bandwidths) 
    Public Function SearchFor2ToneLeader(ByRef intNewSamples() As Int32, ByRef intPtr As Int32, ByRef dblOffsetHz As Double) As Boolean
        ' Status July 6, 2015 Good performance down to MPP_5dB. Operation over 4 search ranges confirmed (50, 100, 150, 200 Hz) 
        '       Optimized for July 6, 2015
        ' search through the samples looking for the telltail 2 tone pattern (nominal tones 1450, 1550 Hz)
        ' Find the offset in Hz (due to missmatch in transmitter - receiver tuning
        '  Finds the power ratio of the tones 1450 and 1550 ratioed to 1350, 1400, 1500, 1600, and 1650
        Dim dblGoertzelReal(56) As Double
        Dim dblGoertzelImag(56) As Double
        Dim dblMag(56) As Double
        Dim dblPower As Double
        Dim dblMaxPeak, dblMaxPeakSN, dblInterpM, dblBinAdj, dblBinAdj1450, dblBinAdj1550 As Double
        Dim intInterpCnt As Int32 = 0 ' the count 0 to 3 of the interpolations that were < +/- .5 bin
        Dim intIatMaxPeak = 0
        Dim dblAlpha As Double = 0.3 ' Works well possibly some room for optimization Changed from .5 to .3 on Rev 0.1.5.3
        Dim dblInterpretThreshold As Double = 1.5 ' Good results June 6, 2014 (was .4)  ' Works well possibly some room for optimization
        Dim dblFilteredMaxPeak As Double = 0
        Dim intStartBin, intStopBin As Int32
        Dim dblLeftCar, dblRightCar, dblBinInterpLeft, dblBinInterpRight, dblCtrR, dblCtrI, dblCtrP, dblLeftP, dblRightP As Double
        Dim dblLeftR(2), dblLeftI(2), dblRightR(2), dblRightI(2) As Double

        Try
            If intNewSamples.Length - intPtr < 960 Then Return False ' insure there are at least 960 samples (8 symbols of 120 samples)
            ' Compute the start and stop bins based on the tuning range Each bin is 12000/960 or 12.5 Hz/bin
            If (objMain.objProtocol.Connected And (Now.Subtract(dttLastGoodFrameTypeDecode).TotalSeconds < 15)) Or MCB.TuningRange = 0 Then
                dblLeftCar = 120 - 4 + dblOffsetHz / 12.5 ' the nominal positions of the two tone carriers based on the last computerd dblOffsetHz
                dblRightCar = 120 + 4 + dblOffsetHz / 12.5
                ' Calculate 4 bins total for Noise value in S/N computation
                GoertzelRealImag(intNewSamples, intPtr, 960, 121 + dblOffsetHz / 12.5, dblCtrR, dblCtrI) ' nominal center + 12.5 Hz
                dblCtrP = dblCtrR ^ 2 + dblCtrI ^ 2
                GoertzelRealImag(intNewSamples, intPtr, 960, 119 + dblOffsetHz / 12.5, dblCtrR, dblCtrI) ' center - 12.5 Hz
                dblCtrP += dblCtrR ^ 2 + dblCtrI ^ 2
                GoertzelRealImag(intNewSamples, intPtr, 960, 127 + dblOffsetHz / 12.5, dblCtrR, dblCtrI) ' Center + 87.5 Hz
                dblCtrP += dblCtrR ^ 2 + dblCtrI ^ 2
                GoertzelRealImag(intNewSamples, intPtr, 960, 113 + dblOffsetHz / 12.5, dblCtrR, dblCtrI) ' center - 87.5 Hz
                dblCtrP += dblCtrR ^ 2 + dblCtrI ^ 2

                ' Calculate one bin above and below the two nominal 2 tone positions for Quinn Spectral Peak locator
                GoertzelRealImag(intNewSamples, intPtr, 960, dblLeftCar - 1, dblLeftR(0), dblLeftI(0))
                GoertzelRealImag(intNewSamples, intPtr, 960, dblLeftCar, dblLeftR(1), dblLeftI(1))
                dblLeftP = dblLeftR(1) ^ 2 + dblLeftI(1) ^ 2
                GoertzelRealImag(intNewSamples, intPtr, 960, dblLeftCar + 1, dblLeftR(2), dblLeftI(2))
                GoertzelRealImag(intNewSamples, intPtr, 960, dblRightCar - 1, dblRightR(0), dblRightI(0))
                GoertzelRealImag(intNewSamples, intPtr, 960, dblRightCar, dblRightR(1), dblRightI(1))
                dblRightP = dblRightR(1) ^ 2 + dblRightI(1) ^ 2
                GoertzelRealImag(intNewSamples, intPtr, 960, dblRightCar + 1, dblRightR(2), dblRightI(2))
                ' Calculate the total power in the two tones (use product vs sum to help reject a single carrier).
                dblPower = Sqrt(dblLeftP * dblRightP) ' sqrt converts back to units of power 
                dblSNdBPwr_1 = dblSNdBPwr
                dblSNdBPwr = 10 * Log(dblPower / (0.25 * dblCtrP)) ' Power S:N
                If dblSNdBPwr > 22 And dblSNdBPwr_1 > 19 Then
                    ' Calculate the interpolation based on the left of the two tones
                    dblBinInterpLeft = QuinnSpectralPeakLocator(dblLeftR(0), dblLeftI(0), dblLeftR(1), dblLeftI(1), dblLeftR(2), dblLeftI(2))
                    ' And the right of the two tones
                    dblBinInterpRight = QuinnSpectralPeakLocator(dblRightR(0), dblRightI(0), dblRightR(1), dblRightI(1), dblRightR(2), dblRightI(2))
                    If Abs(dblBinInterpLeft + dblBinInterpRight) < 1.2 Then ' sanity check for the interpolators
                        If dblBinInterpLeft + dblBinInterpLeft > 0 Then
                            dblOffsetHz = dblOffsetHz + Min((dblBinInterpLeft + dblBinInterpRight) * 6.25, 3) ' average left and right, adjustment bounded to +/- 3Hz max
                        Else
                            dblOffsetHz = dblOffsetHz + Max((dblBinInterpLeft + dblBinInterpRight) * 6.25, -3)
                        End If
                        strDecodeCapture = "Interp Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: "
                        dttStartRmtLeaderMeasure = Now
                        blnLeaderDetected = True
                        If MCB.AccumulateStats Then
                            With stcTuningStats
                                .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
                                .intLeaderDetects += 1
                            End With
                        End If
                        Return True
                    Else
                        intPtr += 480
                        blnLeaderDetected = False
                        Return False
                    End If
                Else
                    intPtr += 480
                    blnLeaderDetected = False
                    Return False
                End If
            Else ' this is the full search over the full tuning range selected.  Uses more CPU time and with possibly larger deviation once connected. 
                intStartBin = CInt((200 - MCB.TuningRange) / 12.5)
                intStopBin = CInt((dblMag.Length - 1) - ((200 - MCB.TuningRange) / 12.5))
                ' Generate the Power magnitudes for up to 46 12.5 Hz bins (a function of MCB.TuningRange) 
                For i As Integer = intStartBin To intStopBin
                    GoertzelRealImag(intNewSamples, intPtr, 960, i + 92, dblGoertzelReal(i), dblGoertzelImag(i))
                    dblMag(i) = (dblGoertzelReal(i) ^ 2) + (dblGoertzelImag(i) ^ 2) ' dblMag(i) in units of power (V^2)
                Next
                ' Search for the bins for the max power in the two tone signal.  
                For i As Integer = intStartBin To intStopBin - 24 ' +/- MCB.TuningRange from nominal 
                    dblPower = Sqrt(dblMag(i + 8) * dblMag(i + 16)) ' using the product to minimize sensitivity to one strong carrier vs the two tone
                    ' sqrt coonverts back to units of power from Power ^2
                    If dblPower > dblMaxPeak Then
                        dblMaxPeak = dblPower
                        intIatMaxPeak = i + 104
                    End If
                Next i
                ' Now compute the max peak:Noise (power)  using the product of the 2 tone bins @ 1450 and 1550 Hz (nominal...spaced at 100 Hz)
                ' Divided by the Noise power at nominal bins 1350, 1400, 1500, 1600, 1650 Hz
                ' Denominator uses average of 5 "noise" bins to reduce variation. Factor of .2 adjusts for the #Bins in the  Denom 
                ' Note sum vs product appeared more consistant with multipath poor. 

                dblMaxPeakSN = dblMaxPeak / (0.2 * (dblMag(intIatMaxPeak - 104) + dblMag(intIatMaxPeak - 100) + _
                dblMag(intIatMaxPeak - 92) + dblMag(intIatMaxPeak - 84) + dblMag(intIatMaxPeak - 80)))
                'dblMaxPeakSN = dblMaxPeak / (0.3333 * (dblMag(intIatMaxPeak - 100) + dblMag(intIatMaxPeak - 92) + dblMag(intIatMaxPeak - 84)))
                dblSNdBPwr_2 = dblSNdBPwr_1
                dblSNdBPwr_1 = dblSNdBPwr
                dblSNdBPwr = 10 * Log(dblMaxPeakSN)
                If dblSNdBPwr > 29 And dblSNdBPwr_1 > 24 Then ' These values selected during optimizatin tests 7/5/2015 @ mpp -5 dB S:N
                    'Do the interpolation based on the two carriers at nominal 1450 and 1550Hz
                    If (intIatMaxPeak - 97) >= intStartBin And (intIatMaxPeak - 87) <= intStopBin Then ' check to insure no index errors
                        ' Interpolate the adjacent bins using QuinnSpectralPeakLocator
                        dblBinAdj1450 = QuinnSpectralPeakLocator(dblGoertzelReal(intIatMaxPeak - 97), dblGoertzelImag(intIatMaxPeak - 97), _
                                                                dblGoertzelReal(intIatMaxPeak - 96), dblGoertzelImag(intIatMaxPeak - 96), _
                                                                dblGoertzelReal(intIatMaxPeak - 95), dblGoertzelImag(intIatMaxPeak - 95))
                        If dblBinAdj1450 < dblInterpretThreshold And dblBinAdj1450 > -dblInterpretThreshold Then
                            dblBinAdj = dblBinAdj1450
                            intInterpCnt += 1
                        End If
                        dblBinAdj1550 = QuinnSpectralPeakLocator(dblGoertzelReal(intIatMaxPeak - 89), dblGoertzelImag(intIatMaxPeak - 89), _
                                                                dblGoertzelReal(intIatMaxPeak - 88), dblGoertzelImag(intIatMaxPeak - 88), _
                                                                dblGoertzelReal(intIatMaxPeak - 87), dblGoertzelImag(intIatMaxPeak - 87))
                        If dblBinAdj1550 < dblInterpretThreshold And dblBinAdj1550 > -dblInterpretThreshold Then
                            dblBinAdj += dblBinAdj1550
                            intInterpCnt += 1
                        End If
                        If intInterpCnt = 0 Then
                            intPtr += 480
                            blnLeaderDetected = False
                            Return False
                        Else
                            dblBinAdj = dblBinAdj / intInterpCnt ' average the offsets that are within .5 bin
                            dblInterpM = intIatMaxPeak + dblBinAdj
                        End If
                    Else
                        intPtr += 480 ' ..reduces CPU loading
                        Return False
                    End If
                    ' update the offsetHz and setup the NCO new freq and Phase inc. Note no change to current NCOphase
                    dblOffsetHz = 12.5 * (dblInterpM - 120) ' compute the tuning offset in Hz using dblInterpM
                Else
                    intPtr += 480 '  intPtr += 240 ' evaluate if this is OK ..reduces CPU loading
                    blnLeaderDetected = False
                    Return False
                End If
                dblNCOFreq = 3000 + dblOffsetHz ' Set the NCO frequency and phase inc for mixing 
                dblNCOPhaseInc = dbl2Pi * dblNCOFreq / 12000
                intPtr = intPtr + 480 ' advance 4 symbols to avoid any noise in start ' optimize?
                State = ReceiveState.AcquireSymbolSync
                blnLeaderDetected = True
                If MCB.AccumulateStats Then
                    With stcTuningStats
                        .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
                        .intLeaderDetects += 1
                    End With
                End If
                strDecodeCapture = "Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: "
                dttStartRmtLeaderMeasure = Now
                Return True
            End If
        Catch Ex As Exception
            Logs.Exception("[DemodulateDecode12K.SearchFor2ToneLeader]  Err:" & Ex.ToString)
            intPtr = intNewSamples.Length ' This will force a discard of all received samples 
            Return False
        End Try
    End Function ' SearchFor2ToneLeader

    ' Function to interpolate spectrum peak using Quinn algorithm
    Public Function QuinnSpectralPeakLocator(ByVal XkM1Re As Double, ByVal XkM1Im As Double, ByVal XkRe As Double, ByVal XkIm As Double, ByVal XkP1Re As Double, ByVal XkP1Im As Double) As Double
        ' based on the Quinn algorithm in Streamlining Digital Processing page 139
        ' Alpha1 = Re(Xk-1/Xk)
        ' Alpha2 = Re(Xk+1/Xk)
        'Delta1 = Alpha1/(1 - Alpha1)
        'Delta2 = Alpha2/(1 - Alpha2)
        ' if Delta1 > 0 and Delta2 > 0 then Delta = Delta2 else Delta = Delta1
        ' should be within .1 bin for S:N > 2 dB
        Dim dblDenom As Double = XkRe ^ 2 + XkIm ^ 2
        Dim dblAlpha1 As Double
        Dim dblAlpha2 As Double
        dblAlpha1 = ((XkM1Re * XkRe) + (XkM1Im * XkIm)) / dblDenom
        dblAlpha2 = ((XkP1Re * XkRe) + (XkP1Im * XkIm)) / dblDenom
        Dim dblDelta1 As Double = dblAlpha1 / (1 - dblAlpha1)
        Dim dblDelta2 As Double = dblAlpha2 / (1 - dblAlpha2)
        If dblDelta1 > 0 And dblDelta2 > 0 Then
            Return dblDelta2
        Else
            Return dblDelta1
        End If
    End Function  ' QuinnSpectralPeakLocator

    ' Function to acquire the Frame Sync for all Frames 
    Public Function AcquireFrameSyncRSB() As Boolean
        ' Two improvements could be incorporated into this function:
        '   1) Provide symbol tracking until the frame sync is found (small corrections should be less than 1 sample per 4 symbols ~2000 ppm)
        '   2) Ability to more accurately locate the symbol center (could be handled by symbol tracking 1) above. 

        ' This is for acquiring FSKFrameSync After Mixing Tones Mirrored around 1500 Hz. e.g. Reversed Sideband
        ' Frequency offset should be near 0 (normally within +/- 1 Hz)  
        ' Locate the sync Symbol which has no phase change from the prior symbol (BPSK leader @ 1500 Hz)   

        Dim intLocalPtr As Int32 = intMFSReadPtr
        Dim intAvailableSymbols As Integer = (intFilteredMixedSamples.Length - intMFSReadPtr) \ 120
        Dim dblPhaseSym1 As Double ' phase of the first symbol 
        Dim dblPhaseSym2 As Double  ' phase of the second symbol 
        Dim dblPhaseSym3 As Double  ' phase of the third symbol

        Dim dblReal, dblImag As Double
        Dim dblPhaseDiff12, dblPhaseDiff23 As Double
        Try
            If intAvailableSymbols < 3 Then Return False ' must have at least 360 samples to search
            ' Calculate the Phase for the First symbol 
            GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, dblReal, dblImag) ' Carrier at 1500 Hz nominal Positioning with no cyclic prefix
            dblPhaseSym1 = Atan2(dblImag, dblReal)
            intLocalPtr += 120 ' advance one symbol
            GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, dblReal, dblImag) ' Carrier at 1500 Hz nominal Positioning with no cyclic prefix
            dblPhaseSym2 = Atan2(dblImag, dblReal)
            intLocalPtr += 120 ' advance one symbol
            For i As Integer = 0 To intAvailableSymbols - 4
                ' Compute the phase of the next symbol  
                GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, dblReal, dblImag) ' Carrier at 1500 Hz nominal Positioning with no cyclic prefix
                dblPhaseSym3 = Atan2(dblImag, dblReal)
                ' Compute the phase differences between sym1-sym2, sym2-sym3
                dblPhaseDiff12 = dblPhaseSym1 - dblPhaseSym2
                If dblPhaseDiff12 > PI Then ' bound phase diff to +/- Pi
                    dblPhaseDiff12 -= dbl2Pi
                ElseIf dblPhaseDiff12 < -PI Then
                    dblPhaseDiff12 += dbl2Pi
                End If
                dblPhaseDiff23 = dblPhaseSym2 - dblPhaseSym3
                If dblPhaseDiff23 > PI Then ' bound phase diff to +/- Pi
                    dblPhaseDiff23 -= dbl2Pi
                ElseIf dblPhaseDiff23 < -PI Then
                    dblPhaseDiff23 += dbl2Pi
                End If
                If (Abs(dblPhaseDiff12) > 0.6667 * PI) And (Abs(dblPhaseDiff23) < 0.3333 * PI) Then 'Tighten the margin to 60 degrees
                    intPSKRefPhase = CShort(dblPhaseSym3 * 1000)

                    ' *********************************
                    ' Debug code to look at filtered waveform
                    'Dim dblFilteredMixed(intFilteredMixedSamples.Length - intMFSReadPtr - 1) As Double
                    'ReDim dblAcquirePSKFrameSyncRSB(intFilteredMixedSamples.Length - intMFSReadPtr - 1)
                    'For k As Integer = 0 To dblAcquirePSKFrameSyncRSB.Length - 1
                    '    dblAcquirePSKFrameSyncRSB(k) = intFilteredMixedSamples(k + intMFSReadPtr)
                    'Next
                    'Dim objWT As New WaveTools
                    'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
                    '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
                    'End If
                    'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\AcquirePSKFrameSyncRSB.wav", 12000, 16, dblFilteredMixed)
                    ' End of debug code
                    '************************************

                    intLeaderRcvdMs = Round((intLocalPtr - 30) / 12) ' 30 is to accomodate offset of inital pointer for filter length. 
                    intMFSReadPtr = intLocalPtr + 120 ' Position read pointer to start of the symbol following reference symbol 
                    If MCB.AccumulateStats Then stcTuningStats.intFrameSyncs += 1 ' accumulate tuning stats
                    'strDecodeCapture &= "Sync; Phase1>2=" & Format(dblPhaseDiff12, "0.00") & " Phase2>3=" & Format(dblPhaseDiff23, "0.00") & ": "
                    Return True ' pointer is pointing to first 4FSK data symbol. (first symbol of frame type)
                Else
                    dblPhaseSym1 = dblPhaseSym2
                    dblPhaseSym2 = dblPhaseSym3
                    intLocalPtr += 120 ' advance one symbol 
                End If
            Next i
            intMFSReadPtr = intLocalPtr - 240 ' back up 2 symbols for next attempt (Current Sym2 will become new Sym1)
            Return False
        Catch ex As Exception
            Logs.Exception("[AcquireFrameSyncRSB] Err: " & ex.ToString)
        End Try
        Return False
    End Function 'AcquireFrameSyncRSB

    ' Function to look at the 2 tone leader and establishes the Symbol framing using envelope search and minimal phase error. 
    Public Function Acquire2ToneLeaderSymbolFraming() As Boolean

        Dim dblCarPh As Double
        Dim dblReal, dblImag As Double
        Dim intLocalPtr As Int32 = intMFSReadPtr  ' try advancing one symbol to minimize initial startup errors 
        Dim dblAbsPhErr As Double
        Dim dblMinAbsPhErr As Double = 5000 ' initialize to an excessive value
        Dim intIatMinErr As Integer
        Dim dblPhaseAtMinErr As Double
        Dim intAbsPeak As Int32 = 0
        Dim intJatPeak As Int32 = 0

        ' Use Phase of 1500 Hz leader  to establish symbol framing. Nominal phase is 0 or 180 degrees

        If (intFilteredMixedSamples.Length - intLocalPtr) < 500 Then Return False
        intLocalPtr = intMFSReadPtr + EnvelopeCorrelator() ' should position the pointer at the symbol boundary
        ' Check 3 samples either side of the intLocalPtr for minimum phase error.(closest to Pi or -Pi) 
        For i As Integer = -3 To 3 ' 0 To 0 '  -2 To 2 ' for just 5 samples
            ' using the full symbol seemed to work best on weak Signals (0 to -5 dB S/N) June 15, 2015
            GoertzelRealImag(intFilteredMixedSamples, intLocalPtr + i, 120, 15, dblReal, dblImag) ' Carrier at 1500 Hz nominal Positioning 
            dblCarPh = Atan2(dblImag, dblReal)
            dblAbsPhErr = Abs(dblCarPh - (Round(dblCarPh / PI) * PI))
            If dblAbsPhErr < dblMinAbsPhErr Then
                dblMinAbsPhErr = dblAbsPhErr
                intIatMinErr = i
                dblPhaseAtMinErr = dblCarPh
            End If
        Next i
        intMFSReadPtr = intLocalPtr + intIatMinErr
        Debug.WriteLine("[Acquire2ToneLeaderSymbolFraming] intIatMinError=" & intIatMinErr.ToString)
        State = ReceiveState.AcquireFrameSync
        If MCB.AccumulateStats Then
            stcTuningStats.intLeaderSyncs += 1
        End If
        'Debug.WriteLine("   [Acquire2ToneLeaderSymbolSync] iAtMinError = " & intIatMinErr.ToString & "   Ptr = " & intMFSReadPtr.ToString & "  MinAbsPhErr = " & Format(dblMinAbsPhErr, "#.00"))
        'Debug.WriteLine("   [Acquire2ToneLeaderSymbolSync]      Ph1500 @ MinErr = " & Format(dblPhaseAtMinErr, "#.000"))
        strDecodeCapture &= "Framing; iAtMinErr=" & intIatMinErr.ToString & ", Ptr=" & intMFSReadPtr.ToString & ", MinAbsPhErr=" & Format(dblMinAbsPhErr, "#.00") & ": "

        ' *********************************
        ' Debug code to look at filtered waveform
        'ReDim dblAcquire2ToneLeaderSymbolFraming(intFilteredMixedSamples.Length - intLocalPtr)
        'For k As Integer = 0 To dblAcquire2ToneLeaderSymbolFraming.Length - 1
        '    dblAcquire2ToneLeaderSymbolFraming(k) = intFilteredMixedSamples(k)
        'Next
        'Dim objWT As New WaveTools
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Acquire2ToneLeaderSymbSync.wav", 12000, 16, dblFilteredMixed)
        ' End of debug code
        '************************************
        Return True
    End Function  'Acquire2ToneLeaderSymbolSync

    ' Subroutine to Mix new samples with NCO to tune to nominal 1500 Hz center with reversed sideband and filter. 
    Public Sub MixNCOFilter(ByRef intNewSamples() As Int32, ByRef intReadPtr As Int32, dblOffsetHz As Double)
        ' Correct the dimension of intPriorMixedSamples if needed (should only happen after a bandwidth setting change). 

        If intNewSamples.Length = 0 Then Exit Sub
        ' Nominal NCO freq is 3000 Hz  to downmix intNewSamples  (NCO - Fnew) to center of 1500 Hz (invertes the sideband too) 
        dblNCOFreq = 3000 + dblOffsetHz
        dblNCOPhaseInc = dblNCOFreq * dbl2Pi / 12000
        ReDim intMixedSamples(intNewSamples.Length - 1 - intReadPtr)
        For i As Integer = 0 To intNewSamples.Length - 1 - intReadPtr
            intMixedSamples(i) = CInt(intNewSamples(intReadPtr + i) * Cos(dblNCOPhase)) ' later may want a lower "cost" implementation of "Cos"
            dblNCOPhase += dblNCOPhaseInc
            If dblNCOPhase > dbl2Pi Then dblNCOPhase -= dbl2Pi
        Next i
        intReadPtr = 0
        ReDim intNewSamples(-1)
        ' showed no significant difference if the 2000 Hz filer used for all bandwidths.
        FSMixFilter2000Hz() ' filter through the FS filter (required to reject image from Local oscillator)
    End Sub 'MixNCOFilter


    ' not currently used.  May help some on receivers with high noize above 3500 Hz audio.
    ' Subroutine to Mix new samples with NCO to tune to nominal 1500 Hz center with reversed sideband and filter. 
    Public Sub MixNCOFilterPrefilter(ByRef intNewSamples() As Int32, ByRef intReadPtr As Int32, dblOffsetHz As Double)
        ' Correct the dimension of intPriorMixedSamples if needed (should only happen after a bandwidth setting change). 

        If intNewSamples.Length = 0 Then Exit Sub
        Dim intSamplesToPreFilter(intNewSamples.Length - 1 - intReadPtr) As Int32
        Array.Copy(intNewSamples, intReadPtr, intSamplesToPreFilter, 0, intSamplesToPreFilter.Length)
        Dim intPreFilteredSamples(intSamplesToPreFilter.Length - 1) As Int32
        LPF2800_12000SR(intSamplesToPreFilter, intPreFilteredSamples)

        ' Nominal NCO freq is 3000 Hz  to downmix intNewSamples  (NCO - Fnew) to center of 1500 Hz (invertes the sideband too) 
        dblNCOFreq = 3000 + dblOffsetHz
        dblNCOPhaseInc = dblNCOFreq * dbl2Pi / 12000
        ReDim intMixedSamples(intPreFilteredSamples.Length - 1)
        For i As Integer = 0 To intPreFilteredSamples.Length - 1
            intMixedSamples(i) = CInt(intPreFilteredSamples(i) * Cos(dblNCOPhase)) ' later may want a lower "cost" implementation of "Cos"
            dblNCOPhase += dblNCOPhaseInc
            If dblNCOPhase > dbl2Pi Then dblNCOPhase -= dbl2Pi
        Next i
        intReadPtr = 0
        ReDim intNewSamples(-1)
        ' showed no significant difference if the 2000 Hz filer used for all bandwidths.
        FSMixFilter2000Hz() ' filter through the FS filter (required to reject image from Local oscillator)
    End Sub 'MixNCOFilterPrefilter

    '  Subroutine to Initialize mixed samples
    Public Sub InitializeMixedSamples()
        ' Measure the time from release of PTT to leader detection of reply.
        objMain.objProtocol.intARQRTmeasuredMs = Math.Min(10000, Now.Subtract(objMain.objProtocol.dttStartRTMeasure).TotalMilliseconds)
        ReDim intMixedSamples(-1) ' Zero the intMixedSamples array
        ReDim intPriorMixedSamples(119)   ' zero out prior samples in Prior sample buffer
        ReDim intFilteredMixedSamples(-1) ' zero out the FilteredMixedSamples array
        intMFSReadPtr = 30 ' reset the MFSReadPtr offset 30 to accomodate the filter delay
    End Sub 'InitializeMixedSamples

    ' Subroutine to apply 2000 Hz filter to mixed samples 
    Private Sub FSMixFilter2000Hz()
        ' assumes sample rate of 12000
        ' implements  23 100 Hz wide sections   (~2000 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables

        Dim intFilteredSamples(intMixedSamples.Length - 1) As Int32 '  Filtered samples

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 3/4/2014 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(26) As Double 'the coefficients
        Static dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Static dblZout_0(26) As Double ' resonator outputs
        Static dblZout_1(26) As Double ' resonator outputs delayed one sample
        Static dblZout_2(26) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2

        ' Initialize the coefficients
        If dblCoef(26) = 0 Then
            For i As Integer = 4 To 26
                dblCoef(i) = 2 * dblR * Cos(2 * PI * i / intN) ' For Frequency = bin i
            Next i
        End If
        Try
            For i As Integer = 0 To intMixedSamples.Length - 1
                If i < intN Then
                    dblZin = intMixedSamples(i) - dblRn * intPriorMixedSamples(i)
                Else
                    dblZin = intMixedSamples(i) - dblRn * intMixedSamples(i - intN)
                End If
                ' Compute the Comb
                dblZComb = dblZin - dblZin_2 * dblR2
                dblZin_2 = dblZin_1
                dblZin_1 = dblZin

                ' Now the resonators
                For j As Integer = 4 To 26   ' calculate output for 23 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 
                    ' Resonators 2 and 13 scaled by .389 get best shape and side lobe supression 
                    ' Scaling also accomodates for the filter "gain" of approx 60. 
                    If j = 4 Or j = 26 Then
                        intFilteredSamples(i) += 0.389 * dblZout_0(j)
                    ElseIf j Mod 2 = 0 Then
                        intFilteredSamples(i) += dblZout_0(j)
                    Else
                        intFilteredSamples(i) -= dblZout_0(j)
                    End If
                Next j
                intFilteredSamples(i) = intFilteredSamples(i) * 0.00833333333 ' rescales for gain of filter
            Next i
            ' update the prior intPriorMixedSamples array for the next filter call 
            Array.Copy(intMixedSamples, intMixedSamples.Length - intN, intPriorMixedSamples, 0, intPriorMixedSamples.Length)
            Dim intCopyPtr As Integer = intFilteredMixedSamples.Length
            ReDim Preserve intFilteredMixedSamples(intFilteredMixedSamples.Length + intFilteredSamples.Length - 1)
            Array.Copy(intFilteredSamples, 0, intFilteredMixedSamples, intCopyPtr, intFilteredSamples.Length)
            ReDim intMixedSamples(-1)
            ' *********************************
            ' Debug code to look at filter output
            'Dim dblFilteredMixed(intFilteredMixedSamples.Length - 1) As Double
            'For k As Integer = 0 To dblFilteredMixed.Length - 1
            '    dblFilteredMixed(k) = intFilteredMixedSamples(k)
            'Next
            'Dim objWT As New WaveTools
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\FSMixFilter2000Hz.wav", 12000, 16, dblFilteredMixed)
            ' End of debug code
            '************************************

        Catch ex As Exception
            Logs.Exception("[Demodulate.FSMixFilter2000] Exception: " & ex.ToString)
        End Try
    End Sub 'FSMixFilter2000Hz

    ' Function to apply 150Hz filter used in Envelope correlator
    Private Function Filter150Hz() As Int32()
        ' assumes sample rate of 12000
        ' implements  3 100 Hz wide sections   (~150 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables

        Dim intSamplesToFilter As Int32 = intFilteredMixedSamples.Length - intMFSReadPtr - 1
        Dim intFilterOut(intSamplesToFilter - 1) As Int32
        'Dim intFilterOut(479) As Int32
        Static dblR As Double = 0.99995  ' insures stability (must be < 1.0) (Value .9995 3/4/2014 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(16) As Double 'the coefficients
        Static dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Static dblZout_0(16) As Double ' resonator outputs
        Static dblZout_1(16) As Double ' resonator outputs delayed one sample
        Static dblZout_2(16) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2

        ' Initialize the coefficients
        If dblCoef(16) = 0 Then
            For i As Integer = 14 To 16
                dblCoef(i) = 2 * dblR * Cos(2 * PI * i / intN) ' For Frequency = bin i
            Next i
        End If
        Try
            For i As Integer = 0 To 479 'intSamplesToFilter - 1
                If i < intN Then
                    dblZin = intFilteredMixedSamples(intMFSReadPtr + i) - dblRn * 0 ' no prior mixed samples
                Else
                    dblZin = intFilteredMixedSamples(intMFSReadPtr + i) - dblRn * intFilteredMixedSamples(intMFSReadPtr + i - intN)
                End If
                ' Compute the Comb
                dblZComb = dblZin - dblZin_2 * dblR2
                dblZin_2 = dblZin_1
                dblZin_1 = dblZin

                ' Now the resonators
                For j As Integer = 14 To 16   ' calculate output for 3 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 

                    ' Scaling also accomodates for the filter "gain" of approx 120. 
                    ' These transition coefficients fairly close to optimum for WGN 0db PSK4, 100 baud (yield highest average quality) 5/24/2014
                    If j = 14 Or j = 16 Then
                        intFilterOut(i) = 0.2 * dblZout_0(j) ' this transisiton minimizes ringing and peaks
                    Else
                        intFilterOut(i) -= dblZout_0(j)
                    End If
                Next j
                intFilterOut(i) = intFilterOut(i) * 0.00833333333 ' rescales for gain of filter
            Next i
            Return intFilterOut
        Catch ex As Exception
            Logs.Exception("[Demodulate.Filter150Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
    End Function 'Filter150Hz

    ' Function to acquire the 4FSK frame type
    Public Function Acquire4FSKFrameType() As Int32
        ' intMFSReadPtr is pointing to start of first symbol of Frame Type (total of 10 4FSK symbols in frame type (2 bytes) + 1 parity symbol per byte 
        ' Returns -1 if minimal distance decoding is below threshold (low likelyhood of being correct)
        ' Returns -2 if insufficient samples 
        ' Else returns frame type 0-255

        If (intFilteredMixedSamples.Length - intMFSReadPtr) < (240 * 10) Then Return -2 ' Check for 12 available 4FSK Symbols (but only 10 are used)  

      
        Dim intToneMags() As Int32 = Nothing
        If Not DemodFrameType4FSK(intMFSReadPtr, intFilteredMixedSamples, intToneMags) Then
            objMain.Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)
            intMFSReadPtr += (240 * 10)
            Return -1
        End If
        intRmtLeaderMeasure = CInt(Now.Subtract(dttStartRmtLeaderMeasure).TotalMilliseconds)
        ' Now do check received  Tone array for testing minimum distance decoder
        If objMain.objProtocol.Pending Then ' If we have a pending connection (btween the IRS first decode of ConReq until it receives a ConAck from the iSS)  
            Acquire4FSKFrameType = MinimalDistanceFrameType(intToneMags, objMain.objProtocol.PendingSessionID) ' The pending session ID will become the session ID once connected) 
        ElseIf objMain.objProtocol.Connected Then ' If we are connected then just use the stcConnection.bytSessionID
            Acquire4FSKFrameType = MinimalDistanceFrameType(intToneMags, stcConnection.bytSessionID)
        Else ' not connected and not pending so use &FF (FEC or ARQ unconnected session ID
            Acquire4FSKFrameType = MinimalDistanceFrameType(intToneMags, &HFF)
        End If
        If Acquire4FSKFrameType > &H30 And Acquire4FSKFrameType < &H39 Then objMain.objHI.QueueCommandToHost("PENDING") ' early pending notice to stop scanners
        If Acquire4FSKFrameType >= 0 AndAlso objFrameInfo.IsShortControlFrame(Acquire4FSKFrameType) Then ' update the constellation if a short frame (no data to follow)
            objMain.Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)
        End If
        If MCB.AccumulateStats Then
            If Acquire4FSKFrameType > 0 Then
                stcTuningStats.intGoodFSKFrameTypes += 1
            Else
                stcTuningStats.intFailedFSKFrameTypes += 1
            End If
        End If
        intMFSReadPtr += (240 * 10) ' advance to read pointer to the next symbol (if there is one)
        Return Acquire4FSKFrameType
    End Function 'Acquire4FSKFrameType

    ' Function to compute the "distance" from a speicific bytFrame Xored by bytID using 1 symbol parity 
    Function ComputeDecodeDistance(intTonePtr As Integer, intToneMags() As Int32, bytFrameType As Byte, bytID As Byte) As Double
        'intTonePtr is the offset into the Frame type symbols. 0 for first Frame byte 20 = (5 x 4) for second frame byte 
        Dim dblDistance As Double
        Dim int4ToneSum As Int32
        Dim intToneIndex As Int32
        Dim bytMask As Byte = &HC0

        For j As Integer = 0 To 4 ' over 5 symbols
            int4ToneSum = 0
            For k As Integer = 0 To 3
                int4ToneSum += intToneMags(intTonePtr + (4 * j) + k)
            Next k
            If int4ToneSum = 0 Then int4ToneSum = 1 ' protects against possible overflow
            If j < 4 Then
                intToneIndex = ((bytFrameType Xor bytID) And bytMask) >> (6 - 2 * j)
            Else
                intToneIndex = objFrameInfo.ComputeTypeParity(bytFrameType)
            End If
            dblDistance += (1 - CDbl(intToneMags(intTonePtr + (4 * j) + intToneIndex)) / CDbl(int4ToneSum))
            bytMask = bytMask >> 2
        Next
        dblDistance = dblDistance / 5  ' normalize back to 0 to 1 range 
        Return dblDistance
    End Function

    ' Function to compute the frame type by selecting the minimal distance from all valid frame types.
    Private Function MinimalDistanceFrameType(intToneMags() As Int32, bytSessionID As Byte) As Int32

        Dim dblMinDistance1 As Double = 5 ' minimal distance for the first byte initialize to large value
        Dim dblMinDistance2 As Double = 5 ' minimal distance for the second byte initialize to large value
        Dim dblMinDistance3 As Double = 5 ' minimal distance for the second byte under exceptional cases initialize to large value
        Dim intIatMinDistance1, intIatMinDistance2, intIatMinDistance3 As Integer
        Dim dblDistance1, dblDistance2, dblDistance3 As Double

        For i As Integer = 0 To objFrameInfo.bytValidFrameTypes.Length - 1
            dblDistance1 = ComputeDecodeDistance(0, intToneMags, objFrameInfo.bytValidFrameTypes(i), &H0)
            dblDistance2 = ComputeDecodeDistance(20, intToneMags, objFrameInfo.bytValidFrameTypes(i), bytSessionID)
            If objMain.objProtocol.Pending Then
                dblDistance3 = ComputeDecodeDistance(20, intToneMags, objFrameInfo.bytValidFrameTypes(i), &HFF)
            Else
                dblDistance3 = ComputeDecodeDistance(20, intToneMags, objFrameInfo.bytValidFrameTypes(i), objMain.objProtocol.bytLastARQSessionID)
            End If
            If dblDistance1 < dblMinDistance1 Then
                dblMinDistance1 = dblDistance1
                intIatMinDistance1 = objFrameInfo.bytValidFrameTypes(i)
            End If
            If dblDistance2 < dblMinDistance2 Then
                dblMinDistance2 = dblDistance2
                intIatMinDistance2 = objFrameInfo.bytValidFrameTypes(i)
            End If
            If dblDistance3 < dblMinDistance3 Then
                dblMinDistance3 = dblDistance3
                intIatMinDistance3 = objFrameInfo.bytValidFrameTypes(i)
            End If
        Next i
       
        With objMain.objProtocol
            If bytSessionID = &HFF Then ' we are in a FEC QSO, monitoring an ARQ session or have not yet reached the ARQ Pending or Connected status 

                ' This handles the special case of a DISC command received from the prior session (where the station sending DISC did not receive an END). 
                If (intIatMinDistance1 = &H29 And intIatMinDistance3 = &H29) And ((dblMinDistance1 < 0.4) Or (dblMinDistance3 < 0.4)) Then
                    strDecodeCapture &= "MD Decode;1 ID=H" & Format(.bytLastARQSessionID, "X") & ", Type=H29:" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D3=" & Format(dblMinDistance3, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                    Return intIatMinDistance1

                    ' no risk of damage to and existing ARQConnection with END, BREAK, DISC, or ACK frames so loosen decoding threshold 
                ElseIf (intIatMinDistance1 = intIatMinDistance2) And ((dblMinDistance1 < 0.4) Or (dblMinDistance2 < 0.4)) Then
                    strDecodeCapture &= "MD Decode;2 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                    dblOffsetLastGoodDecode = dblOffsetHz : dttLastGoodFrameTypeDecode = Now
                    Return intIatMinDistance1
                ElseIf (dblMinDistance1 < 0.3) And (dblMinDistance1 < dblMinDistance2) And objFrameInfo.IsDataFrame(intIatMinDistance1) Then ' this would handle the case of monitoring an ARQ connection where the SessionID is not &HFF
                    strDecodeCapture &= "MD Decode;3 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                    Return intIatMinDistance1
                ElseIf (dblMinDistance2 < 0.3) And (dblMinDistance2 < dblMinDistance1) And objFrameInfo.IsDataFrame(intIatMinDistance2) Then ' this would handle the case of monitoring an FEC transmission that failed above when the session ID is = &HFF
                    strDecodeCapture &= "MD Decode;4 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                    Return intIatMinDistance2
                Else
                    strDecodeCapture &= "MD Decode;5 Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                End If

            ElseIf objMain.objProtocol.Pending Then ' We have a Pending ARQ connection 
                ' this should be a Con Ack from the ISS if we are Pending
                If (intIatMinDistance1 = intIatMinDistance2) Then 'matching indexes at minimal distances so high probablity of correct decode.
                    strDecodeCapture &= "MD Decode;6 ID=H" & Format(bytSessionID, "X") & ", type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If ((dblMinDistance1 < 0.4) Or (dblMinDistance2 < 0.4)) Then
                        dblOffsetLastGoodDecode = dblOffsetHz : dttLastGoodFrameTypeDecode = Now ' This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
                        If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                        Return intIatMinDistance1
                    Else
                        If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                        Return -1 ' indicates poor quality decode so  don't use
                    End If

                    ' handles the case of a received ConReq frame based on an ID of &HFF (ISS must have missed ConAck reply from IRS so repeated ConReq)
                ElseIf (intIatMinDistance1 = intIatMinDistance3) Then 'matching indexes at minimal distances so high probablity of correct decode.
                    strDecodeCapture &= "MD Decode;7 ID=H" & Format(&HFF, "X") & ", type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D3=" & Format(dblMinDistance3, "#.00")
                    If (intIatMinDistance1 >= &H31 And intIatMinDistance1 <= &H38) And ((dblMinDistance1 < 0.4) Or (dblMinDistance3 < 0.4)) Then ' Check for ConReq (ISS must have missed previous ConAck  
                        dblOffsetLastGoodDecode = dblOffsetHz : dttLastGoodFrameTypeDecode = Now ' This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
                        If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                        Return intIatMinDistance1
                    Else
                        If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                        Return -1 ' indicates poor quality decode so  don't use
                    End If
                End If

            ElseIf objMain.objProtocol.Connected Then ' we have an ARQ connected session.
                If MCB.AccumulateStats Then
                    With stcTuningStats
                        .dblAvgDecodeDistance = (.dblAvgDecodeDistance * .intDecodeDistanceCount + 0.5 * (dblMinDistance1 + dblMinDistance2)) / (.intDecodeDistanceCount + 1)
                        .intDecodeDistanceCount += 1
                    End With
                End If
                If (intIatMinDistance1 = intIatMinDistance2) Then 'matching indexes at minimal distances so high probablity of correct decode.
                    If (intIatMinDistance1 >= &HE0 And intIatMinDistance1 <= &HFF) Or (intIatMinDistance1 = &H23) Or _
                        (intIatMinDistance1 = &H2C) Or (intIatMinDistance1 = &H29) Then ' Check for critical ACK, BREAK, END, or DISC frames  
                        strDecodeCapture &= "MD Decode;8 ID=H" & Format(bytSessionID, "X") & ", Critical type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                        If ((dblMinDistance1 < 0.25) Or (dblMinDistance2 < 0.25)) Then ' use tighter limits   here
                            dblOffsetLastGoodDecode = dblOffsetHz : dttLastGoodFrameTypeDecode = Now ' This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
                            If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)

                            Return intIatMinDistance1
                        Else
                            If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                            Return -1 ' indicates poor quality decode so  don't use
                        End If
                    Else ' non critical frames 
                        strDecodeCapture &= "MD Decode;9 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                        ' use looser limits here, there is no risk of protocol damage from these frames
                        If ((dblMinDistance1 < 0.4) Or (dblMinDistance2 < 0.4)) Then
                            If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
                            dblOffsetLastGoodDecode = dblOffsetHz : dttLastGoodFrameTypeDecode = Now ' This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
                            Return intIatMinDistance1
                        Else
                            If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                            Return -1 ' indicates poor quality decode so  don't use
                        End If
                    End If
                Else 'non matching indexes
                    strDecodeCapture &= "MD Decode;10  Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
                    If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
                    Return -1 ' indicates poor quality decode so  don't use
                End If
            End If
            strDecodeCapture &= "MD Decode;11  Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
            If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
            Return -1 ' indicates poor quality decode so  don't use
        End With
    End Function ' MinimalDistanceFrameType

    ' Function to Demodulate and Decode 1 carrier 4FSK 50 baud Connect Request 
    Public Function DemodDecode4FSKConReq(bytFrameType As Byte, ByRef intBW As Integer, ByRef strCaller As String, ByRef strTarget As String) As Boolean

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intCenterFreq, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim bytCall(5) As Byte
        Dim blnRSOK As Boolean = False
        Dim bytDecodedRS() As Byte

        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        intNumSymbolsPerCar = (intDataLen + intRSLen) * 4 '  Two compressed call signs + 1 byte CRC 
        intCenterFreq = 1500
        Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, intCenterFreq, intNumSymbolsPerCar, bytRawData, intToneMags)
        objMain.Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)
        stcQualityStats.int4FSKQuality = intLastRcvdFrameQuality

        ' Modified May 24, 2015 to use RS encoding vs CRC (similar to ID Frame)
        objRS8.MaxCorrections = 1
        bytDecodedRS = objRS8.RSDecode(bytRawData, blnRSOK)
        Array.Copy(bytDecodedRS, 0, bytCall, 0, 6)
        strCaller = DeCompressCallsign(bytCall)
        Array.Copy(bytDecodedRS, 6, bytCall, 0, 6)
        strTarget = DeCompressCallsign(bytCall)
        strRcvFrameTag = "_" & strCaller & " > " & strTarget
        Dim bytCheck() As Byte = objRS8.RSEncode(bytDecodedRS)
        If (bytCheck(12) = bytRawData(12)) Or (bytCheck(13) = bytRawData(13)) And blnRSOK Then
            Select Case bytFrameType
                Case 31 : intBW = 200
                Case 32 : intBW = 500
                Case 33 : intBW = 1000
                Case 34 : intBW = 2000
            End Select
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            intTestFrameCorrectCnt += 1
            Return True
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
            Return False
        End If
    End Function 'DemodDecode4FSKConReq

    ' Function to Demod and Decode 1 carrier 4FSK 50 baud ID frame  
    Public Function DemodDecode4FSKID(bytFrameType As Byte, ByRef strCallID As String, ByRef strGridSquare As String) As Boolean
        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intCenterFreq, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim bytCall(5) As Byte
        Dim blnRSOK As Boolean = False
        Dim bytDecodedRS() As Byte

        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        intNumSymbolsPerCar = (intDataLen + intRSLen) * 4 '  Compresssed Call + Compressed GS + 2 byte RS Parity 
        intCenterFreq = 1500
        Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, intCenterFreq, intNumSymbolsPerCar, bytRawData, intToneMags)
        objMain.Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)
        objRS8.MaxCorrections = 1
        bytDecodedRS = objRS8.RSDecode(bytRawData, blnRSOK)
        Array.Copy(bytDecodedRS, 0, bytCall, 0, 6)
        strCallID = DeCompressCallsign(bytCall)
        Array.Copy(bytDecodedRS, 6, bytCall, 0, 6)
        strGridSquare = DeCompressGridSquare(bytCall).Trim
        If strGridSquare.Length = 6 Then
            strGridSquare = "[" & strGridSquare.Substring(0, 4).ToUpper & strGridSquare.Substring(4, 2).ToLower & "]"
        ElseIf strGridSquare.Length = 8 Then
            strGridSquare = "[" & strGridSquare.Substring(0, 4).ToUpper & strGridSquare.Substring(4, 2).ToLower & strGridSquare.Substring(6, 2) & "]"
        Else
            strGridSquare = "[" & strGridSquare.ToUpper & "]"
        End If
        strRcvFrameTag = "_" & strCallID & " " & strGridSquare
        Dim bytCheck() As Byte = objRS8.RSEncode(bytDecodedRS)
        If (bytCheck(12) = bytRawData(12)) Or (bytCheck(13) = bytRawData(13)) And blnRSOK Then
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            intTestFrameCorrectCnt += 1
            Return True
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
            Return False
        End If
    End Function 'DemodDecode4FSKID

    ' Function to Demod and Decode 1 carrier 4FSK 50 baud Connect Ack with timing 
    Public Function DemodDecode4FSKConACK(bytFrameType As Byte, ByRef intTiming As Integer) As Boolean

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intCenterFreq, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim bytCall(5) As Byte

        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        intTiming = -1
        intNumSymbolsPerCar = (intDataLen + intRSLen) * 4 '  Timing info repeated three times 
        intCenterFreq = 1500
        Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, intCenterFreq, intNumSymbolsPerCar, bytRawData, intToneMags)
        objMain.Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)

        If bytRawData(0) = bytRawData(1) Then
            intTiming = 10 * CInt(bytRawData(0))
        ElseIf bytRawData(0) = bytRawData(2) Then
            intTiming = 10 * CInt(bytRawData(0))
        ElseIf bytRawData(1) = bytRawData(2) Then
            intTiming = 10 * CInt(bytRawData(1))
        End If
        If intTiming >= 0 Then
            strRcvFrameTag = "_" & intTiming.ToString & " ms"
            If MCB.DebugLog Then Logs.WriteDebug("[DemodDecode4FSKConACK]  Remote leader timing reported: " & intTiming.ToString & " ms")
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            intTestFrameCorrectCnt += 1
            stcConnection.intReceivedLeaderLen = intLeaderRcvdMs
            bytLastDataFrameType = 0 ' initialize the LastFrameType to an illegal Data frame
            Return True
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
            Return False
        End If
    End Function 'DemodDecode4FSKConACK

    '  Function to Decode Frames based on frame type
    Public Function DecodeFrame(intFrameType As Int32, ByRef bytData() As Byte) As Boolean
        Dim bytFrameData() As Byte = Nothing
        Dim blnDecodeOK As Boolean = False
        Dim stcStatus As Status = Nothing
        Dim strCallerCallsign As String = ""
        Dim strTargetCallsign As String = ""
        Dim intRcvdQuality As Integer
        Dim intPhase(-1) As Int16
        Dim intMag(-1) As Int16
        Dim strIDCallSign As String = ""
        Dim strGridSQ As String = ""
        Dim intBW As Int32
        Dim intTiming As Int32
        Dim intConstellationQuality As Int32

        ReDim bytData(-1)
        strRcvFrameTag = ""
        stcStatus.ControlName = "lblRcvFrame"

        'DataACK/NAK and short control frames 
        If intFrameType >= 0 And intFrameType <= &H1F Or intFrameType >= &HE0 Then ' DataACK/NAK
            blnDecodeOK = DecodeACKNAK(intFrameType, intRcvdQuality)
            stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag
        ElseIf (objFrameInfo.IsShortControlFrame(intFrameType)) Then ' Short Control Frames
            blnDecodeOK = True
            stcStatus.Text = objFrameInfo.Name(intFrameType)
        End If

        ' Special Frames
        Select Case intFrameType
            Case &H39, &H3A, &H3B, &H3C ' Connect ACKs with Timing
                blnDecodeOK = DemodDecode4FSKConACK(intFrameType, intTiming)
                If blnDecodeOK Then ReDim bytData(0) : bytData(0) = CByte(intTiming / 10)
                stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

            Case &H30 ' ID Frame
                blnDecodeOK = DemodDecode4FSKID(&H30, strIDCallSign, strGridSQ)
                bytData = GetBytes("ID:" & strIDCallSign & " " & strGridSQ & ": ")
                stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

            Case &H31, &H32, &H33, &H34, &H35, &H36, &H37, &H38    ' Connect Request
                blnDecodeOK = DemodDecode4FSKConReq(intFrameType, intBW, strCallerCallsign, strTargetCallsign)
                stcStatus.Text = objFrameInfo.Name(intFrameType) & " " & strCallerCallsign & ">" & strTargetCallsign
                If blnDecodeOK Then bytData = GetBytes(strCallerCallsign & " " & strTargetCallsign)

                ' 1 Carrier Data frames
                '  PSK Data
            Case &H40, &H41, &H42, &H43, &H44, &H45 ' OK
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

                ' 4FSK Data
            Case &H46, &H47, &H48, &H49, &H4A, &H4B, &H4C, &H4D
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 8FSK Data
            Case &H4E, &H4F
                blnDecodeOK = Decode8FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 4FSK Data (600 bd)
            Case &H7A, &H7B, &H7C, &H7D
                blnDecodeOK = Decode4FSK600bdData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 16FSK Data
            Case &H58, &H59, &H5A, &H5B
                blnDecodeOK = Decode16FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)

                ' 2 Carrier Data frames
            Case &H50, &H51, &H52, &H53, &H54, &H55, &H56, &H57
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

                ' 1000 Hz  Data frames
            Case &H60, &H61, &H62, &H63, &H64, &H65, &H66, &H67
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

            Case &H68, &H69
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If MCB.DebugLog Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode " & blnDecodeOK.ToString & "  Quality= " & intLastRcvdFrameQuality.ToString)

                ' 2000 Hz Data frames
            Case &H70, &H71, &H72, &H73, &H74, &H75, &H76, &H77
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If
            Case &H78, &H79
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
            Case &H7A, &H7B, &H7C, &H7D
                blnDecodeOK = Decode4FSK600bdData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)

                ' Experimental Sounding frame
            Case &HD0
                DemodSounder(intMFSReadPtr, intFilteredMixedSamples)
                blnDecodeOK = True
        End Select
        If blnDecodeOK Then
            stcStatus.BackColor = Color.LightGreen
        Else
            stcStatus.BackColor = Color.LightSalmon
        End If
        queTNCStatus.Enqueue(stcStatus)
        If intConstellationQuality > 0 Then
            intLastRcvdFrameQuality = intConstellationQuality
            If MCB.DebugLog And Not blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode FAIL,   Constellation Quality= " & intLastRcvdFrameQuality.ToString)
            If MCB.DebugLog And blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode PASS,   Constellation Quality= " & intLastRcvdFrameQuality.ToString)
        Else
            If MCB.DebugLog And Not blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode FAIL")
            If MCB.DebugLog And blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode PASS")
        End If
        Return blnDecodeOK
    End Function ' DecodeFrame

    ' Function to Strip quality from ACK/NAK frame types
    Private Function DecodeACKNAK(intFrameType As Int32, ByRef intQuality As Int32) As Boolean
        intQuality = 38 + CInt(2 * (intFrameType And &H1F)) 'mask off lower 5 bits ' Range of 38 to 100 in steps of 2
        strRcvFrameTag = "_Q" & intQuality.ToString
        Return True
    End Function 'DecodeACKNAK

    ' Subroutine to "rotate" the phases to try and set the average offset to 0. 
    Private Sub CorrectPhaseForTuningOffset(ByRef intPhase() As Int16, strMod As String)
        ' A tunning error of -1 Hz will rotate the phase calculation Clockwise ~ 64 milliradians (~4 degrees)
        '  This corrects for:
        '1) Small tuning errors which result in a phase bias (rotation) of then entire constellation
        '2) Small Transmitter/receiver drift during the frame by averaging and adjusting to constellation to the average. 
        '  It only processes phase values close to the nominal to avoid generating too large of a correction from outliers: +/- 30 deg for 4PSK, +/- 15 deg for 8PSK
        ' Is very affective in handling initial tuning error.  

        Dim intPSKMode As Int32
        If strMod = "8PSK" Then
            intPSKMode = 8
        ElseIf strMod = "4PSK" Then
            intPSKMode = 4
        Else
            Logs.Exception("[DemodulateDecode.CorrectPhaseForOffset] unknown strMod: " & strMod)
            Exit Sub
        End If
        Dim intPhaseMargin As Int16 = 2095 \ intPSKMode ' Compute the acceptable phase correction range (+/-30 degrees for 4 PSK)
        Dim intPhaseInc As Int16 = 6284 \ intPSKMode
        Dim intTest As Integer
        Dim intOffset, intAvgOffset, intAvgOffsetBeginning, intAvgOffsetEnd As Integer
        Dim intAccOffsetCnt, intAccOffsetCntBeginning, intAccOffsetCntEnd As Int32

        Try
            ' Compute the average offset (rotation) for all symbols within +/- intPhaseMargin of nominal
            Dim intAccOffset, intAccOffsetBeginning, intAccOffsetEnd As Int32
            For i As Integer = 0 To intPhase.Length - 1
                intTest = Round(intPhase(i) / intPhaseInc)
                intOffset = intPhase(i) - intTest * intPhaseInc
                If (intOffset >= 0 And intOffset <= intPhaseMargin) Or (intOffset < 0 And intOffset >= -intPhaseMargin) Then
                    intAccOffsetCnt += 1
                    intAccOffset += intOffset
                    If i <= intPhase.Length \ 4 Then
                        intAccOffsetCntBeginning += 1
                        intAccOffsetBeginning += intOffset
                    ElseIf i >= (3 * intPhase.Length) \ 4 Then
                        intAccOffsetCntEnd += 1
                        intAccOffsetEnd += intOffset
                    End If
                End If
            Next i
            If intAccOffsetCnt > 0 Then intAvgOffset = Round(intAccOffset / intAccOffsetCnt)
            If intAccOffsetCntBeginning > 0 Then intAvgOffsetBeginning = Round(intAccOffsetBeginning / intAccOffsetCntBeginning)
            If intAccOffsetCntEnd > 0 Then intAvgOffsetEnd = Round(intAccOffsetEnd / intAccOffsetCntEnd)
            'Debug.WriteLine("[CorrectPhaseForOffset] Beginning: " & intAvgOffsetBeginning.ToString & "   End: " & intAvgOffsetEnd.ToString & "    Total: " & intAvgOffset.ToString)

            If (intAccOffsetBeginning > intPhase.Length \ 8) And (intAccOffsetCntEnd > intPhase.Length \ 8) Then
                For i As Integer = 0 To intPhase.Length - 1
                    intPhase(i) = intPhase(i) - ((intAvgOffsetBeginning * (intPhase.Length - i) / intPhase.Length) + (intAvgOffsetEnd * i / intPhase.Length))
                    If intPhase(i) > 3142 Then
                        intPhase(i) -= 6284
                    ElseIf intPhase(i) < -3142 Then
                        intPhase(i) += 6284
                    End If
                Next i
                If MCB.DebugLog Then Logs.WriteDebug("[CorrectPhaseForTuningOffset] AvgOffsetBeginning=" & intAvgOffsetBeginning.ToString & "  AvgOffsetEnd=" & intAvgOffsetEnd.ToString & " AccOffsetCnt=" & intAccOffsetCnt.ToString & "/" & intPhase.Length.ToString)
            ElseIf intAccOffsetCnt > intPhase.Length \ 2 Then
                For i As Integer = 0 To intPhase.Length - 1
                    intPhase(i) = intPhase(i) - intAvgOffset
                    If intPhase(i) > 3142 Then
                        intPhase(i) -= 6284
                    ElseIf intPhase(i) < -3142 Then
                        intPhase(i) += 6284
                    End If
                Next i
                If MCB.DebugLog Then Logs.WriteDebug("[CorrectPhaseForTuningOffset] AvgOffset=" & intAvgOffset.ToString & " AccOffsetCnt=" & intAccOffsetCnt.ToString & "/" & intPhase.Length.ToString)
            End If
        Catch ex As Exception
            Logs.Exception("[DemodulateDecode12K.CorrectPhaseForTuningOffset] Err:" & ex.ToString)
        End Try
    End Sub 'CorrectPhaseForTuningOffset

    ' Function to establish symbol sync 
    Private Function EnvelopeCorrelator() As Integer
        ' Compute the two symbol correlation with the Two tone leader template.
        ' slide the correlation one sample and repeat up to 120 steps 
        ' keep the point of maximum or minimum correlation...and use this to identify the the symbol start. 

        Dim dblCorMax As Double = -1000000.0 ' Preset to excessive values
        Dim dblCorMin As Double = 1000000.0
        Dim intJatMax, intJatMin As Int32
        Dim dblCorSum As Double
        If intFilteredMixedSamples.Length < intMFSReadPtr + 480 Then Return -1
        Dim int150HzFilered() As Int32 = Filter150Hz() ' This filter appears to help reduce avg decode distance (10 frames) by about 14%-19% at WGN-5 May 3, 2015
        For j As Integer = 0 To 119
            dblCorSum = 0
            For i As Integer = 0 To 239 ' over two 100 baud symbols (may be able to reduce to 1 symbol)
                If i < 120 Then
                    dblCorSum += objMain.objMod.intTwoToneLeaderTemplate(i) * int150HzFilered(120 + i + j)
                Else
                    dblCorSum -= objMain.objMod.intTwoToneLeaderTemplate(i - 120) * int150HzFilered(120 + i + j)
                End If
            Next i
            If dblCorSum > dblCorMax Then
                dblCorMax = dblCorSum
                intJatMax = j
            End If
            If dblCorSum < dblCorMin Then
                dblCorMin = dblCorSum
                intJatMin = j
            End If
        Next j
        If dblCorMax > Abs(dblCorMin) Then
            Return intJatMax
        Else
            Return intJatMin + 120
        End If
    End Function 'EnvelopeCorrelator

    ' Function to demod all PSKData frames single or multiple carriers 
    Private Function DemodPSK(bytFrameType As Byte, ByRef intPhases() As Int16, ByRef intMags() As Int16, ByRef intQuality As Int32) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For each carrier demodulate the frame yielding intPhases() and intMags() for that carrier, appending each to intPhases() and intMags() in carrier order
        '   Update the quality display for the entire frame
        '   The results (intPhases() and intMags() by ref) will be processed by DecodePSK
        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intCarFreq, intNumSymbolsPerCar, intPSKMode As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim intCarPhases(0) As Int16, intCarMags(0) As Int16
        Dim bytQualThresh As Byte
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        intPSKMode = CInt(strMod.Substring(0, 1))
        ' Assign the starting carrier
        If intNumCar = 1 Then
            intCarFreq = 1500
        Else
            intCarFreq = 1400 + (intNumCar \ 2) * 200 ' start at the highest carrier freq which is actually the lowest transmitted carrier due to Reverse sideband mixing
        End If
        Select Case strMod
            Case "4PSK" : intNumSymbolsPerCar = (1 + intDataLen + 2 + intRSLen) * 4     ' bytCount, data, CRC, RS not counting reference symbol 
            Case "8PSK" : intNumSymbolsPerCar = (1 + intDataLen + 2 + intRSLen) * 8 \ 3
        End Select

        ReDim intPhases(intNumSymbolsPerCar * intNumCar - 1)
        ReDim intMags(intNumSymbolsPerCar * intNumCar - 1)

        ' Repeatedly call Demod1CarPSK once for each carrier building the phases and mags. 
        For i As Integer = 0 To intNumCar - 1
            Demod1CarPSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, intCarFreq, intNumSymbolsPerCar, intCarPhases, intCarMags, strMod, True)
            Array.Copy(intCarPhases, 0, intPhases, i * intCarPhases.Length, intCarPhases.Length)
            Array.Copy(intCarMags, 0, intMags, i * intCarMags.Length, intCarMags.Length)
            intCarFreq -= 200  ' Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 
        Next
        objMain.UpdatePhaseConstellation(intPhases, intMags, strMod, intQuality)
        Return True
    End Function ' DemodPSK

    ' Function to demod and decode all low baud (< 300 baud) 4FSKData frames single or multiple carriers 
    Private Function DemodDecode4FSKData(bytFrameType As Byte, ByRef bytData() As Byte, ByRef intQuality As Integer) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For each carrier demodulate the frame yielding the 4 tone magnitudes for that carrier, appending each  in carrier order
        '   Update the 4FSK quality display for the entire frame

        ' This version creates a rolling average of intToneMags on failed decodes and uses the average to try and recover (analog tone magnitude average)  
        '  Need to evaluate effectiveness of this vs complexity...it should however all still fit within this routine.
        ' Test 3/24 shows avaraging is effective at low S/N with and without multipath


        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytCarData() As Byte = Nothing
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytJustData() As Byte = Nothing
        Dim bytNoRS() As Byte = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim blnSuccess As Boolean = False


        ' These static variables used to accumuate correctly decoded carriers on simultaneous multicarrier 4FSK modes.
        'Static bytLastFrameType As Byte = 0
        Static bytCorrectedDataCar1() As Byte
        Static bytCorrectedDataCar2() As Byte
        Static bytCorrectedDataCar3() As Byte
        Static bytCorrectedDataCar4() As Byte
        'Static bytPass As Byte = 0
        Dim bytPass As Byte = 0
        Static intToneMagHistory(3, 0) As Int32
        Static intSumCounts(0) As Int32

        Static intTestCounts As Int32 = 0
        Static intPassCounts As Int32 = 0

        intTestCounts += 1

        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        intNumSymbolsPerCar = (1 + intDataLen + 2 + intRSLen) * 4 '  byte count, data, 2 byte CRC, RS 
        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedDataCar1(-1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataCar2(-1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataCar3(-1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataCar4(-1) ' Re initialize the bytCorrectedData array
            ReDim intToneMagHistory(3, 4 * intNumSymbolsPerCar - 1) ' clear avg ToneMagHistory used for analog Mem ARQ
            ReDim intSumCounts(3) ' zero out the counts in the averages
        End If
        '

        Dim intToneMagsAllCar(intNumCar * 4 * intNumSymbolsPerCar - 1) As Int32
        Select Case intNumCar
            ' Note car frequences go from high to low to accomodate sideband reversal after mixing 
            Case 1
                If (bytPass And &H80) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H80)
                    ElseIf intSumCounts(0) = 0 Then ' initialize the TonMagHistory for the first carrier
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = intToneMags(m)
                        Next
                        intSumCounts(0) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = (intToneMagHistory(0, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(0, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                            bytPass = bytPass Or &H80
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(0) = 0
                        Else
                            intSumCounts(0) += 1
                        End If
                    End If
                End If

            Case 2
                If (bytPass And &H80) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1750, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H80)
                    ElseIf intSumCounts(0) = 0 Then
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = intToneMags(m)
                        Next
                        intSumCounts(0) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = (intToneMagHistory(0, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(0, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        'Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1750, intNumSymbolsPerCar, bytRawData, intToneMags)

                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                            bytPass = bytPass Or &H80
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(0) = 0
                        Else
                            intSumCounts(0) += 1
                        End If
                    End If
                End If


                If (bytPass And &H40) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1250, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 4 * intNumSymbolsPerCar, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar2(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar2, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H40)
                    ElseIf intSumCounts(1) = 0 Then
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(1, m) = intToneMags(m)
                        Next
                        intSumCounts(1) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(1, m) = (intToneMagHistory(1, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(1, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        ' Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1250, intNumSymbolsPerCar, bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 4 * intNumSymbolsPerCar, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar2(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar2, bytCarData.Length, bytCarData.Length)
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            bytPass = bytPass Or &H40
                            intSumCounts(1) = 0
                        Else
                            intSumCounts(1) += 1
                        End If
                    End If
                End If
            Case 4
                If (bytPass And &H80) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 2250, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H80)
                    ElseIf intSumCounts(0) = 0 Then ' capture tones for later averaging
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = intToneMags(m)
                        Next
                        intSumCounts(0) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(0, m) = (intToneMagHistory(0, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(0, m)
                        Next
                        'Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 2250, intNumSymbolsPerCar, bytRawData, intToneMags)
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                            bytPass = bytPass Or &H80
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(0) = 0
                        Else
                            intSumCounts(0) += 1
                        End If
                    End If
                End If

                If (bytPass And &H40) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1750, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 4 * intNumSymbolsPerCar, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar2(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar2, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H40)
                    ElseIf intSumCounts(1) = 0 Then
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(1, m) = intToneMags(m)
                        Next
                        intSumCounts(1) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(1, m) = (intToneMagHistory(1, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(1, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        'Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1750, intNumSymbolsPerCar, bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 4 * intNumSymbolsPerCar, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar2(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar2, 0, bytCarData.Length)
                            bytPass = bytPass Or &H40
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(1) = 0
                        Else
                            intSumCounts(1) += 1
                        End If
                    End If
                End If

                If (bytPass And &H20) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1250, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 8 * intNumSymbolsPerCar, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar3(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar3, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H20)
                    ElseIf intSumCounts(2) = 0 Then
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(2, m) = intToneMags(m)
                        Next
                        intSumCounts(2) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(2, m) = (intToneMagHistory(2, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(2, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        'Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1250, intNumSymbolsPerCar, bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 8 * intNumSymbolsPerCar, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar3(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar3, 0, bytCarData.Length)
                            bytPass = bytPass Or &H20
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(2) = 0
                        Else
                            intSumCounts(2) += 1
                        End If
                    End If
                End If

                If (bytPass And &H10) = 0 Then
                    Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 750, intNumSymbolsPerCar, bytRawData, intToneMags)
                    Array.Copy(intToneMags, 0, intToneMagsAllCar, 12 * intNumSymbolsPerCar, intToneMags.Length)
                    If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                        ReDim bytCorrectedDataCar4(bytCarData.Length - 1)
                        Array.Copy(bytCarData, 0, bytCorrectedDataCar4, 0, bytCarData.Length)
                        bytPass = (bytPass Or &H10)
                    ElseIf intSumCounts(3) = 0 Then
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(3, m) = intToneMags(m)
                        Next
                        intSumCounts(3) = 1
                    Else ' average the tone mags
                        For m As Integer = 0 To intToneMags.Length - 1
                            intToneMagHistory(3, m) = (intToneMagHistory(3, m) + intToneMags(m)) / 2
                            intToneMags(m) = intToneMagHistory(3, m)
                        Next
                        Decode1Car4FSKFromTones(bytRawData, intToneMags)
                        'Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 750, intNumSymbolsPerCar, bytRawData, intToneMags)
                        Array.Copy(intToneMags, 0, intToneMagsAllCar, 12 * intNumSymbolsPerCar, intToneMags.Length)
                        If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                            ReDim bytCorrectedDataCar4(bytCarData.Length - 1)
                            Array.Copy(bytCarData, 0, bytCorrectedDataCar4, 0, bytCarData.Length)
                            bytPass = bytPass Or &H10
                            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                            intSumCounts(3) = 0
                        Else
                            intSumCounts(3) += 1
                        End If
                    End If
                End If
        End Select
        objMain.Update4FSKConstellation(intToneMagsAllCar, intQuality)

        Select Case intNumCar
            Case 1
                ' Debug.WriteLine("[DemodDecode4FSKData] 1 Carrier, bytPass= " & Format(bytPass, "X"))
                If bytPass = &H80 Then
                    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
                    bytData = bytCorrectedDataCar1
                    intPassCounts += 1
                    Debug.WriteLine("4FSK Decode Success cnts = " & intPassCounts.ToString & "/" & intTestCounts.ToString)
                    Return True
                Else
                    If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
                    Debug.WriteLine("4FSK Decode Success cnts = " & intPassCounts.ToString & "/" & intTestCounts.ToString)
                End If
            Case 2
                'Debug.WriteLine("[DemodDecode4FSKData] 2 Carriers, bytPass= " & Format(bytPass, "X"))
                If bytPass = &HC0 Then
                    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
                    ReDim bytData(bytCorrectedDataCar1.Length + bytCorrectedDataCar2.Length - 1)
                    Array.Copy(bytCorrectedDataCar1, 0, bytData, 0, bytCorrectedDataCar1.Length)
                    Array.Copy(bytCorrectedDataCar2, 0, bytData, bytCorrectedDataCar1.Length, bytCorrectedDataCar2.Length)
                    Return True
                Else
                    If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
                End If
            Case 4
                'Debug.WriteLine("[DemodDecode4FSKData] 4 Carriers, bytPass= " & Format(bytPass, "X"))
                If bytPass = &HF0 Then
                    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
                    ReDim bytData(bytCorrectedDataCar1.Length + bytCorrectedDataCar2.Length + bytCorrectedDataCar3.Length + bytCorrectedDataCar4.Length - 1)
                    Array.Copy(bytCorrectedDataCar1, 0, bytData, 0, bytCorrectedDataCar1.Length)
                    Array.Copy(bytCorrectedDataCar2, 0, bytData, bytCorrectedDataCar1.Length, bytCorrectedDataCar2.Length)
                    Array.Copy(bytCorrectedDataCar3, 0, bytData, bytCorrectedDataCar1.Length + bytCorrectedDataCar2.Length, bytCorrectedDataCar3.Length)
                    Array.Copy(bytCorrectedDataCar4, 0, bytData, bytCorrectedDataCar1.Length + bytCorrectedDataCar2.Length + bytCorrectedDataCar3.Length, bytCorrectedDataCar4.Length)
                    Return True
                Else
                    If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
                End If
        End Select

        Return False
    End Function ' DemodDecode4FSKData

    ' Function to demod and decode all 4FSK High Baud rate (FM) Data frames single carrier 
    Private Function DemodDecode4FSKHighBaudData(bytFrameType As Byte, ByRef bytData() As Byte, ByRef intQuality As Integer) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For one carrier demodulate the High baud data yielding the tone magnitudes for that carrier, appending each data block
        '   Update the 4FSK quality display for the entire frame

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intNumSymbolsPerBlock As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytCarData() As Byte = Nothing
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytJustData() As Byte = Nothing
        Dim bytNoRS() As Byte = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim blnSuccess As Boolean = False
        ' These static variables used to accumuate correctly decoded data blocks.
        'Static bytLastFrameType As Byte = 0
        Static bytCorrectedDataBlock1() As Byte
        Static bytCorrectedDataBlock2() As Byte
        Static bytCorrectedDataBlock3() As Byte
        Static bytPass As Byte = 0
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        intNumSymbolsPerBlock = (1 + intDataLen \ 3 + 2 + intRSLen \ 3) * 4 '  byte count, data, 2 byte CRC, RS 
        Dim bytDataBlock(intDataLen \ 3 + 1 + 2 - 1) As Byte
        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedDataBlock1(intDataLen \ 3 - 1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataBlock2(intDataLen \ 3 - 1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataBlock3(intDataLen \ 3 - 1) ' Re initialize the bytCorrectedData array
        End If
        Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, (intDataLen + intRSLen + 9) * 4, bytRawData, intToneMags)
        If (bytPass And &H80) = 0 Then
            Array.Copy(bytRawData, 0, bytDataBlock, 0, bytDataBlock.Length)
            If CorrectRawDataWithRS(bytDataBlock, bytCorrectedDataBlock1, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H80)
            End If
        End If

        If (bytPass And &H40) = 0 Then
            Array.Copy(bytRawData, bytDataBlock.Length, bytDataBlock, 0, bytDataBlock.Length)
            If CorrectRawDataWithRS(bytDataBlock, bytCorrectedDataBlock1, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H40)
            End If
        End If

        If (bytPass And &H20) = 0 Then
            Array.Copy(bytRawData, 2 * bytDataBlock.Length, bytDataBlock, 0, bytDataBlock.Length)
            If CorrectRawDataWithRS(bytDataBlock, bytCorrectedDataBlock1, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H80)
            End If
        End If
        If bytPass = &HE0 Then
            ReDim bytData(bytCorrectedDataBlock1.Length + bytCorrectedDataBlock2.Length + bytCorrectedDataBlock3.Length - 1)
            Array.Copy(bytCorrectedDataBlock1, 0, bytData, 0, bytCorrectedDataBlock1.Length)
            Array.Copy(bytCorrectedDataBlock2, 0, bytData, bytCorrectedDataBlock1.Length, bytCorrectedDataBlock2.Length)
            Array.Copy(bytCorrectedDataBlock3, 0, bytData, bytCorrectedDataBlock1.Length + bytCorrectedDataBlock2.Length, bytCorrectedDataBlock3.Length)
            Return True
        Else
            Return False
        End If
    End Function ' DemodDecode4FSKHighBaudData

    ' Function to demod and decode all 16FSKData frames single carrier 
    Private Function Decode16FSKData(bytFrameType As Byte, ByRef bytData() As Byte, ByRef intQuality As Integer) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For each carrier demodulate the frame yielding the tone magnituds for that carrier, appending each  in carrier order
        '   Update the 16FSK quality display for the entire frame

        ' This version creates a rolling average of intToneMags on failed decodes and uses the average to try and recover (analog tone magnitude average)  
        '  Need to evaluate effectiveness of this vs complexity...it should however all still fit within this routine.
        ' Test 3/24 does avaraging is effective at low S/N with and without multipath

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytCarData() As Byte = Nothing
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytJustData() As Byte = Nothing
        Dim bytNoRS() As Byte = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim blnSuccess As Boolean = False
        ' These static variables used to accumuate correctly decoded carriers
        'Static bytLastFrameType As Byte = 0
        Static bytCorrectedDataCar1() As Byte
        Static bytPass As Byte = 0
        Static intToneMagHistory(0) As Int32
        Static intSumCounts(0) As Int32
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        intNumSymbolsPerCar = (1 + intDataLen + 2 + intRSLen) * 2 '  byte count, data, 2 byte CRC, RS 
        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedDataCar1(-1) ' Re initialize the bytCorrectedData array
            ReDim intToneMagHistory(16 * intNumSymbolsPerCar - 1) ' clear avg tones for analog Mem ARQ
            ReDim intSumCounts(3) ' zero out the counts in the averages
        End If

        'Select Case intNumCar
        ' Note car frequences go from high to low to accomodate sideband reversal after mixing 
        'Case 1
        If (bytPass And &H80) = 0 Then
            Demod1Car16FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
            'Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
            If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                bytPass = (bytPass Or &H80)
                If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            ElseIf intSumCounts(0) = 0 Then
                For m As Integer = 0 To intToneMags.Length - 1
                    intToneMagHistory(m) = intToneMags(m)
                Next
                intSumCounts(0) = 1
            Else ' average the tone mags
                For m As Integer = 0 To intToneMags.Length - 1
                    intToneMagHistory(m) = (intToneMagHistory(m) + intToneMags(m)) / 2
                    intToneMags(m) = intToneMagHistory(m)
                Next
                ' Demod1Car16FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
                Decode1Car16FSKFromTones(bytRawData, intToneMags)
                'Array.Copy(intToneMags, 0, intToneMagsAllCar, 0, intToneMags.Length)
                If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                    ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                    Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                    bytPass = bytPass Or &H80
                    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                    intSumCounts(0) = 0
                End If
            End If
        End If

        ' End Select
        If Not IsNothing(intToneMags) Then
            objMain.Update16FSKConstellation(intToneMags, intQuality)
        End If

        If bytPass = &H80 Then
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            bytData = bytCorrectedDataCar1
            Return True
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
        End If
        'End Select
        Return False
    End Function ' Decode16FSKData

    ' Function to demod and decode all 8FSKData frames single active carrier 
    Private Function Decode8FSKData(bytFrameType As Byte, ByRef bytData() As Byte, ByRef intQuality As Integer) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For each carrier demodulate the frame yielding the tone magnituds for that carrier, appending each  in carrier order
        '   Update the 8FSK quality display for the entire frame

        ' This version creates a rolling average of intToneMags on failed decodes and uses the average to try and recover (analog tone magnitude average)  
        ' Test 3/24 does avaraging is effective at low S/N with and without multipath

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytCarData() As Byte = Nothing
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytJustData() As Byte = Nothing
        Dim bytNoRS() As Byte = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim blnSuccess As Boolean = False
        ' These static variables used to accumuate correctly decoded carriers on simultaneous multicarrier 4FSK modes.
        Static bytCorrectedDataCar1() As Byte
        Static bytPass As Byte = 0
        Static intToneMagHistory(0) As Int32
        Static intSumCounts(0) As Int32
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        intNumSymbolsPerCar = (8 * (1 + intDataLen + 2 + intRSLen)) \ 3 '  byte count, data, 2 byte CRC, RS (3 bits/symbol
        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedDataCar1(-1) ' Re initialize the bytCorrectedData array
            ReDim intToneMagHistory(8 * intNumSymbolsPerCar - 1) ' clear avg tones for analog Mem ARQ
            ReDim intSumCounts(1) ' zero out the counts in the averages
        End If
        '
        ' Note car frequences go from high to low to accomodate sideband reversal after mixing 

        If (bytPass And &H80) = 0 Then
            Demod1Car8FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
            If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                bytPass = (bytPass Or &H80)
                If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            ElseIf intSumCounts(0) = 0 Then
                Array.Copy(intToneMags, 0, intToneMagHistory, 0, intToneMags.Length)
                intSumCounts(0) = 1
            Else ' average the tone mags
                For m As Integer = 0 To intToneMags.Length - 1
                    intToneMagHistory(m) = (intToneMagHistory(m) + intToneMags(m)) / 2
                    intToneMags(m) = intToneMagHistory(m)
                Next
                'Demod1Car8FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
                Decode1Car8FSKFromTones(bytRawData, intToneMags)
                If CorrectRawDataWithRS(bytRawData, bytCarData, intDataLen, intRSLen, bytFrameType) Then
                    ReDim bytCorrectedDataCar1(bytCarData.Length - 1)
                    Array.Copy(bytCarData, 0, bytCorrectedDataCar1, 0, bytCarData.Length)
                    bytPass = bytPass Or &H80
                    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKSummationDecodes += 1
                    intSumCounts(0) = 0
                End If
            End If
        End If
        If Not IsNothing(intToneMags) Then
            objMain.Update8FSKConstellation(intToneMags, intQuality)
        End If
        If bytPass = &H80 Then
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            bytData = bytCorrectedDataCar1
            Return True
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
        End If
        Return False
    End Function ' Decode8FSKData

    ' Function to demod and decode all 4FSK 600 bdData frames single carrier 
    Private Function Decode4FSK600bdData(bytFrameType As Byte, ByRef bytData() As Byte, ByRef intQuality As Integer) As Boolean
        'Concept:
        '   Look up the frame data for bytFrameType
        '   For each carrier demodulate the frame yielding the tone magnituds for that carrier, appending each  in carrier order
        '   Update the 4FSK quality display for the entire frame

        ' This version creates a rolling average of intToneMags on failed decodes and uses the average to try and recover (analog tone magnitude average)  
        '  Need to evaluate effectiveness of this vs complexity...it should however all still fit within this routine.
        ' Test 3/24 does avaraging is effective at low S/N with and without multipath

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen, intNumSymbolsPerCar As Integer
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytCarData() As Byte = Nothing
        Dim bytQualThresh As Byte
        Dim intToneMags() As Int32 = Nothing
        Dim bytJustData() As Byte = Nothing
        Dim bytNoRS() As Byte = Nothing
        Dim bytRawData() As Byte = Nothing
        Dim bytBlockData() As Byte
        Dim blnSuccess As Boolean = False
        ' These static variables used to accumuate correctly decoded carriers on simultaneous multicarrier 4FSK modes.
        'Static bytLastFrameType As Byte = 0
        Static bytCorrectedDataBlock1() As Byte
        Static bytCorrectedDataBlock2() As Byte
        Static bytCorrectedDataBlock3() As Byte
        Static bytPass As Byte = 0
        
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        If intDataLen <= 200 Then
            intNumSymbolsPerCar = (1 + intDataLen + 2 + intRSLen) * 4 '  byte count, data, 2 byte CRC, RS 
        Else
            intNumSymbolsPerCar = (1 + intDataLen \ 3 + 2 + intRSLen \ 3) * 12 '  byte count, data, 2 byte CRC, RS 
        End If

        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedDataBlock1(-1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataBlock2(-1) ' Re initialize the bytCorrectedData array
            ReDim bytCorrectedDataBlock3(-1) ' Re initialize the bytCorrectedData array
        End If
        Demod1Car4FSK600bd(intMFSReadPtr, intFilteredMixedSamples, intBaud, 1500, intNumSymbolsPerCar, bytRawData, intToneMags)
        If intDataLen > 200 Then
            ReDim bytBlockData(bytRawData.Length \ 3 - 1)
        Else
            ReDim bytBlockData(bytRawData.Length - 1)
        End If
        If (bytPass And &H80) = 0 And intDataLen <= 200 Then
            If CorrectRawDataWithRS(bytRawData, bytCorrectedDataBlock1, intDataLen, intRSLen, bytFrameType) Then
                bytPass = &H80
            End If
        End If
        If ((bytPass And &H80) = 0) And intDataLen > 200 Then
            Array.Copy(bytRawData, 0, bytBlockData, 0, bytBlockData.Length)
            If CorrectRawDataWithRS(bytBlockData, bytCorrectedDataBlock1, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H80)
            End If
        End If
        If ((bytPass And &H40) = 0) And intDataLen > 200 Then
            Array.Copy(bytRawData, bytBlockData.Length, bytBlockData, 0, bytBlockData.Length)
            If CorrectRawDataWithRS(bytBlockData, bytCorrectedDataBlock2, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H40)
            End If
        End If
        If ((bytPass And &H20) = 0) And intDataLen > 200 Then
            Array.Copy(bytRawData, 2 * bytBlockData.Length, bytBlockData, 0, bytBlockData.Length)
            If CorrectRawDataWithRS(bytBlockData, bytCorrectedDataBlock3, intDataLen \ 3, intRSLen \ 3, bytFrameType) Then
                bytPass = (bytPass Or &H20)
            End If
        End If
        If Not IsNothing(intToneMags) Then
            objMain.Update4FSKConstellation(intToneMags, intQuality)
        End If
        If bytPass = &H80 And intDataLen <= 200 Then
            If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            bytData = bytCorrectedDataBlock1
            Return True
        ElseIf bytPass = &HE0 Then
            ReDim bytData(bytCorrectedDataBlock1.Length + bytCorrectedDataBlock2.Length + bytCorrectedDataBlock3.Length - 1)
            Array.Copy(bytCorrectedDataBlock1, 0, bytData, 0, bytCorrectedDataBlock1.Length)
            Array.Copy(bytCorrectedDataBlock2, 0, bytData, bytCorrectedDataBlock1.Length, bytCorrectedDataBlock2.Length)
            Array.Copy(bytCorrectedDataBlock3, 0, bytData, bytCorrectedDataBlock1.Length + bytCorrectedDataBlock2.Length, bytCorrectedDataBlock3.Length)
            If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
            Return True
        End If
        Return False
    End Function ' Decode4FSK600bdData

    ' Function to Correct Raw demodulated data with Reed Solomon FEC 
    Private Function CorrectRawDataWithRS(ByRef bytRawData() As Byte, ByRef bytCorrectedData() As Byte, intDataLen As Int32, intRSLen As Int32, Optional bytFrameType As Byte = 0) As Boolean
        Dim bytNoRS(1 + intDataLen + 2 - 1) As Byte  ' 1 byte byte Count, Data, 2 byte CRC 
        Array.Copy(bytRawData, 0, bytNoRS, 0, bytNoRS.Length)

        If CheckCRC16FrameType(bytNoRS, bytFrameType) Then ' No RS correction needed
            ReDim bytCorrectedData(bytNoRS(0) - 1)
            Array.Copy(bytNoRS, 1, bytCorrectedData, 0, bytNoRS(0))
            Debug.WriteLine("[DemodDecode4FSKData] OK without RS")
            Return True
        End If
        ' Try correcting with RS Parity
        objRS8.MaxCorrections = intRSLen \ 2
        bytNoRS = objRS8.RSDecode(bytRawData)

        If bytNoRS.Length = (intDataLen + 3) AndAlso CheckCRC16FrameType(bytNoRS, bytFrameType) Then ' RS correction successful 
            ReDim bytCorrectedData(bytNoRS(0) - 1)
            Array.Copy(bytNoRS, 1, bytCorrectedData, 0, bytNoRS(0))

            ' test code just to determine how many corrections were applied  ...later remove
            Dim intFailedByteCnt As Int32 = 0
            For j As Integer = 0 To bytNoRS.Length - 1
                If bytNoRS(j) <> bytRawData(j) Then intFailedByteCnt += 1
            Next
            Debug.WriteLine("[DemodDecode4FSKData] OK with RS & " & intFailedByteCnt.ToString & " corrections")
            ' End of test code
            Return True
        End If
        ReDim bytCorrectedData(intDataLen - 1) ' 
        Array.Copy(bytRawData, 1, bytCorrectedData, 0, bytCorrectedData.Length) 'return uncorrected data without byte count or RS Parity
        Return False
    End Function '  CorrectRawDataWithRS

    ' Function to Demod FrameType4FSK
    Private Function DemodFrameType4FSK(intPtr As Int32, ByRef intSamples() As Int32, ByRef intToneMags() As Int32) As Boolean
        ReDim intToneMags(39) ' 10 symbols, 4 tones/symbol
        Dim dblReal, dblImag As Double

        If (intSamples.Length - intPtr) < 2400 Then Return False
        For i As Integer = 0 To 9
            GoertzelRealImag(intSamples, intPtr, 240, 1575 / 50, dblReal, dblImag)
            intToneMags(4 * i) = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, intPtr, 240, 1525 / 50, dblReal, dblImag)
            intToneMags(1 + 4 * i) = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, intPtr, 240, 1475 / 50, dblReal, dblImag)
            intToneMags(2 + 4 * i) = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, intPtr, 240, 1425 / 50, dblReal, dblImag)
            intToneMags(3 + 4 * i) = (dblReal ^ 2) + (dblImag ^ 2)
            intPtr += 240
        Next i
        Return True
    End Function

    ' Function to demodulate one carrier for all low baud rate 4FSK frame types
    Private Function Demod1Car4FSK(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCenterFreq As Int32, intNumOfSymbols As Int32, ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
        ' intPtr should be pointing to the approximate start of the first data symbol  
        ' Updates bytData() with demodulated bytes
        ' Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

        Dim intSampPerSymbol As Int32 = CInt(12000 / intBaud)
        Dim dblReal, dblImag As Double
        Dim intSearchFreq As Integer
        Dim dblMagSum As Double = 0
        Dim dblMag(3) As Double ' The magnitude for each of the 4FSK frequency bins
        Dim bytSym As Byte
        Static bytSymHistory(2) As Byte

        If (intSamples.Length - intPtr) < (intSampPerSymbol * intNumOfSymbols) Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If (intNumOfSymbols Mod 4) <> 0 Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Number of Symbols not a multiple of 4: " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
        ReDim intToneMags(4 * intNumOfSymbols - 1)
        ReDim bytData(intNumOfSymbols \ 4 - 1)

        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            intSearchFreq = CInt(intCenterFreq + 1.5 * intBaud) ' the highest freq (equiv to lowest sent freq because of sideband reversal)
            For j As Integer = 0 To 3 ' for each 4FSK symbol (2 bits) in a byte
                dblMagSum = 0
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, intSearchFreq / intBaud, dblReal, dblImag)
                dblMag(0) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(0)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - intBaud) / intBaud, dblReal, dblImag)
                dblMag(1) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(1)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - 2 * intBaud) / intBaud, dblReal, dblImag)
                dblMag(2) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(2)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - 3 * intBaud) / intBaud, dblReal, dblImag)
                dblMag(3) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(3)
                If dblMag(0) > dblMag(1) And dblMag(0) > dblMag(2) And dblMag(0) > dblMag(3) Then
                    bytSym = 0
                ElseIf dblMag(1) > dblMag(0) And dblMag(1) > dblMag(2) And dblMag(1) > dblMag(3) Then
                    bytSym = 1
                ElseIf dblMag(2) > dblMag(0) And dblMag(2) > dblMag(1) And dblMag(2) > dblMag(3) Then
                    bytSym = 2
                Else
                    bytSym = 3
                End If
                bytData(i) = (bytData(i) << 2) + bytSym
                intToneMags(16 * i + 4 * j) = dblMag(0)
                intToneMags(16 * i + 4 * j + 1) = dblMag(1)
                intToneMags(16 * i + 4 * j + 2) = dblMag(2)
                intToneMags(16 * i + 4 * j + 3) = dblMag(3)
                bytSymHistory(0) = bytSymHistory(1)
                bytSymHistory(1) = bytSymHistory(2)
                bytSymHistory(2) = bytSym
                If (bytSymHistory(0) <> bytSymHistory(1)) And (bytSymHistory(1) <> bytSymHistory(2)) Then ' only track when adjacent symbols are different (statistically about 56% of the time)
                    ' this should allow tracking over 2000 ppm sampling rate error
                    Track1Car4FSK(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory)
                End If
                intPtr += intSampPerSymbol ' advance the pointer one symbol
            Next j
        Next i
        Return True
    End Function  '  Demod1Car4FSK

    ' Function to decode one carrier from tones (used to decode from Averaged intToneMags) 
    Private Function Decode1Car4FSKFromTones(ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Decodes intToneMags() to an array of bytes   
        ' Updates bytData() with decoded 

        Dim bytSym As Byte
        Dim intIndex As Int32
        ReDim bytData(intToneMags.Length \ 16 - 1)

        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            intIndex = 16 * i
            For j As Integer = 0 To 3 ' for each 4FSK symbol (2 bits) in a byte
                If intToneMags(intIndex) > intToneMags(intIndex + 1) And intToneMags(intIndex) > intToneMags(intIndex + 2) And intToneMags(intIndex) > intToneMags(intIndex + 3) Then
                    bytSym = 0
                ElseIf intToneMags(intIndex + 1) > intToneMags(intIndex) And intToneMags(intIndex + 1) > intToneMags(intIndex + 2) And intToneMags(intIndex + 1) > intToneMags(intIndex + 3) Then
                    bytSym = 1
                ElseIf intToneMags(intIndex + 2) > intToneMags(intIndex) And intToneMags(intIndex + 2) > intToneMags(intIndex + 1) And intToneMags(intIndex + 2) > intToneMags(intIndex + 3) Then
                    bytSym = 2
                Else
                    bytSym = 3
                End If
                bytData(i) = (bytData(i) << 2) + bytSym
                intIndex += 4
            Next j
        Next i
        Return True
    End Function  '  Decode1Car4FSKFromTones

    ' Function to decode one carrier from tones (used to decode from Averaged intToneMags) 
    Private Function Decode1Car8FSKFromTones(ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Decodes intToneMags() to an array of bytes   
        ' Updates bytData() with decoded 

        Dim bytSym As Byte
        Dim intThreeBytes As Int32
        ReDim bytData(3 * intToneMags.Length \ 64 - 1)
        Dim intMaxMag As Int32
        For i As Integer = 0 To (bytData.Length \ 3) - 1   ' For each group of 3 bytes data byte
            intThreeBytes = 0
            For j As Integer = 0 To 7 ' for each group of 8 symbols (24 bits) 
                intMaxMag = 0
                For k As Integer = 0 To 7 ' for each of 8 possible tones per symbol
                    If intToneMags((i * 64) + 8 * j + k) > intMaxMag Then
                        intMaxMag = intToneMags((i * 64) + 8 * j + k)
                        bytSym = k
                    End If
                Next k
                intThreeBytes = (intThreeBytes << 3) + bytSym
            Next j
            bytData(3 * i) = (intThreeBytes And &HFF0000) >> 16
            bytData(3 * i + 1) = (intThreeBytes And &HFF00) >> 8
            bytData(3 * i + 2) = (intThreeBytes And &HFF)
        Next i
        Return True
    End Function  '  Decode1Car8FSKFromTones

    ' Function to decode one carrier from tones (used to decode from Averaged intToneMags) 
    Private Function Decode1Car16FSKFromTones(ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Decodes intToneMags() to an array of bytes   
        ' Updates bytData() with decoded tones 

        Dim bytSym As Byte
        Dim intMaxMag As Int32
        ReDim bytData(intToneMags.Length \ 32 - 1)
        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            For j As Integer = 0 To 1 ' for each 16FSK symbol (4 bits) in a byte
                intMaxMag = 0
                For k As Integer = 0 To 15
                    If intToneMags(i * 32 + 16 * j + k) > intMaxMag Then
                        intMaxMag = intToneMags(i * 32 + 16 * j + k)
                        bytSym = k
                    End If
                Next k
                bytData(i) = (bytData(i) << 4) + bytSym
            Next j
        Next i
        Return True
    End Function  '  Decode1Car16FSKFromTones

    ' Function to demodulate one carrier for all 16FSK frame types
    Private Function Demod1Car16FSK(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCenterFreq As Int32, intNumOfSymbols As Int32, ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
        ' intPtr should be pointing to the approximate start of the first data symbol  
        ' Updates bytData() with demodulated bytes
        ' Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

        Dim intSampPerSymbol As Int32 = CInt(12000 / intBaud)
        Dim dblReal, dblImag As Double
        Dim intSearchFreq As Integer
        Dim dblMagSum As Double = 0
        Dim dblMag(15) As Double ' The magnitude for each of the 16FSK frequency bins
        Dim bytSym As Byte
        Dim intMaxMag As Int32
        Static bytSymHistory(2) As Byte

        If (intSamples.Length - intPtr) < (intSampPerSymbol * intNumOfSymbols) Then
            Logs.Exception("[DemodulateDecode.Demod1Car16FSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If (intNumOfSymbols Mod 2) <> 0 Then
            Logs.Exception("[DemodulateDecode.Demod1Car16FSK] Number of Symbols not a multiple of 2: " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
        ReDim intToneMags(16 * intNumOfSymbols - 1)
        ReDim bytData(intNumOfSymbols \ 2 - 1)

        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            intSearchFreq = CInt(intCenterFreq + 7.5 * intBaud) ' the highest freq (equiv to lowest sent freq because of sideband reversal)
            For j As Integer = 0 To 1 ' for each 16FSK symbol (4 bits) in a byte
                dblMagSum = 0
                intMaxMag = 0
                For k As Integer = 0 To 15
                    GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - k * intBaud) / intBaud, dblReal, dblImag)
                    intToneMags(i * 32 + 16 * j + k) = (dblReal ^ 2) + (dblImag ^ 2)
                    dblMagSum += intToneMags(i * 32 + 16 * j + k)
                    If intToneMags(i * 32 + 16 * j + k) > intMaxMag Then
                        intMaxMag = intToneMags(i * 32 + 16 * j + k)
                        bytSym = k
                    End If
                Next k

                bytData(i) = (bytData(i) << 4) + bytSym
                bytSymHistory(0) = bytSymHistory(1)
                bytSymHistory(1) = bytSymHistory(2)
                bytSymHistory(2) = bytSym
                If (bytSymHistory(0) <> bytSymHistory(1)) And (bytSymHistory(1) <> bytSymHistory(2)) Then ' only track when adjacent symbols are different (statistically about 56% of the time)
                    ' this should allow tracking over 2000 ppm sampling rate error
                    Track1Car4FSK(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory)
                End If
                intPtr += intSampPerSymbol ' advance the pointer one symbol
            Next j
        Next i
        Return True
    End Function  '  Demod1Car16FSK

    ' Function to demodulate one carrier for all 8FSK frame types
    Private Function Demod1Car8FSK(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCenterFreq As Int32, intNumOfSymbols As Int32, ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
        ' intPtr should be pointing to the approximate start of the first data symbol  
        ' Updates bytData() with demodulated bytes
        ' Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

        Dim intSampPerSymbol As Int32 = CInt(12000 / intBaud)
        Dim dblReal, dblImag As Double
        Dim intSearchFreq As Integer
        Dim dblMagSum As Double = 0
        Dim bytSym As Byte
        Dim intMaxMag, intThreeBytes As Int32
        Static bytSymHistory(2) As Byte

        If (intSamples.Length - intPtr) < (intSampPerSymbol * intNumOfSymbols) Then
            Logs.Exception("[DemodulateDecode.Demod1Car8FSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If ((intNumOfSymbols * 3) Mod 8) <> 0 Then
            Logs.Exception("[DemodulateDecode.Demod1Car8FSK] Number of Symbols * 3 not a multiple of 8: " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
        ReDim intToneMags(8 * intNumOfSymbols - 1)
        ReDim bytData((intNumOfSymbols * 3) \ 8 - 1)

        For i As Integer = 0 To (bytData.Length \ 3) - 1   ' For each group of 3 bytes data byte
            intThreeBytes = 0
            For j As Integer = 0 To 7 ' for each group of 8 symbols (24 bits) 
                dblMagSum = 0
                intMaxMag = 0
                intSearchFreq = CInt(intCenterFreq + 3.5 * intBaud) ' the highest freq (equiv to lowest sent freq because of sideband reversal)
                For k As Integer = 0 To 7 ' for each of 8 possible tones per symbol
                    GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - k * intBaud) / intBaud, dblReal, dblImag)
                    intToneMags((i * 64) + 8 * j + k) = (dblReal ^ 2) + (dblImag ^ 2)
                    dblMagSum += intToneMags((i * 64) \ 3 + 8 * j + k)
                    If intToneMags((i * 64) + 8 * j + k) > intMaxMag Then
                        intMaxMag = intToneMags((i * 64) + 8 * j + k)
                        bytSym = k
                    End If
                Next k
                intThreeBytes = (intThreeBytes << 3) + bytSym
                bytSymHistory(0) = bytSymHistory(1)
                bytSymHistory(1) = bytSymHistory(2)
                bytSymHistory(2) = bytSym
                'If (bytSymHistory(0) <> bytSymHistory(1)) And (bytSymHistory(1) <> bytSymHistory(2)) Then ' only track when adjacent symbols are different (statistically about 56% of the time)
                '    ' this should allow tracking over 2000 ppm sampling rate error
                '    Track1Car4FSK(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory)
                'End If
                intPtr += intSampPerSymbol ' advance the pointer one symbol
            Next j
            bytData(3 * i) = (intThreeBytes And &HFF0000) >> 16
            bytData(3 * i + 1) = (intThreeBytes And &HFF00) >> 8
            bytData(3 * i + 2) = (intThreeBytes And &HFF)
        Next i
        Return True
    End Function  '  Demod1Car8FSK

    ' Function to demodulate one carrier for all 16FSK frame types
    Private Function Demod1Car4FSK600bd(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCenterFreq As Int32, intNumOfSymbols As Int32, ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
        ' intPtr should be pointing to the approximate start of the first data symbol  
        ' Updates bytData() with demodulated bytes
        ' Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

        Dim intSampPerSymbol As Int32 = CInt(12000 / intBaud)
        Dim dblReal, dblImag As Double
        Dim intSearchFreq As Integer
        Dim dblMagSum As Double = 0
        Dim dblMag(3) As Double ' The magnitude for each of the 16FSK frequency bins
        Dim bytSym As Byte
        Dim intMaxMag As Int32
        Static bytSymHistory(2) As Byte

        If (intSamples.Length - intPtr) < (intSampPerSymbol * intNumOfSymbols) Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK600bd] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If (intNumOfSymbols Mod 4) <> 0 Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK600bd] Number of Symbols not a multiple of 4: " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
        ReDim intToneMags(4 * intNumOfSymbols - 1)
        ReDim bytData(intNumOfSymbols \ 4 - 1)
        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            intSearchFreq = CInt(intCenterFreq + 1.5 * intBaud) ' the highest freq (equiv to lowest sent freq because of sideband reversal)
            For j As Integer = 0 To 3 ' for each 4FSK symbol (2 bits) in a byte
                dblMagSum = 0
                intMaxMag = 0
                For k As Integer = 0 To 3
                    GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - k * intBaud) / intBaud, dblReal, dblImag)
                    intToneMags(i * 16 + 4 * j + k) = (dblReal ^ 2) + (dblImag ^ 2)
                    dblMagSum += intToneMags(i * 16 + 4 * j + k)
                    If intToneMags(i * 16 + 4 * j + k) > intMaxMag Then
                        intMaxMag = intToneMags(i * 16 + 4 * j + k)
                        bytSym = k
                    End If
                Next k
                bytData(i) = (bytData(i) << 2) + bytSym
                bytSymHistory(0) = bytSymHistory(1)
                bytSymHistory(1) = bytSymHistory(2)
                bytSymHistory(2) = bytSym
                If (bytSymHistory(0) <> bytSymHistory(1)) And (bytSymHistory(1) <> bytSymHistory(2)) Then ' only track when adjacent symbols are different (statistically about 56% of the time)
                    ' this should allow tracking over 2000 ppm sampling rate error
                    Track1Car4FSK600bd(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory)
                End If
                intPtr += intSampPerSymbol ' advance the pointer one symbol
            Next j
        Next i
        Return True
    End Function  '  Demod1Car4FSK600bd

    ' Subroutine to track 1 carrier 4FSK. Used for both single and multiple simultaneous carrier 4FSK modes.
    Private Sub Track1Car4FSK(ByRef intSamples() As Int32, ByRef intPtr As Int32, intSampPerSymbol As Int32, intSearchFreq As Int32, intBaud As Int32, bytSymHistory() As Byte)
        'look at magnitude of the tone for bytHistory(1)  2 sample2 earlier and 2 samples later.  and pick the maximum adjusting intPtr + or - 1
        ' this seems to work fine on test Mar 16, 2015. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

        Dim dblReal, dblImag, dblMagEarly, dblMag, dblMagLate As Double
        Dim dblBinToSearch As Double = (intSearchFreq - (intBaud * bytSymHistory(1))) / intBaud ' select the 2nd last symbol for magnitude comparison
        Try
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol - 2), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagEarly = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMag = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol + 2), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagLate = (dblReal ^ 2) + (dblImag ^ 2)
            If dblMagEarly > dblMag And dblMagEarly > dblMagLate Then
                intPtr -= 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking -= 1
            ElseIf dblMagLate > dblMag And dblMagLate > dblMagEarly Then
                intPtr += 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking += 1
            End If
        Catch
            ' should handle any pointer boundary issues with no corrections
        End Try
    End Sub '  Track1Car4FSK

    Private Sub Track1Car4FSK600bd(ByRef intSamples() As Int32, ByRef intPtr As Int32, intSampPerSymbol As Int32, intSearchFreq As Int32, intBaud As Int32, bytSymHistory() As Byte)
        'look at magnitude of the tone for bytHistory(1)  2 sample2 earlier and 2 samples later.  and pick the maximum adjusting intPtr + or - 1
        ' this seems to work fine on test Mar 16, 2015. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

        Dim dblReal, dblImag, dblMagEarly, dblMag, dblMagLate As Double
        Dim dblBinToSearch As Double = (intSearchFreq - (intBaud * bytSymHistory(1))) / intBaud ' select the 2nd last symbol for magnitude comparison
        Try
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol - 1), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagEarly = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMag = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol + 1), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagLate = (dblReal ^ 2) + (dblImag ^ 2)
            If dblMagEarly > dblMag And dblMagEarly > dblMagLate Then
                intPtr -= 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking -= 1
            ElseIf dblMagLate > dblMag And dblMagLate > dblMagEarly Then
                intPtr += 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking += 1
            End If
        Catch
            ' should handle any pointer boundary issues with no corrections
        End Try
    End Sub '  Track1Car4FSK600bd

    ' A function to decode all PSK data frames, all number of carriers, all PSK modes
    Private Function DecodePSKData(bytFrameType As Byte, ByRef intPhases() As Int16, ByRef intMags() As Int16, ByRef bytData() As Byte) As Boolean
        ' phases have all been rotation corrected. 
        ' This version creates and average intPhases on failed decodes and uses the average to try and recover (analog phase average of millirads)  
        '  Need to evaluate effectiveness of this vs complexity...it should however all still fit within this routine.
        ' Test 3/24 does avaraging is effective at low S/N with and without multipath
        '  Modified 6/30/2015 to use magnitude weighted averaging in the averaging of repeated frames.  A measureable improvement over simple 
        '  averaging of before. 
        Dim intNumCar, intPSKMode, intBaud, intDataLen, intRSLen, intNumSymbolsPerCar As Integer
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim intCarPhases(0) As Int16, intCarMags(0) As Int16
        Dim bytCarMask As Byte = &H80
        Dim bytQualThresh As Byte = 100
        Dim bytNoRS(-1) As Byte
        Dim bytCorrectedCarData(-1) As Byte

        ' These static variables are used to accumuate correctly decoded carriers after repeats on multicarrier modes.
        'Static bytLastFrameType As Byte = 0
        Static bytCorrectedData0(-1) As Byte ' an array of correctedData one for carrier 0
        Static bytCorrectedData1(-1) As Byte ' an array of correctedData one for carrier 1
        Static bytCorrectedData2(-1) As Byte ' an array of correctedData one for carrier 2
        Static bytCorrectedData3(-1) As Byte ' an array of correctedData one for carrier 3
        Static bytCorrectedData4(-1) As Byte ' an array of correctedData one for carrier 4
        Static bytCorrectedData5(-1) As Byte ' an array of correctedData one for carrier 5
        Static bytCorrectedData6(-1) As Byte ' an array of correctedData one for carrier 6
        Static bytCorrectedData7(-1) As Byte ' an array of correctedData one for carrier 7

        Static bytPass As Byte = 0
        Static intCarPhaseAvg(7, 0) As Int16 ' array to accumulate phases for averaging (Memory ARQ)
        Static intCarMagAvg(7, 0) As Int16 ' array to accumulate mags for averaging (Memory ARQ) 
        Static intSumCounts(7) As Int32
        DecodePSKData = False ' Preset for failure 
        'Look up the details by Frame type
        objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        intPSKMode = CInt(strMod.Substring(0, 1))
        intNumSymbolsPerCar = intPhases.Length \ intNumCar
        objRS8.MaxCorrections = intRSLen \ 2
        Dim bytRawData((intPhases.Length \ 4) \ intNumCar - 1) As Byte ' for 4PSK modes
        If intPSKMode = 8 Then
            ReDim bytRawData((intPhases.Length * 3 \ 8) \ intNumCar - 1) ' Redim for 3 bits per phase value for 8PSK modes
        End If
        ReDim intCarPhases((intPhases.Length \ intNumCar) - 1)
        ReDim intCarMags((intMags.Length \ intNumCar) - 1)
        ReDim bytNoRS(1 + intDataLen + 2 - 1) ' 1 byte byte Count, Data, 2 byte CRC 
        ' initialise the static data if Accumulating Stats or on a change in frame type (e.g. even to odd, different # car, etc)
        If bytLastDataFrameType <> bytFrameType Then
            bytPass = 0 ' clear the passing flags (prior successful decodes of bytFrameType)
            bytLastDataFrameType = bytFrameType
            ReDim bytCorrectedData0(-1) ' Re initialize the bytCorrectedData array for carrier 0
            ReDim bytCorrectedData1(-1) ' Re initialize the bytCorrectedData array for carrier 1
            ReDim bytCorrectedData2(-1) ' Re initialize the bytCorrectedData array for carrier 2
            ReDim bytCorrectedData3(-1) ' Re initialize the bytCorrectedData array for carrier 3
            ReDim bytCorrectedData4(-1) ' Re initialize the bytCorrectedData array for carrier 4
            ReDim bytCorrectedData5(-1) ' Re initialize the bytCorrectedData array for carrier 5
            ReDim bytCorrectedData6(-1) ' Re initialize the bytCorrectedData array for carrier 6
            ReDim bytCorrectedData7(-1) ' Re initialize the bytCorrectedData array for carrier 7
            ReDim intCarPhaseAvg(7, intCarPhases.Length - 1) ' clear avg phase of multiple frames used for analog Mem ARQ
            ReDim intCarMagAvg(7, intCarPhases.Length - 1) ' clear avg Mags of mulitple frames used for Analog Mem ARQ (Added Rev 0.3.5.1)
            ReDim intSumCounts(7) ' zero out the counts in the averages
        End If
        ' Across all carriers
        For i As Integer = 0 To intNumCar - 1
            If (bytCarMask And bytPass) = 0 Then ' decode only those carriers not yet correctly decoded.   
                Array.Copy(intPhases, i * intCarPhases.Length, intCarPhases, 0, intCarPhases.Length)
                Array.Copy(intMags, i * intCarMags.Length, intCarMags, 0, intCarMags.Length)
                If Decode1CarPSK(intCarPhases, intPSKMode, bytRawData, bytCorrectedCarData, intRSLen, bytFrameType) Then
                    Select Case i
                        Case 0 : ReDim bytCorrectedData0(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData0, bytCorrectedCarData.Length)
                        Case 1 : ReDim bytCorrectedData1(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData1, bytCorrectedCarData.Length)
                        Case 2 : ReDim bytCorrectedData2(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData2, bytCorrectedCarData.Length)
                        Case 3 : ReDim bytCorrectedData3(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData3, bytCorrectedCarData.Length)
                        Case 4 : ReDim bytCorrectedData4(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData4, bytCorrectedCarData.Length)
                        Case 5 : ReDim bytCorrectedData5(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData5, bytCorrectedCarData.Length)
                        Case 6 : ReDim bytCorrectedData6(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData6, bytCorrectedCarData.Length)
                        Case 7 : ReDim bytCorrectedData7(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData7, bytCorrectedCarData.Length)
                    End Select
                    If MCB.DebugLog Then Logs.WriteDebug("[DecodePSKData] carrier " & i.ToString & " pass on initial decode") ' Added 0.3.5.1
                    bytPass = bytCarMask Or bytPass
                    intSumCounts(i) = 0
                Else
                    ' failure in correction so just add to average and try to decode
                    If intSumCounts(i) = 0 Then ' initialize Sum counts Phase average and Mag Average 
                        For m As Integer = 0 To intCarPhases.Length - 1
                            intCarPhaseAvg(i, m) = intCarPhases(m)
                            intCarMagAvg(i, m) = intCarMags(m)
                        Next
                        intSumCounts(i) = 1
                        ' Debug.WriteLine("[DecodePSKData2] Reset sum for carrier " & i.ToString & " sum count = " & intSumCounts(i).ToString)
                    Else
                        For k As Integer = 0 To intCarPhases.Length - 1
                            intCarPhaseAvg(i, k) = WeightedAngleAvg(intCarPhaseAvg(i, k), intCarPhases(k), intCarMagAvg(i, k), intCarMags(k)) ' Modified 0.3.5.1
                            intCarPhases(k) = intCarPhaseAvg(i, k)
                            intCarMagAvg(i, k) = CShort((intCarMagAvg(i, k) ^ 2 + intCarMags(k) ^ 2) / (intCarMagAvg(i, k) + intCarMags(k))) ' Added 0.3.5.1
                        Next k
                        intSumCounts(i) += 1
                        ' now try to decode based on the WeightedAveragePhases
                        If Decode1CarPSK(intCarPhases, intPSKMode, bytRawData, bytCorrectedCarData, intRSLen, bytFrameType) Then
                            Select Case i
                                Case 0 : ReDim bytCorrectedData0(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData0, bytCorrectedCarData.Length)
                                Case 1 : ReDim bytCorrectedData1(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData1, bytCorrectedCarData.Length)
                                Case 2 : ReDim bytCorrectedData2(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData2, bytCorrectedCarData.Length)
                                Case 3 : ReDim bytCorrectedData3(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData3, bytCorrectedCarData.Length)
                                Case 4 : ReDim bytCorrectedData4(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData4, bytCorrectedCarData.Length)
                                Case 5 : ReDim bytCorrectedData5(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData5, bytCorrectedCarData.Length)
                                Case 6 : ReDim bytCorrectedData6(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData6, bytCorrectedCarData.Length)
                                Case 7 : ReDim bytCorrectedData7(bytCorrectedCarData.Length - 1) : Array.Copy(bytCorrectedCarData, bytCorrectedData7, bytCorrectedCarData.Length)
                            End Select
                            bytPass = bytCarMask Or bytPass
                            If MCB.AccumulateStats Then stcTuningStats.intGoodPSKSummationDecodes += 1
                            If MCB.DebugLog Then Logs.WriteDebug("[DecodePSKData] carrier " & i.ToString & " pass after " & intSumCounts(i).ToString & " sums")
                        End If
                    End If
                End If
            End If
            bytCarMask = bytCarMask >> 1 ' Shift the mask
        Next i '
        Select Case intNumCar
            Case 1
                DecodePSKData = (bytPass = &H80)
            Case 2
                DecodePSKData = (bytPass = &HC0)
            Case 4
                DecodePSKData = (bytPass = &HF0)
            Case 8
                DecodePSKData = (bytPass = &HFF)
        End Select
        If DecodePSKData Then
            Dim intPtr As Integer = 0
            ReDim bytData(-1)
            For i As Integer = 0 To intNumCar - 1
                Select Case i
                    Case 0 : ReDim Preserve bytData(intPtr + bytCorrectedData0.Length - 1) : Array.Copy(bytCorrectedData0, 0, bytData, intPtr, bytCorrectedData0.Length) : intPtr += bytCorrectedData0.Length
                    Case 1 : ReDim Preserve bytData(intPtr + bytCorrectedData1.Length - 1) : Array.Copy(bytCorrectedData1, 0, bytData, intPtr, bytCorrectedData1.Length) : intPtr += bytCorrectedData1.Length
                    Case 2 : ReDim Preserve bytData(intPtr + bytCorrectedData2.Length - 1) : Array.Copy(bytCorrectedData2, 0, bytData, intPtr, bytCorrectedData2.Length) : intPtr += bytCorrectedData2.Length '<<Corrected 0.3.2.3
                    Case 3 : ReDim Preserve bytData(intPtr + bytCorrectedData3.Length - 1) : Array.Copy(bytCorrectedData3, 0, bytData, intPtr, bytCorrectedData3.Length) : intPtr += bytCorrectedData3.Length '<<Corrected 0.3.2.3
                    Case 4 : ReDim Preserve bytData(intPtr + bytCorrectedData4.Length - 1) : Array.Copy(bytCorrectedData4, 0, bytData, intPtr, bytCorrectedData4.Length) : intPtr += bytCorrectedData4.Length '<<Corrected 0.3.2.3
                    Case 5 : ReDim Preserve bytData(intPtr + bytCorrectedData5.Length - 1) : Array.Copy(bytCorrectedData5, 0, bytData, intPtr, bytCorrectedData5.Length) : intPtr += bytCorrectedData5.Length '<<Corrected 0.3.2.3
                    Case 6 : ReDim Preserve bytData(intPtr + bytCorrectedData6.Length - 1) : Array.Copy(bytCorrectedData6, 0, bytData, intPtr, bytCorrectedData6.Length) : intPtr += bytCorrectedData6.Length '<<Corrected 0.3.2.3
                    Case 7 : ReDim Preserve bytData(intPtr + bytCorrectedData7.Length - 1) : Array.Copy(bytCorrectedData7, 0, bytData, intPtr, bytCorrectedData7.Length)
                End Select
            Next i

            If MCB.AccumulateStats Then stcTuningStats.intGoodPSKFrameDataDecodes += 1
            ' Debug.WriteLine("[DecodePSKData2] bytPass = " & Format(bytPass, "X"))
        Else
            If MCB.AccumulateStats Then stcTuningStats.intFailedPSKFrameDataDecodes += 1
            ' Debug.WriteLine("[DecodePSKData2] bytPass = " & Format(bytPass, "X"))
        End If
    End Function  ' DecodePSKData

    '  Function to average two angles using magnitude weighting
    Private Function WeightedAngleAvg(intAng1 As Int16, intAng2 As Int16, intMag1 As Int16, intMag2 As Int16) As Int16
        ' Ang1 and Ang 2 are in the range of -3142 to + 3142 (miliradians)
        'works but should come up with a routine that avoids Sin, Cos, Atan2
        ' Modified in Rev 0.3.5.1 to "weight" averaging by intMag1 and intMag2
        Dim dblSumX, dblSumY As Double
        dblSumX = Cos(intAng1 / 1000) * intMag1 + Cos(intAng2 / 1000) * intMag2
        dblSumY = Sin(intAng1 / 1000) * intMag1 + Sin(intAng2 / 1000) * intMag2
        Return CInt(1000 * Atan2(dblSumY, dblSumX))
    End Function 'WeightedAngleAvg

    ' Function to Decode one Carrier of PSK modulation 
    Private Function Decode1CarPSK(intCarPhases() As Int16, intPSKMode As Integer, bytRawData() As Byte, ByRef bytCorrectCarData() As Byte, intRSLen As Integer, bytFrameType As Byte) As Boolean

        Dim intPhasePtr, int24Bits, intData As Int32
        Dim bytNoRS(bytRawData.Length - intRSLen - 1) As Byte

        For j As Integer = 0 To bytRawData.Length - 1
            Select Case intPSKMode
                Case 4 ' process 4 sequential phases per byte (2 bits per phase)
                    For k As Integer = 0 To 3
                        If k = 0 Then
                            bytRawData(j) = 0
                        Else
                            bytRawData(j) = bytRawData(j) << 2
                        End If

                        intPhasePtr = 4 * j + k
                        If intCarPhases(intPhasePtr) < 786 And intCarPhases(intPhasePtr) > -786 Then

                        ElseIf intCarPhases(intPhasePtr) >= 786 And intCarPhases(intPhasePtr) < 2356 Then
                            bytRawData(j) += 1
                        ElseIf intCarPhases(intPhasePtr) >= 2356 Or intCarPhases(intPhasePtr) <= -2356 Then
                            bytRawData(j) += 2
                        Else
                            bytRawData(j) += 3
                        End If
                    Next k
                Case 8 ' Process 8 sequential phases (3 bits per phase)  for 24 bits or 3 bytes  
                    ' Status verified on 1 Carrier 8PSK with no RS needed for High S/N
                    If (j + 1) Mod 3 = 0 Then ' we have 3 unprocessed bytes (24 bits) so process all 8 phases 
                        For k As Integer = 0 To 7
                            intPhasePtr = ((j - 2) \ 3) * 8 + k
                            If intCarPhases(intPhasePtr) < 393 And intCarPhases(intPhasePtr) > -393 Then
                                intData = 0
                            ElseIf intCarPhases(intPhasePtr) >= 393 And intCarPhases(intPhasePtr) < 1179 Then
                                intData = 1
                            ElseIf intCarPhases(intPhasePtr) >= 1179 And intCarPhases(intPhasePtr) < 1965 Then
                                intData = 2
                            ElseIf intCarPhases(intPhasePtr) >= 1965 And intCarPhases(intPhasePtr) < 2751 Then
                                intData = 3
                            ElseIf intCarPhases(intPhasePtr) >= 2751 Or intCarPhases(intPhasePtr) < -2751 Then
                                intData = 4
                            ElseIf intCarPhases(intPhasePtr) >= -2751 And intCarPhases(intPhasePtr) < -1965 Then
                                intData = 5
                            ElseIf intCarPhases(intPhasePtr) >= -1965 And intCarPhases(intPhasePtr) <= -1179 Then
                                intData = 6
                            Else
                                intData = 7
                            End If
                            If k = 0 Then
                                int24Bits = intData << 21
                            Else
                                int24Bits += (intData << (21 - 3 * k))
                            End If
                        Next k
                        bytRawData(j - 2) = (int24Bits And &HFF0000) >> 16
                        bytRawData(j - 1) = (int24Bits And &HFF00) >> 8
                        bytRawData(j) = (int24Bits And &HFF)
                    End If
            End Select
        Next j

        Array.Copy(bytRawData, 0, bytNoRS, 0, bytNoRS.Length)
        ' Check to see if correct without RS parity
        If CheckCRC16FrameType(bytNoRS, bytFrameType) Then ' No RS correction needed
            ReDim bytCorrectCarData(bytNoRS(0) - 1)
            Array.Copy(bytNoRS, 1, bytCorrectCarData, 0, bytNoRS(0))
            Return True
        Else
            ' Try correction with RS Parity
            Dim bytFailedNoRS(bytNoRS.Length - 1) As Byte
            Array.Copy(bytNoRS, bytFailedNoRS, bytNoRS.Length)
            objRS8.MaxCorrections = intRSLen \ 2
            bytNoRS = objRS8.RSDecode(bytRawData)
            If CheckCRC16FrameType(bytNoRS, bytFrameType) Then ' No RS correction needed
                ReDim bytCorrectCarData(bytNoRS(0) - 1)
                Array.Copy(bytNoRS, 1, bytCorrectCarData, 0, bytNoRS(0))
                Return True
            Else
                Return False
            End If
        End If
    End Function  ' Decode1CarPSK

    ' Function to demodulate one carrier for all PSK frame types
    Private Function Demod1CarPSK(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCarFreq As Int32, intNumOfSymbols As Int32, ByRef intPhase() As Int16, ByRef intMag() As Int16, strMod As String, blnInitReference As Boolean) As Boolean
        ' Converts intSamples to an array of differential phase and magnitude values for the Specific Carrier Freq
        ' intPtr should be pointing to the approximate start of the first reference/training symbol (1 of 3) 
        ' intPhase() is an array of phase values (in milliradians range of 0 to 6283) for each symbol 
        ' intMag() is an array of Magnitude values (not used in PSK decoding but for constellation plotting or QAM decoding)
        ' Objective is to use Minimum Phase Error Tracking to maintain optimum pointer position

        Dim intOutputPtr As Integer = 0
        Dim intSampPerSymbol As Int32
        Dim dblPhase, dblReal, dblImag As Double
        Dim intMiliRadPerSample As Int32 = intCarFreq * PI / 6
        Dim intCP As Integer  ' Cyclic prefix offset 
        Dim dblFreqBin As Double = intCarFreq / 200
        Dim dblPhaseInc As Double = 2 * PI * 1000 / 4 ' in milliradians for strMod= "4PSK"
        Dim intNforGoertzel As Int32

        If strMod = "8PSK" Then
            dblPhaseInc = 2 * PI * 1000 / 8
        End If
        Dim intPSKPhase_1, intPSKPhase_0, intPtrAtLastAdjust As Int32
        Dim intPCThresh As Int32 = 194 ' (about 22 degrees... should work for 4PSK or 8PSK)
        Static intSymbolCnt As Int32

        If intBaud = 100 And intCarFreq = 1500 Then
            intCP = 20 ' These values selected for best decode percentage (92%) and best average 4PSK Quality (82) on MPP0dB channel
            intSampPerSymbol = 120
            dblFreqBin = intCarFreq / 150
            intNforGoertzel = 80
        ElseIf intBaud = 100 Then
            intCP = 28 ' This value selected for best decoding percentage (56%) and best Averag 4PSK Quality (77) on mpg +5 dB
            intSampPerSymbol = 120
            intNforGoertzel = 60
            dblFreqBin = intCarFreq / 200
        ElseIf intBaud = 167 Then
            intCP = 6  ' Need to optimize (little difference between 6 and 12 @ wgn5, 2 Car 500 Hz)
            intSampPerSymbol = 72
            intNforGoertzel = 60
            dblFreqBin = intCarFreq / 200
        End If
        ' Check for sufficient samples
        If (intSamples.Length - intPtr) < (intSampPerSymbol + intSampPerSymbol * intNumOfSymbols) Then ' include one extra reference symbol
            Logs.Exception("[DemodulateDecode.Demod1CarPSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        ReDim Preserve intPhase(intOutputPtr + intNumOfSymbols - 1) ' 
        ReDim Preserve intMag(intOutputPtr + intNumOfSymbols - 1)
        intPtrAtLastAdjust = intPtr + 13
        If blnInitReference Then
            ' Get initial Reference Phase
            GoertzelRealImag(intSamples, intPtr + intCP, intNforGoertzel, dblFreqBin, dblReal, dblImag)
            dblPhase = Atan2(dblImag, dblReal)
            Track1CarPSK(intPtr, intCarFreq, strMod, dblPhase, True)
            intPSKPhase_1 = CShort(1000 * dblPhase)
            intPtr += intSampPerSymbol
        End If
        For i As Integer = 0 To intNumOfSymbols - 1
            GoertzelRealImag(intSamples, intPtr + intCP, intNforGoertzel, dblFreqBin, dblReal, dblImag)
            intMag(intOutputPtr) = CShort(Sqrt(dblReal ^ 2 + dblImag ^ 2))
            intPSKPhase_0 = 1000 * Atan2(dblImag, dblReal)
            intPhase(intOutputPtr) = -CShort(ComputeAng1_Ang2(intPSKPhase_0, intPSKPhase_1))
            intSymbolCnt += 1
            If Track1CarPSK(intPtr, intCarFreq, strMod, Atan2(dblImag, dblReal), False) <> 0 Then ' recompute inPSKPhase_0 upon a +/-1 adjustment in intPtr
                GoertzelRealImag(intSamples, intPtr + intCP, intNforGoertzel, dblFreqBin, dblReal, dblImag)
                intPSKPhase_0 = 1000 * Atan2(dblImag, dblReal)
            End If
            intPSKPhase_1 = intPSKPhase_0
            intOutputPtr += 1
            intPtr += intSampPerSymbol
        Next i
        If MCB.AccumulateStats Then stcTuningStats.intPSKSymbolCnt += intPhase.Length
        CorrectPhaseForTuningOffset(intPhase, strMod)
        Return True
    End Function  '  Demod1CarPSK

    ' Throw away test code to demodulate and log Sounder frames (to eventually be used in Path Compensation
    Public Function DemodSounder(intPtr As Int32, ByRef intSamples() As Int32) As Boolean  ' 
        Dim dblReal, dblImag, dblMag(7), dblMaxMag, dblPhase(7) As Double
        Dim strLog As String = ""
        Try
            For j As Integer = 0 To 16 ' for 17 groups
                intPtr += 60 ' advance 60 samples past end of Frame type to first tone of first group
                dblMaxMag = 0
                strLog = ""
                For i As Integer = 0 To 7 ' for 8 tones per group
                    ' Logs.WriteDebug("[DemodSounder] Ptr=" & intPtr.ToString)
                    Select Case i
                        Case 0, 1, 2, 3
                            GoertzelRealImag(intSamples, intPtr, 60, 4 + (2 * i), dblReal, dblImag)
                            dblMag(i) = Sqrt(dblReal ^ 2 + dblImag ^ 2)
                            dblPhase(i) = Atan2(dblImag, dblReal)
                            If dblMag(i) > dblMaxMag Then dblMaxMag = dblMag(i)
                        Case 4, 5, 6, 7
                            GoertzelRealImag(intSamples, intPtr, 60, -3 + (2 * i), dblReal, dblImag)
                            dblMag(i) = Sqrt(dblReal ^ 2 + dblImag ^ 2)
                            dblPhase(i) = Atan2(dblImag, dblReal)
                            If dblMag(i) > dblMaxMag Then dblMaxMag = dblMag(i)
                    End Select
                    intPtr += 120
                Next i
                For k As Integer = 0 To 7
                    dblMag(k) = dblMag(k) / dblMaxMag ' normalize the mags to highest = 1.000
                Next
                For n = 0 To 7
                    strLog &= "  Mag" & n.ToString & ":" & Format(dblMag(n), "0.00").PadLeft(5, " ") & " Ph:" & Format(dblPhase(n), "0.00").PadLeft(5, " ")
                Next n
                Logs.WriteDebug("[DemodSounder]" & j.ToString.PadRight(2, " ") & " " & strLog)
            Next j
        Catch ex As Exception
            Logs.Exception("[DemodSounder] Err: " & ex.ToString)
            Return False
        End Try
        Return True
    End Function 'DemodSounder

    'Function to compute PSK symbol tracking (all PSK modes, used for single or multiple carrier modes) 
    Public Function Track1CarPSK(ByRef intPtr As Int32, intCarFreq As Int32, strPSKMod As String, dblUnfilteredPhase As Double, blnInit As Boolean) As Int32
        ' This routine initializes and tracks the phase offset per symbol and adjust intPtr +/-1 when the offset creeps to a threshold value.
        ' adjusts (by Ref) intPtr 0, -1 or +1 based on a filtering of phase offset. 
        ' this seems to work fine on test Mar 21, 2015. May need optimization after testing with higher sample rate errors. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

        Dim dblAlpha As Double = 0.3 ' low pass filter constant  may want to optimize value after testing with large sample rate error. 
        ' (Affects how much averaging is done) lower values of dblAlpha will minimize adjustments but track more slugishly.
        Dim dblPhaseOffset As Double
        Static dblTrackingPhase As Double = 0
        Static dblModFactor As Double
        Static dblRadiansPerSample As Double ' range is .4188 @ car freq = 800 to 1.1195 @ car freq 2200
        Static dblPhaseAtLastTrack As Double
        Static intCountAtLastTrack As Int32
        Static dblFilteredPhaseOffset As Double
        If blnInit Then
            ' dblFilterredPhase = dblUnfilteredPhase
            dblTrackingPhase = dblUnfilteredPhase
            If strPSKMod = "8PSK" Then
                dblModFactor = PI / 4
            ElseIf strPSKMod = "4PSK" Then
                dblModFactor = PI / 2
            End If
            dblRadiansPerSample = intCarFreq * dbl2Pi / 12000
            dblPhaseOffset = dblUnfilteredPhase - dblModFactor * Round(dblUnfilteredPhase / dblModFactor)
            dblPhaseAtLastTrack = dblPhaseOffset
            dblFilteredPhaseOffset = dblPhaseOffset
            intCountAtLastTrack = 0
            Return 0
        End If
        intCountAtLastTrack += 1
        dblPhaseOffset = dblUnfilteredPhase - dblModFactor * Round(dblUnfilteredPhase / dblModFactor)
        dblFilteredPhaseOffset = (1 - dblAlpha) * dblFilteredPhaseOffset + dblAlpha * dblPhaseOffset
        If (dblFilteredPhaseOffset - dblPhaseAtLastTrack) > dblRadiansPerSample Then
            'Debug.WriteLine("Filtered>LastTrack: Cnt=" & intCountAtLastTrack.ToString & "  Filtered = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))
            dblFilteredPhaseOffset = dblPhaseOffset - dblRadiansPerSample
            dblPhaseAtLastTrack = dblFilteredPhaseOffset
            intPtr -= 1
            If MCB.AccumulateStats Then
                stcTuningStats.intPSKTrackAttempts += 1
                stcTuningStats.intAccumPSKTracking -= 1
            End If
            Return -1
        ElseIf (dblPhaseAtLastTrack - dblFilteredPhaseOffset) > dblRadiansPerSample Then
            'Debug.WriteLine("Filtered<LastTrack: Cnt=" & intCountAtLastTrack.ToString & "  Filtered = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))
            dblFilteredPhaseOffset = dblPhaseOffset + dblRadiansPerSample
            dblPhaseAtLastTrack = dblFilteredPhaseOffset
            intPtr += 1
            If MCB.AccumulateStats Then
                stcTuningStats.intPSKTrackAttempts += 1
                stcTuningStats.intAccumPSKTracking += 1
            End If
            Return +1
        Else
            'Debug.WriteLine("Filtered Phase = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))
        End If
        Return 0
    End Function '  Track1CarPSK

    ' Function to compute the differenc of two angles 
    Private Function ComputeAng1_Ang2(intAng1 As Int32, intAng2 As Int32) As Int32
        ' do an angle subtraction intAng1 minus intAng2 (in milliradians) 
        ' Results always between -3142 and 3142 (+/- Pi)
        Dim intDiff As Int32
        intDiff = intAng1 - intAng2
        If intDiff < -3142 Then
            intDiff += 6284
        ElseIf intDiff > 3142 Then
            intDiff -= 6284
        End If
        Return intDiff
    End Function 'ComputeAng1_Ang2

    ' Subroutine to capture wave data for debugging 
    'Private Sub CaptureWaveData()
    '    Try
    '        Dim strTimestamp As String = Format(DateTime.UtcNow, "HH:mm:ss")
    '        strTimestamp = strTimestamp.Replace(":", "_")
    '        Logs.WriteDebug("    [CaptureWaveData] with timestamp " & strTimestamp)
    '        Dim objWT As New WaveTools
    '        If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
    '            IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
    '        End If
    '        objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Acquire2ToneLeaderSymbolFraming_" & strTimestamp & ".wav", 12000, 16, dblAcquire2ToneLeaderSymbolFraming)
    '        objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\AcquirePSKFrameSyncRSB_" & strTimestamp & ".wav", 12000, 16, dblAcquirePSKFrameSyncRSB)
    '        objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Acquire4FSKFrameType_" & strTimestamp & ".wav", 12000, 16, dblAcquire4FSKFrameType)
    '        objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\FilEnvelopeIn_" & strTimestamp & ".wav", 12000, 16, dblFilEnvelopeIn)
    '        objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\FilEnvelopeOut_" & strTimestamp & ".wav", 12000, 16, dblFilEnvelopeOut)
    '    Catch ex As Exception
    '        Logs.Exception("[DemodDecod.CaptureWaveData] Err: " & ex.ToString)
    '    End Try
    'End Sub 'CaptureWaveData

    Public Sub New(objRef As Main)
        objMain = objRef
    End Sub

    ' Test code for evauating BiQuad filter implementation
    Public Function BiQuad(ByRef intSamples() As Int32, ByVal dblABCoef() As Double) As Int32()
        ' Implements a biQuad IIR filter
        ' dblABCoeff is a0,a1, a2, b0, b1, b2 bi Quad coeff
        Dim dblZ_1, dblZ_2, dblW As Double
        Dim intY(intSamples.Length - 1) As Int32

        For i As Integer = 0 To intSamples.Length - 1
            dblW = dblABCoef(0) * intSamples(i) - dblABCoef(1) * dblZ_1 - dblABCoef(2) * dblZ_2
            intY(i) = dblABCoef(3) * dblW + dblABCoef(4) * dblZ_1 + dblABCoef(5) * dblZ_2
            dblZ_2 = dblZ_1
            dblZ_1 = dblW
        Next
        Return intY

        'Public Function BiQuad(ByRef dblSamples() As Double, ByVal dblABCoef() As Double) As Double()
        '    ' Implements a biQuad IIR filter
        '    ' dblABCoeff is a0,a1, a2, b0, b1, b2 bi Quad coeff
        '    Dim dblZ_1, dblZ_2, dblW As Double
        '    Dim dblY(dblSamples.Length - 1) As Double

        '    For i As Integer = 0 To dblSamples.Length - 1
        '        dblW = dblABCoef(0) * dblSamples(i) - dblABCoef(1) * dblZ_1 - dblABCoef(2) * dblZ_2
        '        dblY(i) = dblABCoef(3) * dblW + dblABCoef(4) * dblZ_1 + dblABCoef(5) * dblZ_2
        '        dblZ_2 = dblZ_1
        '        dblZ_1 = dblW
        '    Next
        '    Return dblY
        'End Function

    End Function

    ' These are the half coefficients of a 33 tap FIR Lowpass filter 12000 sample rate
    ' Passband upper freq = 2800, stopband Lower freq = 3400, Ripple .75 dB Attn 41 dB (Simple Parks McClean) 
    Private dblLPF_2800() As Double = {-0.0050755947464400679, -0.018417131256505643, -0.00632999305648268, 0.012904481445110163, _
            0.00593147626279439, -0.018256051002382943, -0.0065547067034473641, 0.02599645802118513, _
            0.0075007481632127547, -0.037800329591872649, -0.00838240035124446, 0.057969479485350417, _
            0.0090936643461256052, -0.1025794677242949, -0.0095222361499455455, 0.31712010535725649, 0.5096703241022944}

    ' Not currently used.
    Public Sub LPF2800_12000SR(ByRef intSamples() As Int32, ByRef intFiltered() As Int32, Optional ByVal blnInit As Boolean = True)
        ' Assumes sample rate is 12000 and dblCoeff are the half coef mirrored about the last coeff
        Static dblRegister() As Double ' holds the history
        Static intPtr As Integer = 0
        Dim dblSum As Double
        Dim intRegPtr As Integer
        If blnInit Then
            ReDim dblRegister(dblLPF_2800.Length * 2 - 2)
            intPtr = 0
        End If
        ReDim intFiltered(intSamples.Length - 1)
        For i As Integer = 0 To intSamples.Length - 1
            dblSum = 0
            dblRegister(intPtr) = intSamples(i)
            For j As Integer = 0 To dblRegister.Length - 1
                intRegPtr = intPtr - j
                If intRegPtr < 0 Then intRegPtr += dblRegister.Length ' circular pointer
                If j < dblLPF_2800.Length Then
                    dblSum += dblLPF_2800(j) * dblRegister(intRegPtr)
                Else
                    dblSum += dblLPF_2800((2 * dblLPF_2800.Length - 1) - j) * dblRegister(intRegPtr)
                End If
            Next j
            intFiltered(i) = CInt(dblSum)
            intPtr += 1
            If intPtr >= dblRegister.Length Then intPtr = 0 'circular buffer
        Next i

    End Sub
End Class
