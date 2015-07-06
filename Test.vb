Public Class Test
    Dim objMain As Main
    Dim intSamples() As Int32
    Dim bytEncodedBytes() As Byte
    Dim strFilename As String
    Dim strMsg As String = "KN6KB: The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                           "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                             "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                           "Now is the time for all good men to come to the aid of their country." & vbCr & _
                             "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                            "Now is the time for all good men to come to the aid of their country." & vbCr & _
                              "The quick brown fox jumped over the lazy dog's back." & vbCr & _
                            "Now is the time for all good men to come to the aid of their country." & vbCr & _
                             "The quick brown fox jumped over the lazy dog's back." & vbCr

    Public Sub New(objCaller As Main)

        ' This call is required by the designer.
        InitializeComponent()
        objMain = objCaller ' Provides a callback mechanism
        ' Add any initialization after the InitializeComponent() call.
        objMain.blnInTestMode = True
    End Sub

    Public Sub UpdateFrameCounter(intCount As Integer)
        lblCount.Text = intCount.ToString
        Refresh()
    End Sub

    Private Sub Button1_Click(sender As System.Object, e As System.EventArgs) Handles Button1.Click
        lblCount.Text = nudRepeats.Value.ToString
        InitializeConnection()
        ReDim bytEncodedBytes(0)
        intBreakCounts = 0
        intTestFrameCorrectCnt = 0


        ' BREAK
        ' for 4FSK modulation 
        If rdoBreak.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKControl(&H23, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(&H23, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' END
        If rdoEnd.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKControl(&H2C, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(&H2C, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' DISC
        If rdoDisc.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKControl(&H29, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(&H29, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' IDLE
        If rdoIdle.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKControl(&H26, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(&H26, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' CONREJBUSY
        If rdoConRejBusy.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKControl(&H2D, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(&H2D, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        'DataNAK
        If rdoDataNAK.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeDATANAK(50, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(bytEncodedBytes(0), bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        'DataACK
        If rdoDataACK.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeDATAACK(80, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKFrameID(bytEncodedBytes(0), bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' ConREQ
        If rdoCONREQ200.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeARQConRequest("KN6KB", "W1AW", "200M", strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H31, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        ElseIf rdoCONReq500.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeARQConRequest("KN6KB", "W1AW", "500M", strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H32, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        ElseIf rdoConReq1000.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeARQConRequest("KN6KB", "W1AW", "1000M", strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H33, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        ElseIf rdoConReq2000.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeARQConRequest("KN6KB", "W1AW", "2000M", strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H34, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        'ConACK with timing:
        If rdoConAck200.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeConACKwTiming(&H39, 1000, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKData(&H39, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        If rdoConAck500.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeConACKwTiming(&H3A, 100, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKData(&H3A, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        If rdoConAck1000.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeConACKwTiming(&H3B, 100, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKData(&H3B, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        If rdoConAck2000.Checked Then
            bytEncodedBytes = objMain.objMod.EncodeConACKwTiming(&H3C, 100, strFilename, &HFF)
            intSamples = objMain.objMod.Mod4FSKData(&H3C, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        '200 Hz modes

        ' 64 Data Byte + 32 RS 4PSK 200 Hz data frame (modified from 72 + 24 RS Dec 2, 2014
        If rdo4PSK200_64.Checked Then
            Dim dttStartTime As Date = Now
            Dim bytMsg() As Byte = GetBytes(strMsg.Substring(0, 64))  ' modified from 72 data bytes per frame 12/2/2014
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 64))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H40, bytMsg, strFilename)
            intSamples = objMain.objMod.ModPSK(&H40, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 16 Byte 4PSK 200 Hz data frame
        If rdo4PSK200_16.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 16))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H42, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H42, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 108 Byte 8PSK 200 Hz data frame
        If rdo8PSK200_108.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 108))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H44, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H44, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 32 byte 4FSK 200Hz data frame 
        If rdo4FSK200_32.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 32))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H46, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H46, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 16 byte 4FSK 200Hz data frame 
        If rdo4FSK200_16.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 16))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H48, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H48, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 24 byte 8FSK 200Hz data frame 
        If rdo8FSK200_24.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 24))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H4E, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod8FSKData(&H4E, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If


        ' 500 Hz modes:
        ' 128 byte 2 Car 4PSK 500 Hz BW frame
        If rdo4PSK500_128.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 128))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H50, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H50, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 240 byte 4PSK 167 baud Data frame
        If rdo4PSK500_240.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 240))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H54, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H54, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 216 Byte 8PSK 500 Hz data frame
        If rdo8PSK500_216.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 216))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H52, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H52, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 318 Byte 8PSK 500 Hz data frame
        If rdo8PSK500_318.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 318))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H56, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H56, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 64 byte 4FSK 500Hz data frame 
        If rdo4FSK500_64.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 64))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H4A, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H4A, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 32 byte 4FSK 500Hz data frame 
        If rdo4FSK500_32.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 32))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H4C, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H4C, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 32 byte 16FSK 500Hz data frame 
        If rdo16FSK500_32.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 32))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H58, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod16FSKData(&H58, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 16 byte 16FSK 500Hz data frame 
        If rdo16FSK500_16.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 16))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H5A, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod16FSKData(&H5A, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        '1000 Hz modes:
        ' 256 byte 4 Car 4PSK 1000 Hz BW frame
        If rdo4PSK1000_256.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 256))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H60, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H60, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 480 byte 4PSK 167 baud Data frame
        If rdo4PSK1000_480.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 480))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H64, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H64, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 432 Byte 8PSK 500 Hz data frame
        If rdo8PSK1000_432.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 432))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H62, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H62, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 636 Byte 8PSK 1000 Hz data frame
        If rdo8PSK1000_636.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 636))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H66, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H66, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 128 byte 2 car 4FSK 
        If rdo4FSK1000_128.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 128))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H68, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H68, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' 2000 Hz Modes:
        ' 512 byte 8 Car 4PSK 2000 Hz BW frame
        If rdo4PSK2000_512.Checked Then
            Dim dttTimeTest As Date = Now
            Dim bytMsg() As Byte = GetBytes(strMsg.Substring(0, 512))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H70, bytMsg, strFilename)
            intSamples = objMain.objMod.ModPSK(&H70, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 960 byte 4PSK 167 baud Data frame
        If rdo4PSK2000_960.Checked Then
            bytEncodedBytes = GetBytes((strMsg & strMsg).Substring(0, 960))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H74, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H74, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 864 Byte 8PSK 2000 Hz data frame
        If rdo8PSK2000_864.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 864))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H72, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H72, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 1272 Byte 8PSK 2000 Hz data frame
        If rdo8PSK2000_1272.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 1272))
            bytEncodedBytes = objMain.objMod.EncodePSK(&H76, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.ModPSK(&H76, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 256 byte 4 car 4FSK 
        If rdo4FSK2000_256.Checked Then
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 256))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H78, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H78, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 600 byte 1 Car 4FSK 600 bd
        If rdo4FSK2000_600.Checked Then
            ' Temp 600 byte 4FSK 600 Baud test (experimental) 
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 590))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H7A, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSK600BdData(&H7A, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If
        ' 200 byte 1 Car 4FSK 600 bd
        If rdo4FSK2000_200.Checked Then
            ' Temp 200 byte 4FSK 600 Baud test (experimental) 
            bytEncodedBytes = GetBytes(strMsg.Substring(0, 190))
            bytEncodedBytes = objMain.objMod.EncodeFSKData(&H7C, bytEncodedBytes, strFilename)
            intSamples = objMain.objMod.Mod4FSK600BdData(&H7C, bytEncodedBytes)
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' ID Frame 
        If rdoIDFrame.Checked Then
            bytEncodedBytes = objMain.objMod.Encode4FSKIDFrame("KN6KB", "EL98pf", strFilename)
            intSamples = objMain.objMod.Mod4FSKData(&H30, bytEncodedBytes)
            If MCB.CWID Then
                ReDim Preserve intSamples(intSamples.Length + 4800)
                Dim intCWID(-1) As Int32
                Dim intPtr As Integer = intSamples.Length
                objMain.objMod.CWID("DE " & MCB.Callsign, intCWID, False)
                ReDim Preserve intSamples(intSamples.Length + intCWID.Length)
                Array.Copy(intCWID, 0, intSamples, intPtr, intCWID.Length)
            End If
            objMain.SendTestFrame(intSamples, strFilename, nudRepeats.Value)
        End If

        ' Experimental Sounding
        If rdoSounding.Checked Then
            intSamples = objMain.objMod.ModSounder(&HFF)
            objMain.SendTestFrame(intSamples, "SOUND2K", nudRepeats.Value)
        End If
    End Sub

    Private Sub btnFECTest_Click(sender As System.Object, e As System.EventArgs) Handles btnFECTest.Click
        ' Set up to send 3 data frames repeated twice (9 total) using 1KHz 2 Car 4FSK FEC mode.
        ClearTuningStats()
        ClearQualityStats()
        InitializeConnection()
        Dim bytEncodedData() As Byte
        bytEncodedData = GetBytes(strMsg.Substring(0, 256)) ' Enough data for 2 full frames of 2 Car 4FSK 
        MCB.GridSquare = "EL98pf"
        objMain.objProtocol.ClearDataToSend()
        objMain.objProtocol.AddDataToDataToSend(bytEncodedData)
        Dim bytNull(-1) As Byte
        objMain.objProtocol.StartFEC(bytNull, MCB.FECMode, MCB.FECRepeats, True)
    End Sub

    Private Sub Test_FormClosed(sender As Object, e As System.Windows.Forms.FormClosedEventArgs) Handles Me.FormClosed
        objMain.blnInTestMode = False
    End Sub

    Private Sub Test_Load(sender As Object, e As System.EventArgs) Handles Me.Load
        lblFECInfo.Text = "Mode: " & MCB.FECMode & " Rpt = " & MCB.FECRepeats.ToString
    End Sub

    Private Sub bttFECAbort_Click(sender As System.Object, e As System.EventArgs) Handles bttFECAbort.Click
        objMain.objProtocol.Abort()
    End Sub

    Private Sub GroupBox2_Enter(sender As System.Object, e As System.EventArgs) Handles GroupBox2.Enter

    End Sub
End Class