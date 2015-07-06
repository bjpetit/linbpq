Imports System.Math

Public Class BusyDetector
    ' A class to implement a channel busy detector
    Private objMain As Main
    Public intLastStart, intLastStop As Integer
    Public dblAvgPk2BaselineRatio, dblAvgBaselineSlow, dblAvgBaselineFast As Double
    Private dttLastBusy As Date = Now
    Private dttLastClear As Date = Now

    Public Property LastStart() As Integer
        Get
            Return intLastStart
        End Get

        Set(value As Integer)
            intLastStart = value
        End Set
    End Property

    Public Property LastStop() As Integer
        Get
            Return intLastStop
        End Get

        Set(value As Integer)
            intLastStop = value
        End Set
    End Property

    Public Property AvgPk2BaselineRatio() As Double
        Get
            Return dblAvgPk2BaselineRatio
        End Get

        Set(value As Double)
            dblAvgPk2BaselineRatio = value
        End Set
    End Property

    Public Property AvgBaselineSlow() As Double
        Get
            Return dblAvgBaselineSlow
        End Get

        Set(value As Double)
            dblAvgBaselineSlow = value
        End Set
    End Property

    Public Property AvgBaselineFast() As Double
        Get
            Return dblAvgBaselineFast
        End Get

        Set(value As Double)
            dblAvgBaselineFast = value
        End Set
    End Property

    Public ReadOnly Property LastBusy As Date
        Get
            Return dttLastBusy
        End Get
    End Property

    Public ReadOnly Property LastClear As Date
        Get
            Return dttLastClear
        End Get
    End Property

    ' Function to implement a busy detector based on 1024 point FFT
    Public Function BusyDetect(ByRef dblMag() As Double, intStart As Integer, intStop As Integer) As Boolean
        ' this only called while searching for leader ...once leader detected, no longer called.
        ' First look at simple peak over the frequency band of  interest.
        ' Status:  May 28, 2014.  Some initial encouraging results. But more work needed.
        '       1) Use of Start and Stop ranges good and appear to work well ...may need some tweaking +/_ a few bins.
        '       2) Using a Fast attack and slow decay for the dblAvgPk2BaselineRation number e.g.
        '       dblAvgPk2BaselineRatio = Max(dblPeakPwrAtFreq / dblAvgBaselineX, 0.9 * dblAvgPk2BaselineRatio)
        ' Seems to work well for the narrow detector. Tested on real CW, PSK, RTTY. 
        ' Still needs work on the wide band detector. (P3, P4 etc)  May be issues with AGC speed. (my initial on-air tests using AGC Fast).
        ' Ideally can find a set of parameters that won't require user "tweaking"  (Busy Squelch) but that is an alternative if needed. 
        ' use of technique of re initializing some parameters on a change in detection bandwidth looks good and appears to work well with 
        ' scanning.  Could be expanded with properties to "read back" these parameters so they could be saved and re initialize upon each new frequency. 

        Static intBusyCountPkToBaseline As Integer = 0
        Static intBusyCountFastToSlow As Integer = 0
        Static intBusyCountSlowToFast As Integer = 0
        Static blnLastBusy As Boolean = False

        Dim dblAvgBaseline As Double
        Dim dblPwrAtPeakFreq As Double
        Dim dblAvgBaselineX As Double
        Dim dblAlphaBaselineSlow As Double = 0.1 ' This factor affects the filtering of baseline slow. smaller = slower filtering
        Dim dblAlphaBaselineFast As Double = 0.5 ' This factor affects the filtering of baseline fast. smaller = slower filtering
        Dim intPkIndx As Integer
        Dim dblFSRatioNum, dblSFRatioNum As Double
        Dim blnFS, blnSF, blnPkBaseline As Boolean

        ' This holds off any processing of data until 100 ms after PTT release to allow receiver recovery.
        If Now.Subtract(objMain.objProtocol.dttStartRTMeasure).TotalMilliseconds < 100 Then Return blnLastBusy

        For i As Integer = intStart To intStop ' cover a range that matches the bandwidth expanded (+/-) by the tuning range
            If dblMag(i) > dblPwrAtPeakFreq Then
                dblPwrAtPeakFreq = dblMag(i)
                intPkIndx = i
            End If
            dblAvgBaseline += dblMag(i)
        Next i
        If intPkIndx = 0 Then Return 0

        ' add in the 2 bins above and below the peak (about 59 Hz total bandwidth)
        ' This needs refinement for FSK mods like RTTY which have near equal peaks making the Peak and baseline on strong signals near equal
        ' Computer the power within a 59 Hz spectrum around the peak
        dblPwrAtPeakFreq += (dblMag(intPkIndx - 2) + dblMag(intPkIndx - 1)) + (dblMag(intPkIndx + 2) + dblMag(intPkIndx + 1))
        dblAvgBaselineX = (dblAvgBaseline - dblPwrAtPeakFreq) / (intStop - intStart - 5) ' the avg Pwr per bin ignoring the 59 Hz area centered on the peak
        dblPwrAtPeakFreq = dblPwrAtPeakFreq / 5 'the the average Power (per bin) in the region of the peak (peak +/- 2bins...about 59 Hz)

        If intStart = intLastStart And intStop = intLastStop Then
            dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX
            dblAvgBaselineSlow = (1 - dblAlphaBaselineSlow) * dblAvgBaselineSlow + dblAlphaBaselineSlow * dblAvgBaseline
            dblAvgBaselineFast = (1 - dblAlphaBaselineFast) * dblAvgBaselineFast + dblAlphaBaselineFast * dblAvgBaseline
        Else ' This initializes the values after a bandwidth change
            dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX
            dblAvgBaselineSlow = dblAvgBaseline
            dblAvgBaselineFast = dblAvgBaseline
            intLastStart = intStart
            intLastStop = intStop
        End If

        If Now.Subtract(dttLastBusy).TotalMilliseconds < 1000 Or _
            (objMain.objProtocol.GetARDOPProtocolState <> ProtocolState.DISC) Then Return blnLastBusy
        If dblAvgPk2BaselineRatio > 1.118 * (MCB.Squelch ^ 1.5) Then   ' These values appear to work OK but may need optimization April 21, 2015
            blnPkBaseline = True
            dblAvgBaselineSlow = dblAvgBaseline
            dblAvgBaselineFast = dblAvgBaseline
            
        Else 'If intBusyCountPkToBaseline > 0 Then
            blnPkBaseline = False
        End If

        ' This detects wide band "pulsy" modes like Pactor 3, MFSK etc

        Select Case MCB.Squelch ' this provides a modest adjustment to the ratio limit based on practical squelch values
            ' These values yield less sensiivity for F:S which minimizes trigger by static crashes but my need further optimization May 2, 2015
            Case 2, 1, 0 : dblFSRatioNum = 1.9 : dblSFRatioNum = 1.2
            Case 3 : dblFSRatioNum = 2.1 : dblSFRatioNum = 1.4
            Case 4 : dblFSRatioNum = 2.3 : dblSFRatioNum = 1.6
            Case 5 : dblFSRatioNum = 2.5 : dblSFRatioNum = 1.8
            Case 6 : dblFSRatioNum = 2.7 : dblSFRatioNum = 2.0
            Case 7 : dblFSRatioNum = 2.9 : dblSFRatioNum = 2.2
            Case 8, 9, 10 : dblFSRatioNum = 3.1 : dblSFRatioNum = 2.4
        End Select

        ' This covers going from Modulation to no modulation e.g. End of Pactor frame
        If ((dblAvgBaselineSlow / dblAvgBaselineFast) > dblSFRatioNum) Then
            'Debug.WriteLine("     Slow to Fast")
            blnSF = True
        Else
            blnSF = False
        End If
        ' This covers going from no modulation to Modulation e.g. Start of Pactor Frame or Static crash
        If ((dblAvgBaselineFast / dblAvgBaselineSlow) > dblFSRatioNum) Then
            'Debug.WriteLine("     Fast to Slow")
            blnFS = True
        Else
            blnFS = False
        End If

        If blnFS Or blnSF Or blnPkBaseline Then
            'If blnFS Then Debug.WriteLine("Busy: Fast to Slow")
            'If blnSF Then Debug.WriteLine("Busy: Slow to Fast")
            'If blnPkBaseline Then Debug.WriteLine("Busy: Pk to Baseline")
            blnLastBusy = True
            dttLastBusy = Now
            Return True
        Else
            blnLastBusy = False
            dttLastClear = Now
            Return blnLastBusy
        End If
    End Function ' BusyDetect

    Public Sub New(objRef As Main)
        objMain = objRef
    End Sub
End Class
