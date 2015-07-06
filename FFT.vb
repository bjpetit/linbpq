#Region "Options and Imports"
Option Explicit On
Option Strict Off
Imports System.Math
Imports System.IO
#End Region ' Options and Imports

Public Class FFT ' A class to impmplemnt a Fast Fourier Transform Algorithm 
#Region "Notes and documentation"
    '********************************************************************
    ' Execution time for a 2048 point FFT on a 1700 MHz P4 was about 5 ms)
    ' Some optimization could be made if only real inputs are insured.
    '   Rick Muething KN6KB, Mar 31, 2004
    '********************************************************************
    '--------------------------------------------------------------------
    ' VB FFT Release 2-B
    ' by Murphy McCauley (MurphyMc@Concentric.NET)
    ' 10/01/99
    '--------------------------------------------------------------------
    ' About:
    ' This code is very, very heavily based on Don Cross's fourier.pas
    ' Turbo Pascal Unit for calculating the Fast Fourier Transform.
    ' I've not implemented all of his functions, though I may well do
    ' so in the future.
    ' For more info, you can contact me by email, check my website at:
    ' http://www.fullspectrum.com/deeth/
    ' or check Don Cross's FFT web page at:
    ' http://www.intersrv.com/~dcross/fft.html
    ' You also may be intrested in the FFT.DLL that I put together based
    ' on Don Cross's FFT C code.  It's callable with Visual Basic and
    ' includes VB declares.  You can get it from either website.
    '--------------------------------------------------------------------
    ' History of Release 2-B:
    ' Fixed a couple of errors that resulted from me mucking about with
    '   variable names after implementation and not re-checking.  BAD ME.
    '  --------
    ' History of Release 2:
    ' Added FrequencyOfIndex() which is Don Cross's Index_to_frequency().
    ' FourierTransform() can now do inverse transforms.
    ' Added CalcFrequency() which can do a transform for a single
    '   frequency.
    '--------------------------------------------------------------------
    ' Usage:
    ' The useful functions are:
    ' FourierTransform() performs a Fast Fourier Transform on an pair of
    '  Double arrays -- one real, one imaginary.  Don't want/need
    '  imaginary numbers?  Just use an array of 0s.  This function can
    '  also do inverse FFTs.
    ' FrequencyOfIndex() can tell you what actual frequency a given index
    '  corresponds to.
    ' CalcFrequency() transforms a single frequency.
    '--------------------------------------------------------------------
    ' Notes:
    ' All arrays must be 0 based (i.e. Dim TheArray(0 To 1023) or
    '  Dim TheArray(1023)).
    ' The number of samples must be a power of two (i.e. 2^x).
    ' FrequencyOfIndex() and CalcFrequency() haven't been tested much.
    ' Use this ENTIRELY AT YOUR OWN RISK.
    '--------------------------------------------------------------------
#End Region ' Notes and documentation


#Region "Private Subs and Functions "
    Private Function NumberOfBitsNeeded(ByVal PowerOfTwo As Int32) As Byte
        Dim I As Byte
        For I = 0 To 16
            If (PowerOfTwo And (2 ^ I)) <> 0 Then
                NumberOfBitsNeeded = I
                Exit Function
            End If
        Next
        Return 0
    End Function


    Private Function IsPowerOfTwo(ByVal X As Int32) As Boolean
        IsPowerOfTwo = False
        If (X < 2) Then IsPowerOfTwo = False : Exit Function
        If (X And (X - 1)) = False Then IsPowerOfTwo = True
    End Function


    Private Function ReverseBits(ByVal Index As Int32, ByVal NumBits As Byte) As Int32
        Dim I As Byte, Rev As Int32

        For I = 0 To NumBits - 1
            Rev = (Rev * 2) Or (Index And 1)
            Index = Index \ 2
        Next

        ReverseBits = Rev
    End Function

    ' Subroutine for debugging saving intermediate values to file for later analysis by DSP Scope
    Private Sub SaveIQToFile(ByVal aryI() As Double, ByVal aryQ() As Double, ByVal strFilename As String)
        ' used for debug to allow reading with DSP Scope
        If IO.File.Exists(strFilename) Then IO.File.Delete(strFilename)
        Dim sw As New StreamWriter(strFilename, True)
        For i As Integer = 0 To aryI.Length - 1
            Dim strData As String = Format(aryI(i), "00000000.000000") & " " & Format(aryQ(i), "00000000.000000")
            sw.WriteLine(strData)
        Next i
        sw.Flush()
        sw.Close()
    End Sub 'SaveIQToFile
#End Region ' Private Subs and Functions

#Region "Friend Subs and Functions"

    Friend Sub FourierTransform(ByVal NumSamples As Long, ByVal RealIn() As Double, ByVal ImageIn() As Double, ByRef RealOut() As Double, ByRef ImagOut() As Double, Optional ByVal InverseTransform As Boolean = False)
        Dim AngleNumerator As Double
        Dim NumBits As Byte, I As Int32, j As Int32, K As Int32, n As Int32, BlockSize As Int32, BlockEnd As Int32
        Dim DeltaAngle As Double, DeltaAr As Double
        Dim Alpha As Double, Beta As Double
        Dim TR As Double, TI As Double, AR As Double, AI As Double

        If InverseTransform Then
            AngleNumerator = -2.0# * PI
        Else
            AngleNumerator = 2.0# * PI
        End If

        If (IsPowerOfTwo(NumSamples) = False) Or (NumSamples < 2) Then
            Logs.Exception("[FFT.FourierTransform] NumSamples is " & CStr(NumSamples) & ", which is not a positive integer power of two.")
            Return
        End If

        NumBits = NumberOfBitsNeeded(NumSamples)
        For I = 0 To (NumSamples - 1)
            j = ReverseBits(I, NumBits)
            RealOut(j) = RealIn(I)
            ImagOut(j) = ImageIn(I)
        Next

        BlockEnd = 1
        BlockSize = 2

        Do While BlockSize <= NumSamples
            DeltaAngle = AngleNumerator / BlockSize
            Alpha = Sin(0.5 * DeltaAngle)
            Alpha = 2.0# * Alpha * Alpha
            Beta = Sin(DeltaAngle)

            I = 0
            Do While I < NumSamples
                AR = 1.0#
                AI = 0.0#

                j = I
                For n = 0 To BlockEnd - 1
                    K = j + BlockEnd
                    TR = AR * RealOut(K) - AI * ImagOut(K)
                    TI = AI * RealOut(K) + AR * ImagOut(K)
                    RealOut(K) = RealOut(j) - TR
                    ImagOut(K) = ImagOut(j) - TI
                    RealOut(j) = RealOut(j) + TR
                    ImagOut(j) = ImagOut(j) + TI
                    DeltaAr = Alpha * AR + Beta * AI
                    AI = AI - (Alpha * AI - Beta * AR)
                    AR = AR - DeltaAr
                    j = j + 1
                Next

                I = I + BlockSize
            Loop

            BlockEnd = BlockSize
            BlockSize = BlockSize * 2
        Loop

        If InverseTransform Then
            'Normalize the resulting time samples...
            For I = 0 To NumSamples - 1
                RealOut(I) = RealOut(I) / NumSamples
                ImagOut(I) = ImagOut(I) / NumSamples
            Next
        End If
    End Sub


    Friend Function FrequencyOfIndex(ByVal NumberOfSamples As Int32, ByVal Index As Int32) As Double
        'Based on IndexToFrequency().  This name makes more sense to me.

        If Index >= NumberOfSamples Then
            FrequencyOfIndex = 0.0#
            Exit Function
        ElseIf Index <= NumberOfSamples / 2 Then
            FrequencyOfIndex = CDbl(Index) / CDbl(NumberOfSamples)
            Exit Function
        Else
            FrequencyOfIndex = -CDbl(NumberOfSamples - Index) / CDbl(NumberOfSamples)
            Exit Function
        End If
    End Function


    Friend Sub CalcFrequency(ByVal NumberOfSamples As Int32, ByVal FrequencyIndex As Int32, ByVal RealIn() As Double, ByVal ImagIn() As Double, ByVal RealOut As Double, ByVal ImagOut As Double)

        Dim K As Int32
        Dim Cos1 As Double, Cos2 As Double, Cos3 As Double, Theta As Double, Beta As Double
        Dim Sin1 As Double, Sin2 As Double, Sin3 As Double

        Theta = 2 * PI * FrequencyIndex / CDbl(NumberOfSamples)
        Sin1 = Sin(-2 * Theta)
        Sin2 = Sin(-Theta)
        Cos1 = Cos(-2 * Theta)
        Cos2 = Cos(-Theta)
        Beta = 2 * Cos2

        For K = 0 To NumberOfSamples - 2
            'Update trig values
            Sin3 = Beta * Sin2 - Sin1
            Sin1 = Sin2
            Sin2 = Sin3

            Cos3 = Beta * Cos2 - Cos1
            Cos1 = Cos2
            Cos2 = Cos3

            RealOut = RealOut + RealIn(K) * Cos3 - ImagIn(K) * Sin3
            ImagOut = ImagOut + ImagIn(K) * Cos3 + RealIn(K) * Sin3
        Next
    End Sub
#End Region ' Friend Subs and Functions

#Region "Public Subs and Functions"


    ' returns a magnitude FFT from a byte array of real Sound Card/wave file 16 bit data 
    Public Function MakeFFT(ByVal aryData() As Byte, ByVal intPtr As Integer, ByVal intFFTSize As Integer, Optional ByVal intDecimation As Integer = 1, Optional ByVal blnWin As Boolean = False) As Double()

        ' aryData is buffered SC data 2 bytes per sample (circular buffer)  
        ' intPtr is the pointer to the first data sample to use in the FFT
        ' int FFTsize is the FFT points (must be power of 2) 
        ' intDecimation is the decimation index (power of 2) value of 1 causes no decimation
        Static intWaveCnt As Integer
        Dim ReT((intFFTSize \ intDecimation) - 1) As Double
        Dim ImT((intFFTSize \ intDecimation) - 1) As Double
        Dim ReF((intFFTSize \ intDecimation) - 1) As Double
        Dim ImF((intFFTSize \ intDecimation) - 1) As Double
        Dim dblFFT((intFFTSize \ intDecimation) \ 2 - 1) As Double ' array for FFT output
        Dim intAryIndex As Integer
        Dim i As Integer
        If blnWin Then
            Dim dblAngleInc As Double = 2 * PI / (ReT.Length - 1)
            Dim dblAngle As Double = 0
            For i = 0 To ReT.Length - 1
                ' Read the captured data, window with Hanning window and convert to Real part of T array (double)
                intAryIndex = (intPtr + (2 * i)) Mod aryData.Length
                ReT(i) = (0.5 - 0.5 * Math.Cos(dblAngle)) * System.BitConverter.ToInt16(aryData, intAryIndex)
                dblAngle += dblAngleInc
            Next i
        Else
            For i = 0 To ReT.Length - 1
                ' Read the captured data without windowing and convert to Real part of T array (double)
                intAryIndex = (intPtr + (2 * i)) Mod aryData.Length
                System.BitConverter.ToInt16(aryData, intAryIndex)
            Next i
        End If
        ' Do the FFT creating Real and Imaginary freq
        FourierTransform(ReT.Length, ReT, ImT, ReF, ImF)
        intWaveCnt += 1
        ' compute the magnitude output array
        For i = 0 To dblFFT.Length - 1
            dblFFT(i) = Sqrt(ReF(i) ^ 2 + ImF(i) ^ 2)
            'dblFFT(i) = Abs(ReF(i))
        Next i
        Return dblFFT
    End Function

    ' returns the interpolated Frequency in bins and the magnitude (by ref) at the bin  
    Public Function FindPeakAndMag(ByVal aryData() As Byte, ByVal intPtr As Integer, ByVal intFFTSize As Integer, ByVal StartBin As Integer, ByVal StopBin As Integer, ByRef Mag As Double, ByRef SN As Double) As Double

        ' aryData is buffered SC data 2 bytes per sample (circular buffer)  
        ' intPtr is the pointer to the first data sample to use in the FFT
        ' int FFTsize is the FFT points (must be power of 2) 
        ' intDecimation is the decimation index (power of 2) value of 1 causes no decimation
        ' Returns interpolate bin of peak energy between StartBin and StopBin
        ' Also sets by Ref Magnitude at the peak and S/S+N at the peak
        Dim ReT(intFFTSize - 1) As Double
        Dim ImT(intFFTSize - 1) As Double
        Dim ReF(intFFTSize - 1) As Double
        Dim ImF(intFFTSize - 1) As Double
        Dim intAryIndex As Integer
        Dim i As Integer
        ' Read the captured data without windowing and convert to Real part of T array (double)
        For i = 0 To intFFTSize - 1
            intAryIndex = (intPtr + (2 * i)) Mod aryData.Length
            ReT(i) = System.BitConverter.ToInt16(aryData, intAryIndex)
        Next i
        ' Do the FFT creating Real and Imaginary freq
        FourierTransform(intFFTSize, ReT, ImT, ReF, ImF)
        ' Search for the peak...should only have to search the ReF since mag of ImF is same as ReF
        Dim dblPeak As Double = 0
        Dim intPeakIndex As Integer
        Dim dblEgr As Double
        Dim dblSum As Double
        For j As Integer = StartBin To StopBin
            dblEgr = ReF(j) ^ 2 + ImF(j) ^ 2
            dblSum += Sqrt(dblEgr)
            If dblEgr > dblPeak Then
                dblPeak = dblEgr
                intPeakIndex = j
            End If
        Next
        dblPeak = Sqrt(dblPeak) ' take the square root of the peak^2 energy
        ' do the interpolation based on formula found in Richard Lyons 
        ' Understanding Digital Signal Processing, 2nd Ed p(525) (should be accurate to better than .1 bin) 
        If intPeakIndex > 1 And intPeakIndex < ReF.Length - 2 Then ' possible to do the interpolation
            Dim Xk_1R As Double = ReF(intPeakIndex - 1) ' real component of one bin less than the peak
            Dim Xk_1I As Double = ImF(intPeakIndex - 1) ' imaginary component of one bin less than the peak
            Dim Xk1R As Double = ReF(intPeakIndex + 1) ' real component of one bin more than the peak
            Dim Xk1I As Double = ImF(intPeakIndex + 1) ' imaginary component of one bin more than the peak
            Dim DeltaNumR As Double = Xk1R - Xk_1R
            Dim DeltaNumI As Double = Xk1I - Xk_1I
            Dim DeltaDenomR As Double = 2 * ReF(intPeakIndex) - Xk_1R - Xk1R
            Dim DeltaDenomI As Double = 2 * ImF(intPeakIndex) - Xk_1I - Xk1I
            Dim DeltaMag As Double = Sqrt(DeltaNumR ^ 2 + DeltaNumI ^ 2) / Sqrt(DeltaDenomR ^ 2 + DeltaDenomI ^ 2)
            Dim DeltaAng As Double = Atan2(DeltaNumI, DeltaNumR) - Atan2(DeltaDenomI, DeltaDenomR)
            Dim DeltaR As Double = DeltaMag * Cos(DeltaAng)  ' the real part should be in the range -.5 to .5
            ' alternate (old) method
            Dim dblOldInterp As Double
            Dim dblOldSum As Double = dblPeak
            dblOldInterp = intPeakIndex * dblPeak
            dblOldInterp += (intPeakIndex - 1) * Sqrt(ReF(intPeakIndex - 1) ^ 2 + ImF(intPeakIndex - 1) ^ 2)
            dblOldSum += Sqrt(ReF(intPeakIndex - 1) ^ 2 + ImF(intPeakIndex - 1) ^ 2)
            dblOldInterp += (intPeakIndex + 1) * Sqrt(ReF(intPeakIndex + 1) ^ 2 + ImF(intPeakIndex + 1) ^ 2)
            dblOldSum += Sqrt(ReF(intPeakIndex + 1) ^ 2 + ImF(intPeakIndex + 1) ^ 2)
            dblOldInterp = dblOldInterp / dblOldSum
            FindPeakAndMag = dblOldInterp
            Mag = dblOldSum
        Else
            FindPeakAndMag = intPeakIndex ' no interpolation
            Mag = dblPeak
        End If
        SN = Mag / dblSum ' and the S/N (by Ref) 
    End Function

    ' returns the high resolution interpolated Frequency in bins and the magnitude (by ref) at the bin  
    Public Function HighResPeakAndMag(ByVal aryData() As Byte, ByVal intPtr As Integer, ByVal intFFTSize As Integer, ByVal StartBin As Integer, ByVal StopBin As Integer, ByRef Mag As Double, ByRef SN As Double) As Double
        ' similar to FindPeakAndMag but always uses 1024 point FFT with 0 padding and windowing for improved resolution
        ' aryData is buffered SC data 2 bytes per sample (circular buffer)  
        ' intPtr is the pointer to the first data sample to use in the FFT
        ' int FFTsize is the FFT points (must be power of 2) 
        ' Start and Stop bins are relative to 256 point FFT with bin resolution of 43.066 Hz
        ' Returns interpolate bin of peak energy between StartBin and StopBin relative to 256 point FFT (43.066 Hz/bin) 
        ' Also sets by Ref Magnitude at the peak and S/S+N at the peak
        Dim ReT(1023) As Double
        Dim ImT(1023) As Double
        Dim ReF(1023) As Double
        Dim ImF(1023) As Double
        Dim intAryIndex As Integer
        Dim intOff As Integer
        Select Case intFFTSize
            Case 1024
                intOff = 0
            Case 512
                intOff = 256
            Case 256
                intOff = 384
        End Select

        Dim dblAngleInc As Double = 2 * PI / (intFFTSize - 1)
        Dim dblAngle As Double = 0
        For i As Integer = 0 To intFFTSize - 1
            ' Read the captured data, window with Hanning window and convert to Real part of T array (double)
            intAryIndex = (intPtr + (2 * i)) Mod aryData.Length
            ReT(i + intOff) = (0.5 - 0.5 * Math.Cos(dblAngle)) * System.BitConverter.ToInt16(aryData, intAryIndex)
            dblAngle += dblAngleInc
        Next i

        ' Do the 1024 point FFT creating Real and Imaginary freq
        FourierTransform(1024, ReT, ImT, ReF, ImF)
        ' Search for the peak
        Dim dblPeak As Double = 0
        Dim intPeakIndex As Integer
        Dim dblEgr As Double
        Dim dblSum As Double
        For j As Integer = 4 * StartBin To 4 * StopBin ' assume start and stop are relative to 256 points transform
            dblEgr = ReF(j) ^ 2 + ImF(j) ^ 2
            dblSum += Sqrt(dblEgr)
            If dblEgr > dblPeak Then
                dblPeak = dblEgr
                intPeakIndex = j
            End If
        Next
        dblPeak = Sqrt(dblPeak) ' take the square root of the peak^2 energy
        ' do the interpolation based on formula found in Richard Lyons 
        ' Understanding Digital Signal Processing, 2nd Ed p(525) (should be accurate to better than .1 bin) 
        ' basic interpolation based on adjacent bin energy
        Dim dblInterp As Double
        Dim dblAdjSum As Double = dblPeak
        dblInterp = intPeakIndex * dblPeak
        dblInterp += (intPeakIndex - 1) * Sqrt(ReF(intPeakIndex - 1) ^ 2 + ImF(intPeakIndex - 1) ^ 2)
        dblAdjSum += Sqrt(ReF(intPeakIndex - 1) ^ 2 + ImF(intPeakIndex - 1) ^ 2)
        dblInterp += (intPeakIndex + 1) * Sqrt(ReF(intPeakIndex + 1) ^ 2 + ImF(intPeakIndex + 1) ^ 2)
        dblAdjSum += Sqrt(ReF(intPeakIndex + 1) ^ 2 + ImF(intPeakIndex + 1) ^ 2)
        dblInterp = dblInterp / dblAdjSum
        HighResPeakAndMag = 0.25 * dblInterp ' scale interpolated index back to releative to 256 point transform
        Mag = dblAdjSum
        SN = Mag / dblSum ' and the S/N (by Ref) 
    End Function ' HighResPeakAndMag

#End Region ' Public Subs and functions

End Class
