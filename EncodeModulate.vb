Imports System.Math
Public Class EncodeModulate
    ' Programmer Rick Muething, KN6KB  rmuething@cfl.rr.com 

    ' This class provides for all Encoding (including FEC) and Modulation for the ARDOP protocol 
    Private intAmp As Int32 = 26000   ' Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 
    Public intTwoToneLeaderTemplate(119) As Int32 ' holds just 1 symbol (10 ms) of the leader
    Private intPSK100bdCarTemplate(8, 3, 119) As Int32 ' The actual templates over 9 carriers for 4 phase values and 120 samples
    '   (only positive Phase values are in the table, sign reversal is used to get the negative phase values) This reduces the table size from 7680 to 3840 integers
    Private intPSK200bdCarTemplate(8, 3, 71) As Int32 ' Templates for 200 bd with cyclic prefix
    Private intFSK25bdCarTemplate(15, 479) As Int32  ' Template for 16FSK carriers spaced at 25 Hz, 25 baud
    Private intFSK50bdCarTemplate(3, 239) As Int32  ' Template for 4FSK carriers spaced at 50 Hz, 50 baud
    Private intFSK100bdCarTemplate(19, 119) As Int32  ' Template for 4FSK carriers spaced at 100 Hz, 100 baud
    Private intFSK600bdCarTemplate(3, 19) As Int32 ' Template for 4FSK carriers spaced at 600 Hz, 600 baud  (used for FM only)

    Private objRS8 As New ReedSoloman8 ' Reed Soloman Encoder used in RS FEC modes. 
    Private objFrameInfo As New FrameInfo
    Private objWave As New WaveTools
    Private objMain As Main

    ' Subroutine to generate 1 symbol of leader
    Private Sub GenerateTwoToneLeaderTemplate()
        ' to create leader alternate these template samples reversing sign on each adjacent symbol
        For i As Integer = 0 To 119
            intTwoToneLeaderTemplate(i) = intAmp * 0.6 * (Sin(((1500 - 50) / 1500) * (i / 8 * 2 * PI)) - Sin((1500 + 50) / 1500 * (i / 8 * 2 * PI)))
        Next i
    End Sub 'GenerateTwoToneLeader

    ' Function to generate the Two-tone leader and Frame Sync (used in all frame types) 
    Private Function GetTwoToneLeaderWithSync(intSymLen As Int32) As Int32()
        ' Generate a 100 baud (10 ms symbol time) 2 tone leader 
        ' leader tones used are 1450 and 1550 Hz.  
        '
        Dim intLeader((120 * intSymLen) - 1) As Int32
        Dim intPtr As Int32 = 0
        Dim intSign As Integer = 1

        If intSymLen Mod 2 = 1 Then intSign = -1

        For i As Integer = 0 To intSymLen - 1  'for the number of symbols needed (two symbols less than total leader length) 
            For j As Integer = 0 To 119   ' for 120 samples per symbol (100 baud) 
                If i <> (intSymLen - 1) Then
                    intLeader(intPtr) = intSign * intTwoToneLeaderTemplate(j)
                Else
                    intLeader(intPtr) = -intSign * intTwoToneLeaderTemplate(j)
                End If
                intPtr += 1
            Next
            intSign = -intSign
        Next i
        ' *********************************
        ' Debug code to look at Leader
        'Dim dblLeader(intLeader.Length - 1) As Double
        'For k As Integer = 0 To intLeader.Length - 1
        '    dblLeader(k) = intLeader(k)
        'Next k
        'Dim objWT As New WaveTools
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\TwoToneLdr.wav", 12000, 16, dblLeader)
        ' End of debug code
        '************************************
        Return (intLeader)
    End Function  ' GenerateTwoToneLeaderWithSync2

    ' Function to generate a 5 second burst of two tone (1450 and 1550 Hz) used for setting up drive level
    Public Function ModTwoToneTest() As Int32()
        Return GetTwoToneLeaderWithSync(500)
    End Function 'ModTwoToneTest

    ' Function to generate Sounder for testing channel
    Public Function ModSounder(bytID As Byte) As Int32()

        Dim intSampleLen, intSamplePtr As Int32
        Dim intLeader() As Int32
        Dim intSampPerSym As Int32 = 60
        Dim bytMask As Byte
        Dim bytSymToSend As Byte
        Dim bytEncodedData() As Byte = {&HD0, &HD0 Xor bytID}

        ' For FEC transmission the computed leader length = MCB.Leader length
        intSampleLen = MCB.LeaderLength * 12 + 240 * 10 + intSampPerSym * 18 * 40 ' 40 groups of 17 test symbols
        Dim intSamples(intSampleLen - 1) As Int32
        intLeader = GetTwoToneLeaderWithSync(MCB.LeaderLength \ 10)
        Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
        intSamplePtr = intLeader.Length
        For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
            bytMask = &HC0
            For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                If k < 4 Then
                    bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                Else
                    bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                End If
                For n As Integer = 0 To 239
                    If ((5 * j + k) Mod 2 = 0) Then
                        intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                    Else
                        intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                    End If
                Next n
                intSamplePtr += 240
                bytMask = bytMask >> 2
            Next k
        Next j
        ' This generates 40 groups each group lasting 85 ms.  
        ' The first half of the group covers frequencies 800, 1200, 1600, and 2000
        ' The second half of the group covers frequencies 1000, 1400, 1800 and 2000 Hz
        ' In an actual sounding situation one would probably just use alternating groups of (800, 1200, 1600, and 2000)  followed by 
        '  a group of (1000, 1400, 1800, 2200) spaced at intervals (.5 to 1 sec)  throughout the frame. 

        For i As Integer = 0 To 39 ' for 40 groups
            For j As Integer = 0 To 15
                If j Mod 2 = 1 Then
                    For k As Integer = 0 To 59 ' for 60 samples per symbol 
                        Select Case j
                            Case 1, 3
                                intSamples(intSamplePtr) = intPSK200bdCarTemplate((j - 1), 0, k) ' tones at 800,  1200
                            Case 5, 7
                                intSamples(intSamplePtr) = intPSK200bdCarTemplate(j, 0, k) ' tones at  1600, 2000
                            Case 9, 11
                                intSamples(intSamplePtr) = intPSK200bdCarTemplate(j - 8, 0, k) ' tones at  1000, 1400
                            Case 13, 15
                                intSamples(intSamplePtr) = intPSK200bdCarTemplate((j - 7), 0, k) 'tones at  1800, 2200
                        End Select
                        intSamplePtr += 1
                    Next k
                Else
                    intSamplePtr += 60 ' 5 ms dead sone between symbols
                End If
            Next j
            intSamplePtr += 60 ' 5 ms dead sone between groups
        Next i

        ' *********************************
        ' Debug code to look at filter output
        'Dim dblSamples(intSampleLen - 1) As Double ' for plotting
        'Dim objWT As New WaveTools
        'For i As Integer = 0 To intSamples.Length - 1
        '    dblSamples(i) = intSamples(i)
        'Next
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFilteredSounding.wav", 12000, 16, dblSamples)
        ' objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered2000Hz.wav", 12000, 16, dblFilteredSamples)
        ' End of debug code
        '************************************
        Return FSXmtFilter2000_1500Hz(intSamples)
    End Function

    ' Subroutine to create the PSK symbol templates for 8 tones and 8 phases at 200 baud
    Private Sub GeneratePSKTemplates()
        ' Generate templates of 120 samples (each template = 10 ms) for each of the 9 possible carriers used in PSK modulation. 
        ' Used to speed up computation of PSK frames and reduce use of Sin functions.
        'Amplitude values will have to be scaled based on the number of Active Carriers (1, 2, 4 or 8) initial values should be OK for 1 carrier
        'Tone values 
        ' the carrier frequencies in Hz
        Dim dblCarFreq() As Double = {800, 1000, 1200, 1400, 1500, 1600, 1800, 2000, 2200}
        ' for 1 carrier modes use index 4 (1500)
        ' for 2 carrier modes use indexes 3, 5 (1400 and 1600 Hz)
        ' for 4 carrier modes use indexes 2, 3, 5, 6 (1200, 1400, 1600, 1800Hz) 
        ' for 8 carrier modes use indexes 0,1,2,3,5,6,7,8 (800, 1000, 1200, 1400, 1600, 1800, 2000, 2200 Hz) 
        Dim dblCarPhaseInc(8) As Double ' the phase inc per sample

        Dim dblAngle As Double ' Angle in radians
        'Dim dblPeakAmp As Double = intAmp * 0.5 ' may need to adjust 
        'Compute the phase inc per sample
        For i As Integer = 0 To 8
            dblCarPhaseInc(i) = 2 * PI * dblCarFreq(i) / 12000
        Next i
        ' Now compute the templates: (4320 32 bit values total)  
        For i As Integer = 0 To 8 'across 9 tones
            For j As Integer = 0 To 3 ' ( using only half the values and sign compliment for the opposit phases) 
                dblAngle = 2 * PI * j / 8
                ' 100 baud template
                For k As Integer = 0 To 119 ' for 120 samples (one 100 baud symbol, 200 baud modes will just use half of the data)
                    intPSK100bdCarTemplate(i, j, k) = intAmp * Sin(PI * k / 119) * Sin(dblAngle) ' with envelope control using Sin
                    dblAngle += dblCarPhaseInc(i)
                    If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
                Next k
                ' 167 baud template
                dblAngle = 2 * PI * j / 8
                For k As Integer = 0 To 71
                    intPSK200bdCarTemplate(i, j, k) = intAmp * Sin(dblAngle) ' with no envelope control
                    dblAngle += dblCarPhaseInc(i)
                    If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
                Next k
            Next j
        Next i
    End Sub  ' GeneratePSKTemplates

    ' Subroutine to create the FSK symbol templates
    Private Sub GenerateFSKTemplates()
        ' Generate templates of 240 samples (each symbol template = 20 ms) for each of the 4 possible carriers used in 200 Hz BW FSK modulation.
        ' Generate templates of 120 samples (each symbol template = 10 ms) for each of the 20 possible carriers used in 500, 1000 and 2000 Hz BW 4FSK modulation.
        ' Used to speed up computation of FSK frames and reduce use of Sin functions.
        '50 baud Tone values 
        ' the possible carrier frequencies in Hz ' note gaps for groups of 4 at 900, 1400, and 1900 Hz improved isolation between simultaneous carriers
        Dim dblCarFreq() As Double = {1425, 1475, 1525, 1575, 600, 700, 800, 900, 1100, 1200, 1300, 1400, 1600, 1700, 1800, 1900, 2100, 2200, 2300, 2400}
        Dim dblAngle As Double ' Angle in radians
        Dim dblCarPhaseInc(19) As Double
        'Compute the phase inc per sample
        For i As Integer = 0 To 3
            dblCarPhaseInc(i) = 2 * PI * dblCarFreq(i) / 12000
        Next i
        ' Now compute the templates: (960 32 bit values total)   
        For i As Integer = 0 To 3 'across the 4 tones for 50 baud frequencies
            dblAngle = 0
            '50 baud template
            For k As Integer = 0 To 239 ' for 240 samples (one 50 baud symbol)
                intFSK50bdCarTemplate(i, k) = intAmp * 1.1 * Sin(dblAngle) ' with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
                dblAngle += dblCarPhaseInc(i)
                If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
            Next k
        Next i

        ' 16 FSK templates (500 Hz BW, 25 baud)
        For i As Integer = 0 To 15 'across the 16 tones for 25 baud frequencies
            dblAngle = 0
            '25 baud template
            For k As Integer = 0 To 479 ' for 480 samples (one 25 baud symbol)
                intFSK25bdCarTemplate(i, k) = intAmp * 1.1 * Sin(dblAngle) ' with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
                dblAngle += (2 * PI / 12000) * (1312.5 + i * 25)
                If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
            Next k
        Next i

        ' 4FSK templates for 600 baud (2 Khz bandwidth) 
        For i As Integer = 0 To 3 'across the 4 tones for 600 baud frequencies
            dblAngle = 0
            '600 baud template
            For k As Integer = 0 To 19 ' for 20 samples (one 600 baud symbol)
                intFSK600bdCarTemplate(i, k) = intAmp * 1.1 * Sin(dblAngle) ' with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
                dblAngle += (2 * PI / 12000) * (600 + i * 600)
                If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
            Next k
        Next i

        '100 baud Tone values for a single carrier case 
        ' the 100 baud carrier frequencies in Hz
        dblCarFreq(0) = 1350
        dblCarFreq(1) = 1450
        dblCarFreq(2) = 1550
        dblCarFreq(3) = 1650
        ' Values of dblCarFreq for index 4-19 as in Dim above
        'Compute the phase inc per sample
        For i As Integer = 0 To 19
            dblCarPhaseInc(i) = 2 * PI * dblCarFreq(i) / 12000
        Next i
        ' Now compute the templates: (2400 32 bit values total)   
        For i As Integer = 0 To 19 'across 20 tones
            dblAngle = 0
            '100 baud template
            For k As Integer = 0 To 119 ' for 120 samples (one 100 baud symbol)
                intFSK100bdCarTemplate(i, k) = intAmp * 1.1 * Sin(dblAngle) ' with no envelope control (factor 1.1 chosen emperically to keep FSK peak amplitude slightly below 2 tone peak)
                dblAngle += dblCarPhaseInc(i)
                If dblAngle >= 2 * PI Then dblAngle -= 2 * PI
            Next k
        Next i
    End Sub  ' GenerateFSKTemplates

    ' Called upon instantiation of this class
    Public Sub New(objRef As Main)
        objMain = objRef
        GenerateTwoToneLeaderTemplate()
        GeneratePSKTemplates() ' Generate all templates for 9 carriers,  8 phases, for  10 ms  symbols 
        GenerateFSKTemplates() ' Generate all templates for 50 baud 4FSK and 25 baud 16FSK
    End Sub  ' New

    ' Subroutine to add trailer before filtering
    Private Sub AddTrailer(ByRef intSamples() As Int32)
        Dim intPtr As Int32 = intSamples.Length
        Dim intAddedSymbols As Integer = 1 + MCB.TrailerLength \ 10 ' add 1 symbol + 1 per each 10 ms of MCB.Trailer
        ReDim Preserve intSamples(intSamples.Length + intAddedSymbols * 120 - 1)
        For i As Integer = 1 To intAddedSymbols
            For k As Integer = 0 To 119
                intSamples(intPtr) = intPSK100bdCarTemplate(4, 0, k)
                intPtr += 1
            Next k
        Next i
    End Sub 'AddTrailer

    ' Subroutine to creat a playable wave stream (more efficent than playing a .wav file from Disk) 
    Public Sub CreateWaveStream(ByRef intFilteredSamplesToSend() As Int32)
        If intFilteredSamplesToSend.Length < 1000 Then
            Logs.Exception("[EncodeModulate.CreateWaveStream] intFilteredSamplesToSend Length = " & intFilteredSamplesToSend.Length.ToString)
            Exit Sub
        End If

        Dim intTrace As Integer = 1
        Try
            Dim aryWaveData(2 * intFilteredSamplesToSend.Length - 1) As Byte
            For n As Integer = 0 To intFilteredSamplesToSend.Length - 1
                aryWaveData(2 * n) = CByte(intFilteredSamplesToSend(n) And &HFF)
                aryWaveData(2 * n + 1) = CByte((intFilteredSamplesToSend(n) And &HFF00) \ 256)
            Next n
            intTrace = 2
            ' objWave.WriteRIFF(strFilename, 12000, 16, aryWaveData) ' used for debug to observe file
            objWave.WriteRIFFStream(memWaveStream, 12000, 16, aryWaveData)
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.CreateWaveStream] intFilteredSamplesToSend Length = " & intFilteredSamplesToSend.Length.ToString & "  Trace=" & intTrace.ToString & "  Err: " & ex.ToString)
        End Try
    End Sub  ' CreateWaveStream

    ' Function to encode data for all PSK frame types
    Public Function EncodePSK(bytFrameType As Byte, ByRef bytDataToSend() As Byte, ByRef strFrameName As String) As Byte()
        ' Objective is to use this to use this to send all PSK data frames 
        'Output is a byte array which includes:
        ' 1) A 2 byte Header which includes the Frame ID.  The frame ID will be sent using 4FSK 50 baud (1500 Hz center). It will include the Frame ID and the Frame ID XOR with bytSession ID.
        ' 2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
        ' Ininitial implementation:
        '   intNum Car may be 1, 2, 4 or 8
        '   intBaud may be 100, 167
        '   intPSKMode may be 4 (4PSK) or 8 (8PSK) 
        '   bytDataToSend must be equal to or less than max data supported or a exception will be logged and an empty array returned

        ' First determine if bytDataToSend is compatible with the requested modulation mode.
        Dim intNumCar, intBaud, intDataLen, intRSLen, intDataToSendPtr, intEncodedDataPtr As Integer
        Dim intCarDataCnt, intStartIndex As Integer
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim blnFrameTypeOK As Boolean
        Dim bytQualThresh As Byte

        blnFrameTypeOK = objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        If intDataLen = 0 Or bytDataToSend.Length = 0 Or Not blnFrameTypeOK Then
            Logs.Exception("[EncodePSK] Failure to update parameters for frame type " & bytFrameType.ToString & "  DataToSend Len=" & bytDataToSend.Length.ToString)
            Return Nothing
        End If
        If bytDataToSend.Length > intNumCar * intDataLen Then
            Logs.Exception("[EncodePSK] Data length exceeds frame capacity. Frame type=H" & Format(bytFrameType, "X") & "  Data Length=" & bytDataToSend.Length.ToString)
            Return Nothing
        End If
        strFrameName = strType
        '  Need: (2 bytes for Frame Type) +(1 byte byteCount,  Data + 2 byteCRC + RS) per carrier
        Dim bytEncodedData(2 + intNumCar * (intDataLen + intRSLen + 3) - 1) As Byte
        ' Generate the 2 bytes for the frame type data:
        bytEncodedData(0) = bytFrameType
        bytEncodedData(1) = bytFrameType Xor stcConnection.bytSessionID
        Dim bytToRS(intDataLen + 3 - 1) As Byte ' byte Count + Data  + 2 byte CRC

        intDataToSendPtr = 0
        intEncodedDataPtr = 2
        objRS8.MaxCorrections = intRSLen \ 2  ' RS length must be even

        ' Now compute the RS frame for each carrier in sequence and move it to bytEncodedData 
        For i As Integer = 0 To intNumCar - 1 ' across all carriers
            intCarDataCnt = bytDataToSend.Length - intDataToSendPtr
            If intCarDataCnt >= intDataLen Then
                ReDim bytToRS(intDataLen + 3 - 1)
                bytToRS(0) = intDataLen : intStartIndex = intEncodedDataPtr
                Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intDataLen)
                intDataToSendPtr += intDataLen
            Else
                ReDim bytToRS(intDataLen + 3 - 1) ' zero out bytToRS
                bytToRS(0) = intCarDataCnt ' Could be 0 if insuffient data for # of carriers 
                If intCarDataCnt > 0 Then Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intCarDataCnt)
                intDataToSendPtr += intCarDataCnt
            End If
            'GenCRC16(bytToRS, 0, intDataLen) 'calculate the CRC on the byte count  data bytes
            GenCRC16FrameType(bytToRS, 0, intDataLen, bytFrameType)
            bytToRS = objRS8.RSEncode(bytToRS)
            Array.Copy(bytToRS, 0, bytEncodedData, intEncodedDataPtr, bytToRS.Length)
            intEncodedDataPtr += bytToRS.Length
        Next i
        Return bytEncodedData
    End Function 'EncodePSK

    ' Function to encode data for all FSK frame types
    Public Function EncodeFSKData(bytFrameType As Integer, ByRef bytDataToSend() As Byte, ByRef strFrameName As String) As Byte()
        ' Objective is to use this to use this to send all 4FSK data frames 
        'Output is a byte array which includes:
        ' 1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
        ' 2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
        ' Ininitial implementation:
        '   intNum Car may be 1, 2, 4 or 8
        '   intBaud may be 50, 100
        '   strMod is 4FSK) 
        '   bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned
        ' First determine if bytDataToSend is compatible with the requested modulation mode.
        Dim intNumCar, intBaud, intDataLen, intRSLen, intDataToSendPtr, intEncodedDataPtr As Integer
        Dim intCarDataCnt, intStartIndex As Integer
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim blnFrameTypeOK As Boolean
        Dim bytQualThresh As Byte

        blnFrameTypeOK = objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        If intDataLen = 0 Or bytDataToSend.Length = 0 Or Not blnFrameTypeOK Then
            Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
            Return Nothing
        End If
        strFrameName = strType
        '  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)
        Dim bytEncodedData(2 + intNumCar * (intDataLen + intRSLen + 1 + 2) - 1) As Byte
        ' Generate the 2 bytes for the frame type data:
        bytEncodedData(0) = bytFrameType
        bytEncodedData(1) = bytFrameType Xor stcConnection.bytSessionID
        Dim bytToRS(intDataLen + 3 - 1) As Byte ' Data + Count + 2 byte CRC
        intDataToSendPtr = 0
        intEncodedDataPtr = 2
        objRS8.MaxCorrections = intRSLen \ 2  ' RS length must be even

        If intBaud < 600 Or intDataLen < 600 Then
            ' Now compute the RS frame for each carrier in sequence and move it to bytEncodedData 
            For i As Integer = 0 To intNumCar - 1 ' across all carriers
                intCarDataCnt = bytDataToSend.Length - intDataToSendPtr
                If intCarDataCnt >= intDataLen Then
                    ReDim bytToRS(intDataLen + 3 - 1)
                    bytToRS(0) = intDataLen : intStartIndex = intEncodedDataPtr
                    Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intDataLen)
                    intDataToSendPtr += intDataLen
                Else
                    ReDim bytToRS(intDataLen + 3 - 1) ' zero out bytToRS
                    bytToRS(0) = intCarDataCnt ' Could be 0 if insuffient data for # of carriers 
                    If intCarDataCnt > 0 Then Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intCarDataCnt)
                    intDataToSendPtr += intCarDataCnt
                End If
                GenCRC16FrameType(bytToRS, 0, intDataLen, CByte(bytFrameType)) 'calculate the CRC on the byte count  data bytes
                bytToRS = objRS8.RSEncode(bytToRS)
                Array.Copy(bytToRS, 0, bytEncodedData, intEncodedDataPtr, bytToRS.Length)
                intEncodedDataPtr += bytToRS.Length
            Next i
            Return bytEncodedData
        Else ' special case for 600 baud 4FSK which has 600 byte data field sent as three sequencial (200 byte + 50 byte RS) groups
            ReDim bytEncodedData((2 + intDataLen + intRSLen + 9) - 1) ' handles 3 groups of data with independed count and CRC
            bytEncodedData(0) = bytFrameType
            bytEncodedData(1) = bytFrameType Xor stcConnection.bytSessionID
            objRS8.MaxCorrections = intRSLen \ 6
            For i As Integer = 0 To 2 ' for three blocks of RS data
                ReDim bytToRS(intDataLen \ 3 + 3 - 1)
                intCarDataCnt = bytDataToSend.Length - intDataToSendPtr
                If intCarDataCnt >= intDataLen \ 3 Then
                    bytToRS(0) = intDataLen \ 3
                    Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, intDataLen \ 3)
                    intDataToSendPtr += intDataLen \ 3
                Else
                    bytToRS(0) = bytDataToSend.Length - intDataToSendPtr
                    Array.Copy(bytDataToSend, intDataToSendPtr, bytToRS, 1, bytDataToSend.Length - intDataToSendPtr)
                    intDataToSendPtr = intDataLen
                End If
                GenCRC16FrameType(bytToRS, 0, intDataLen \ 3, CByte(bytFrameType)) 'calculate the CRC on the byte count  data bytes
                bytToRS = objRS8.RSEncode(bytToRS)
                Array.Copy(bytToRS, 0, bytEncodedData, intEncodedDataPtr, bytToRS.Length)
                intEncodedDataPtr += bytToRS.Length
            Next i
            Return bytEncodedData
        End If
    End Function 'Encode4FSKD

    '  Function to accept the output of EncodePSK and create the integer array of 32 bit samples suitable for playing 
    Public Function ModPSK(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()

        Dim intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim intCarIndex, intCarStartIndex As Integer
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytQualThresh As Byte
        Dim bytSym, bytSymToSend, bytMask As Byte
        Dim bytLastSym(8) As Byte ' Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 
        Dim dblCarScalingFactor As Double
        Dim intPeakAmp As Int32
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        Try
            If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType) Then
                Logs.Exception("[ModPSK] no data frame info for frame type H" & Format(bytFrame, "X"))
                Return Nothing
            End If

            strLastWavStream = strType
            If intLeaderLen = 0 Then
                intLeaderLenMS = MCB.LeaderLength
            Else
                intLeaderLenMS = intLeaderLen
            End If
            Select Case intBaud
                Case 100
                    intSampPerSym = 120
                    If strMod = "4PSK" Then
                        intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * (4 * (intDataLen + intRSLen + 3)) ' add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym for initial reference phase
                    ElseIf strMod = "8PSK" Then
                        intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * ((8 * (intDataLen + intRSLen + 3)) \ 3) ' add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym reference phase
                    End If
                Case 167
                    intSampPerSym = 72 ' the total number of samples per symbol 
                    If strMod = "4PSK" Then
                        intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + intSampPerSym * (4 * (intDataLen + intRSLen + 3)) ' add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym for initial reference phase
                    ElseIf strMod = "8PSK" Then
                        intSampleLen = intLeaderLenMS * 12 + 10 * 240 + intSampPerSym + +intSampPerSym * ((8 * (intDataLen + intRSLen + 3)) \ 3) ' add 10 50 baud 4FSK symbols for Frame Type + one extra intSamplePerSym reference phase
                    End If
            End Select
            Select Case intNumCar
                Case 1 : intCarStartIndex = 4 : dblCarScalingFactor = 1.0 ' Starting at 1500 Hz  (scaling factors determined emperically to minimize crest factor)  TODO:  needs verification
                Case 2 : intCarStartIndex = 3 : dblCarScalingFactor = 0.53 ' Starting at 1400 Hz
                Case 4 : intCarStartIndex = 2 : dblCarScalingFactor = 0.29 ' Starting at 1200 Hz
                Case 8 : intCarStartIndex = 0 : dblCarScalingFactor = 0.17 ' Starting at 800 Hz
            End Select
            intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
            Dim intSamples(intSampleLen) As Int32
            Dim intLeader() As Int32 = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)
            Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
            intSamplePtr = intLeader.Length
            ' Create the 10 symbols (20 bit) 50 baud 4FSK frame type with Implied SessionID and 2 symbol parity
            ' No reference needed for 4FSK
            For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
                bytMask = &HC0
                For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                    If k < 4 Then
                        bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                    Else
                        bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                    End If
                    For n As Integer = 0 To 239
                        If ((5 * j + k) Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                        End If
                    Next n
                    intSamplePtr += 240
                    bytMask = bytMask >> 2
                Next k
            Next j

            intPeakAmp = 0
            intCarIndex = intCarStartIndex ' initialize to correct starting carrier
            ' Now create a reference symbol for each carrier
            For i As Integer = 0 To intNumCar - 1 ' across all carriers
                bytSymToSend = 0  '  using non 0 causes error on first data byte 12/8/2014   ...Values 0-3  not important (carries no data).   (Possible chance for Crest Factor reduction?)
                bytLastSym(i) = bytSymToSend
                For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                    If intBaud = 100 Then
                        If bytSymToSend < 2 Then
                            intSamples(intSamplePtr + n) += intPSK100bdCarTemplate(intCarIndex, bytSymToSend * 2, n) ' double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
                        Else
                            intSamples(intSamplePtr + n) -= intPSK100bdCarTemplate(intCarIndex, 2 * (bytSymToSend - 2), n) ' subtract 2 from the symbol value before doubling and subtract value of table 
                        End If
                        If i = intNumCar - 1 Then
                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor ' on the last carrier rescale value based on # of carriers to bound output
                        End If
                    ElseIf intBaud = 167 Then
                        If bytSymToSend < 2 Then  ' This uses the symmetry of the symbols to reduce the table size by a factor of 2
                            intSamples(intSamplePtr + n) += intPSK200bdCarTemplate(intCarIndex, 2 * bytSymToSend, n) ' double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
                        Else ' untested 
                            intSamples(intSamplePtr + n) -= intPSK200bdCarTemplate(intCarIndex, 2 * (bytSymToSend - 2), n)
                        End If
                        If i = intNumCar - 1 Then
                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor ' on the last carrier rescale value based on # of carriers to bound output
                        End If
                    End If
                Next n
                intCarIndex += 1
                If intCarIndex = 4 Then intCarIndex += 1 ' skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
            Next i
            intSamplePtr += intSampPerSym
            ' End of reference phase generation 

            intDataPtr = 2 ' initialize pointer to start of data.
            Select Case strMod
                Case "4PSK"
                    For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data (all carriers) 
                        bytMask = &HC0 ' Initialize mask each new data byte
                        For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                            intCarIndex = intCarStartIndex ' initialize the carrrier index
                            For i As Integer = 0 To intNumCar - 1 ' across all carriers
                                bytSym = (bytMask And bytEncodedData(intDataPtr + i * intDataBytesPerCar)) >> (2 * (3 - k))
                                bytSymToSend = ((bytLastSym(intCarIndex) + bytSym) Mod 4) ' Values 0-3

                                For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                                    If intBaud = 100 Then
                                        If bytSymToSend < 2 Then  ' This uses the symmetry of the symbols to reduce the table size by a factor of 2
                                            intSamples(intSamplePtr + n) += intPSK100bdCarTemplate(intCarIndex, 2 * bytSymToSend, n) ' double the symbol value during template lookup for 4PSK. (skips over odd PSK 8 symbols)
                                        Else
                                            intSamples(intSamplePtr + n) -= intPSK100bdCarTemplate(intCarIndex, 2 * (bytSymToSend - 2), n) ' subtract 2 from the symbol value befor doubling and subtract value of table 
                                        End If
                                        If i = intNumCar - 1 Then
                                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor ' Need to examine clipping and scaling here 
                                        End If
                                    Else
                                        If bytSymToSend < 2 Then  ' This uses the symmetry of the symbols to reduce the table size by a factor of 2
                                            intSamples(intSamplePtr + n) += intPSK200bdCarTemplate(intCarIndex, 2 * bytSymToSend, n)
                                        Else ' untested 
                                            intSamples(intSamplePtr + n) -= intPSK200bdCarTemplate(intCarIndex, 2 * (bytSymToSend - 2), n)
                                        End If
                                        If i = intNumCar - 1 Then
                                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor
                                        End If
                                    End If
                                Next n
                                bytLastSym(intCarIndex) = bytSymToSend
                                intCarIndex += 1
                                If intCarIndex = 4 Then intCarIndex += 1 ' skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
                            Next i
                            bytMask = bytMask >> 2
                            intSamplePtr += intSampPerSym
                        Next k
                        intDataPtr += 1
                    Next m

                Case "8PSK"
                    ' More complex ...must go through data in 3 byte chunks creating 8 Three bit symbols for each 3 bytes of data. 
                    For m As Integer = 0 To intDataBytesPerCar \ 3 - 1
                        For k As Integer = 0 To 7 ' for 8 symbols in 24 bits of int3Bytes
                            intCarIndex = intCarStartIndex ' initialize the carrrier index
                            For i = 0 To intNumCar - 1
                                bytSym = GetSym8PSK(intDataPtr, k, i, bytEncodedData, intDataBytesPerCar)
                                bytSymToSend = ((bytLastSym(intCarIndex) + bytSym) Mod 8)
                                For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                                    If intBaud = 100 Then
                                        If bytSymToSend < 4 Then  ' This uses the symmetry of the symbols to reduce the table size by a factor of 2
                                            intSamples(intSamplePtr + n) += intPSK100bdCarTemplate(intCarIndex, bytSymToSend, n) ' positive phase values template lookup for 8PSK.
                                        Else
                                            intSamples(intSamplePtr + n) -= intPSK100bdCarTemplate(intCarIndex, bytSymToSend - 4, n) ' negative phase values,  subtract value of table 
                                        End If
                                        If i = intNumCar - 1 Then
                                            intSamples(intSamplePtr + n) = intSamples(intSamplePtr + n) * dblCarScalingFactor ' Need to examine clipping and scaling here 
                                        End If
                                    Else
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
                                Next n
                                bytLastSym(intCarIndex) = bytSymToSend
                                intCarIndex += 1
                                If intCarIndex = 4 Then intCarIndex += 1 ' skip over 1500 Hz for multi carrier modes (multi carrier modes all use even hundred Hz tones)
                            Next i
                            intMask = intMask >> 3
                            intSamplePtr += intSampPerSym
                        Next k
                        intDataPtr += 3
                    Next m
            End Select
            ' Xmit filter per bandwidth
            Select Case intNumCar
                Case 1
                    intSamples = FSXmtFilter200_1500Hz(intSamples)
                Case 2
                    intSamples = FSXmtFilter500_1500Hz(intSamples)
                Case 4
                    intSamples = FSXmtFilter1000_1500Hz(intSamples)
                Case 8
                    intSamples = FSXmtFilter2000_1500Hz(intSamples)
            End Select
            Return intSamples
        Catch ex As Exception
            Logs.Exception("[ModPSK] bytFrame= H" & Format(bytFrame, "X") & "  DataLen=" & bytEncodedData.Length.ToString & "  Err: " & ex.ToString)
        End Try
        Return Nothing
    End Function ' ModPSK

    ' Function to Modulate encoded data to 8FSK and create the integer array of 32 bit samples suitable for playing 
    Public Function Mod8FSKData(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()
        ' generates 8FSK code for 1 active carrier, 200 Hz bandwidth centered on 1500 Hz
        Dim intThreeBytes, intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytSymToSend, bytMask, bytMinQualThresh As Byte
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        Try
            If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytMinQualThresh, strType) Then
                Logs.Exception("[Mod8FSKData] no data for frame type H" & Format(bytFrame, "x"))
                Return Nothing
            End If
            If strMod <> "8FSK" Then
                Logs.Exception("[Mod8FSKData] Frame Type: H" & Format(bytFrame, "X") & " is not 8FSK")
                Return Nothing
            End If
            strLastWavStream = strType
            If intLeaderLen = 0 Then
                intLeaderLenMS = MCB.LeaderLength
            Else
                intLeaderLenMS = intLeaderLen
            End If
            Dim intLeader(-1) As Int32
            intSampPerSym = 12000 \ intBaud
            'For FEC transmission the computed leader length = MCB.Leader length
            intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * ((8 * (bytEncodedData.Length - 2)) \ 3)
            intLeader = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)

            ' Create the leader
            intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
            Dim intSamples(intSampleLen - 1) As Int32
            Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
            intSamplePtr = intLeader.Length

            ' Create the 10symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
            ' No reference needed for FSK
            ' note revised To accomodate 1 parity symbol per byte (10 symbols total)
            For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
                bytMask = &HC0
                For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                    If k < 4 Then
                        bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                    Else
                        bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                    End If
                    For n As Integer = 0 To 239
                        If ((5 * j + k) Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                        End If
                    Next n
                    intSamplePtr += 240
                    bytMask = bytMask >> 2
                Next k
            Next j

            ' No reference needed for FSK
            intDataPtr = 2 ' point to first data byte past frame type
            For m As Integer = 0 To intDataBytesPerCar - 1
                If m Mod 3 = 0 Then
                    intThreeBytes = bytEncodedData(intDataPtr)
                    intThreeBytes = (intThreeBytes << 8) + bytEncodedData(intDataPtr + 1)
                    intThreeBytes = (intThreeBytes << 8) + bytEncodedData(intDataPtr + 2)
                    intMask = &HE00000
                    For k As Integer = 0 To 7
                        bytSymToSend = CByte((intMask And intThreeBytes) >> (3 * (7 - k)))
                        For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                            ' note value of "+ 4" below allows using 16FSK template for 8FSK using only the "inner" 8 tones around 1500
                            If (k Mod 2 = 0) Then
                                intSamples(intSamplePtr + n) = intFSK25bdCarTemplate(bytSymToSend + 4, n) ' Symbol vlaues 4- 11 (surrounding 1500 Hz)  
                            Else
                                intSamples(intSamplePtr + n) = -intFSK25bdCarTemplate(bytSymToSend + 4, n) ' sign reversal eliminates phase discontinuity
                            End If
                        Next n
                        intMask = intMask >> 3
                        intSamplePtr += intSampPerSym
                    Next k
                End If
                intDataPtr += 1
            Next m
            intSamples = FSXmtFilter200_1500Hz(intSamples) ' filter for 200 Hz bandwidth
            Return intSamples
        Catch ex As Exception
            Logs.Exception("[Mod8FSKdata] bytFrame= H" & Format(bytFrame, "X") & "  DataLen=" & bytEncodedData.Length.ToString & "  Err: " & ex.ToString)
        End Try
        Return Nothing
    End Function 'Mod8FSKData

    ' Function to encoded data  for 16FSK modes and create the integer array of 32 bit samples suitable for playing 
    Public Function Mod16FSKData(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()

        Dim intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytSymToSend, bytMask, bytMinQualThresh As Byte
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        Try
            If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytMinQualThresh, strType) Then
                Logs.Exception("[Mod16FSKData] no data for frame type H" & Format(bytFrame, "x"))
                Return Nothing
            End If
            If strMod <> "16FSK" Then
                Logs.Exception("[Mod16FSKData] Frame Type: H" & Format(bytFrame, "X") & " is not 16FSK")
                Return Nothing
            End If
            strLastWavStream = strType
            If intLeaderLen = 0 Then
                intLeaderLenMS = MCB.LeaderLength
            Else
                intLeaderLenMS = intLeaderLen
            End If
            Dim intLeader(-1) As Int32
            intSampPerSym = 480
            'For FEC transmission the computed leader length = MCB.Leader length
            intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (2 * (bytEncodedData.Length - 2))
            intLeader = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)

            ' Create the leader
            intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
            Dim intSamples(intSampleLen - 1) As Int32
            Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
            intSamplePtr = intLeader.Length

            ' Create the 10symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
            ' No reference needed for 4FSK
            ' note revised To accomodate 1 parity symbol per byte (10 symbols total)
            For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
                bytMask = &HC0
                For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                    If k < 4 Then
                        bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                    Else
                        bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                    End If
                    For n As Integer = 0 To 239
                        If ((5 * j + k) Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                        End If
                    Next n
                    intSamplePtr += 240
                    bytMask = bytMask >> 2
                Next k
            Next j

            ' No reference needed for 16FSK
            intDataPtr = 2
            For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
                bytMask = &HF0 ' Initialize mask each new data byte
                For k As Integer = 0 To 1 ' for 2 symbol values per byte of data
                    bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (4 * (1 - k)) ' Values 0 - 15
                    For n As Integer = 0 To intSampPerSym - 1
                        If (k Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK25bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK25bdCarTemplate(bytSymToSend, n)
                        End If
                    Next n
                    bytMask = bytMask >> 4
                    intSamplePtr += intSampPerSym
                Next k
                intDataPtr += 1
            Next m
            intSamples = FSXmtFilter500_1500Hz(intSamples)
            Return intSamples
        Catch ex As Exception
            Logs.Exception("[Mod16FSKData] bytFrame= H" & Format(bytFrame, "X") & "  DataLen=" & bytEncodedData.Length.ToString & "  Err: " & ex.ToString)
        End Try
        Return Nothing
    End Function 'Mod16FSKData

    ' Function to Modulate data encoded for 4FSK High baud rate and create the integer array of 32 bit samples suitable for playing 
    Public Function Mod4FSK600BdData(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()

        Dim intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytSymToSend, bytMask, bytMinQualThresh As Byte
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        Try
            If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytMinQualThresh, strType) Then
                Logs.Exception("[Mod4FSK600BdData] no data for frame type H" & Format(bytFrame, "x"))
                Return Nothing
            End If
            strLastWavStream = strType
            If intLeaderLen = 0 Then
                intLeaderLenMS = MCB.LeaderLength
            Else
                intLeaderLenMS = intLeaderLen
            End If
            Dim intLeader(-1) As Int32
            intSampPerSym = 12000 \ intBaud
            'For FEC transmission the computed leader length = MCB.Leader length
            intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (bytEncodedData.Length - 2))
            intLeader = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)

            ' Create the leader
            intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
            Dim intSamples(intSampleLen - 1) As Int32
            Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
            intSamplePtr = intLeader.Length

            ' Create the 10symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
            ' No reference needed for 4FSK
            ' note revised To accomodate 1 parity symbol per byte (10 symbols total)
            For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
                bytMask = &HC0
                For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                    If k < 4 Then
                        bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                    Else
                        bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                    End If
                    For n As Integer = 0 To 239
                        If ((5 * j + k) Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                        End If
                    Next n
                    intSamplePtr += 240
                    bytMask = bytMask >> 2
                Next k
            Next j

            ' No reference needed for 4FSK (will have to expand to hande 3 sets of 200 data bytes each (to accomodate RS limitation of 254 max bytes)
            intDataPtr = 2
            For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
                bytMask = &HC0 ' Initialize mask each new data byte
                For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                    bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (2 * (3 - k)) ' Values 0 - 15
                    For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                        intSamples(intSamplePtr + n) = intFSK600bdCarTemplate(bytSymToSend, n)
                    Next n
                    bytMask = bytMask >> 2
                    intSamplePtr += intSampPerSym
                Next k
                intDataPtr += 1
            Next m
            intSamples = FSXmtFilter2000_1500Hz(intSamples)
            Return intSamples
        Catch ex As Exception
            Logs.Exception("[Mod4FSK600BdData] bytFrame= H" & Format(bytFrame, "X") & "  DataLen=" & bytEncodedData.Length.ToString & "  Err: " & ex.ToString)
        End Try
        Return Nothing
    End Function 'Mod4FSK600BdData

    ' Function to Modulate data encoded for 4FSK and create the integer array of 32 bit samples suitable for playing 
    Public Function Mod4FSKData(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()
        ' Function works for 1, 2 or 4 simultaneous carriers 
        Dim intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytSymToSend, bytMask, bytMinQualThresh As Byte
        Dim bytLastSym(8) As Byte ' Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 
        Dim dblCarScalingFactor As Double
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        Try
            If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytMinQualThresh, strType) Then
                Logs.Exception("[Mod4FSK] no data for frame type H" & Format(bytFrame, "x"))
                Return Nothing
            End If
            If strMod <> "4FSK" Then
                Logs.Exception("[Mod4FSKData] Frame Type: H" & Format(bytFrame, "X") & " is not 4FSK")
                Return Nothing
            End If
            If Not (strType = "DataACK" Or strType = "DataNAK" Or strType = "IDFrame" Or strType.StartsWith("ConReq") Or strType.StartsWith("ConAck")) Then
                strLastWavStream = strType
            End If
            If intLeaderLen = 0 Then
                intLeaderLenMS = MCB.LeaderLength
            Else
                intLeaderLenMS = intLeaderLen
            End If
            Dim intLeader(-1) As Int32
            Select Case intBaud
                Case 50
                    intSampPerSym = 240
                    ' For FEC transmission the computed leader length = MCB.Leader length
                    intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (bytEncodedData.Length - 2) \ intNumCar)
                    intLeader = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)
                Case 100
                    intSampPerSym = 120
                    intSampleLen = intLeaderLenMS * 12 + 240 * 10 + intSampPerSym * (4 * (bytEncodedData.Length - 2) \ intNumCar)
                    intLeader = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)
            End Select
            ' Create the leader
            intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
            Dim intSamples(intSampleLen - 1) As Int32
            Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
            intSamplePtr = intLeader.Length
            ' Create the 8 symbols (16 bit) 50 baud 4FSK frame type with Implied SessionID
            ' No reference needed for 4FSK

            ' note revised To accomodate 1 parity symbol per byte (10 symbols total)
            For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
                bytMask = &HC0
                For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                    If k < 4 Then
                        bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                    Else
                        bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                    End If
                    For n As Integer = 0 To 239
                        If ((5 * j + k) Mod 2 = 0) Then
                            intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                        Else
                            intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                        End If
                    Next n
                    intSamplePtr += 240
                    bytMask = bytMask >> 2
                Next k
            Next j

            ' No reference needed for 4FSK
            intDataPtr = 2
            Select Case intNumCar
                Case 1 ' use carriers 0-4
                    dblCarScalingFactor = 1.0 '  (scaling factors determined emperically to minimize crest factor) 
                    For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
                        bytMask = &HC0 ' Initialize mask each new data byte
                        For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                            bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (2 * (3 - k)) ' Values 0-3
                            For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                                If (k Mod 2 = 0) Then
                                    If intBaud = 50 Then
                                        intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                                    Else
                                        intSamples(intSamplePtr + n) = intFSK100bdCarTemplate(bytSymToSend, n)
                                    End If
                                Else
                                    If intBaud = 50 Then
                                        intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n)
                                    Else
                                        intSamples(intSamplePtr + n) = -intFSK100bdCarTemplate(bytSymToSend, n)
                                    End If
                                End If
                            Next n
                            bytMask = bytMask >> 2
                            intSamplePtr += intSampPerSym
                        Next k
                        intDataPtr += 1
                    Next m
                    If intBaud = 50 Then
                        intSamples = FSXmtFilter200_1500Hz(intSamples)
                    ElseIf intBaud = 100 Then
                        intSamples = FSXmtFilter500_1500Hz(intSamples)
                    End If
                Case 2 ' use carriers 8-15 (100 baud only)
                    dblCarScalingFactor = 0.51 '  (scaling factors determined emperically to minimize crest factor)
                    For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
                        bytMask = &HC0 ' Initialize mask each new data byte
                        For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                            For n As Integer = 0 To intSampPerSym - 1 ' for all the samples of a symbol for 2 carriers 
                                ' First carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = intFSK100bdCarTemplate(8 + bytSymToSend, n)
                                ' Second carrier
                                bytSymToSend = (bytMask And bytEncodedData(intDataPtr + intDataBytesPerCar)) >> (2 * (3 - k)) ' Values 0-3
                                intSamples(intSamplePtr + n) = dblCarScalingFactor * (intSamples(intSamplePtr + n) + intFSK100bdCarTemplate(12 + bytSymToSend, n))
                            Next n
                            bytMask = bytMask >> 2
                            intSamplePtr += intSampPerSym
                        Next k
                        intDataPtr += 1
                    Next m
                    intSamples = FSXmtFilter1000_1500Hz(intSamples)
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
            End Select
            Return intSamples
        Catch ex As Exception
            Logs.Exception("[Mod4FSKData] bytFrame= H" & Format(bytFrame, "X") & "  DataLen=" & bytEncodedData.Length.ToString & "  Err: " & ex.ToString)
        End Try
        Return Nothing
    End Function 'Mod4FSKData

    '  Function to modulate 4FSK ID frame and create the integer array of 32 bit samples suitable for playing 
    Public Function Mod4FSKFrameID(bytFrame As Byte, bytEncodedData() As Byte, Optional intLeaderLen As Int32 = 0) As Int32()

        Dim intNumCar, intBaud, intDataLen, intRSLen, intSampleLen, intSamplePtr, intDataPtr, intSampPerSym, intDataBytesPerCar As Int32
        Dim intCarIndex, intCarStartIndex As Integer
        Dim blnOdd As Boolean
        Dim strType As String = ""
        Dim strMod As String = ""
        Dim bytSymToSend, bytMask, bytMinQualThresh As Byte
        Dim bytLastSym(8) As Byte ' Holds the last symbol sent (per carrier). bytLastSym(4) is 1500 Hz carrier (only used on 1 carrier modes) 
        Dim dblCarScalingFactor As Double
        Dim intPeakAmp As Int32
        Dim intMask As Int32 = 0
        Dim intLeaderLenMS As Int32

        If Not objFrameInfo.FrameInfo(bytFrame, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytMinQualThresh, strType) Then
            Logs.Exception("[Mod4FSK] no data for frame type H" & Format(bytFrame, "x"))
            Return Nothing
        End If
        If Not (strType = "DataACK" Or strType = "DataNAK") Then
            strLastWavStream = strType
        End If
        If intLeaderLen = 0 Then
            intLeaderLenMS = MCB.LeaderLength
        Else
            intLeaderLenMS = intLeaderLen
        End If
        Select Case intBaud
            Case 50
                intSampPerSym = 240
                intSampleLen = intLeaderLenMS * 12 + intSampPerSym * (2 + (4 * (2 + (bytEncodedData.Length - 2) \ intNumCar)))
        End Select
        Select Case intNumCar
            Case 1 : intCarStartIndex = 4 : dblCarScalingFactor = 1.0 ' Starting at 1500 Hz  (scaling factors determined emperically to minimize crest factor)  TODO:  needs verification
        End Select
        intDataBytesPerCar = (bytEncodedData.Length - 2) \ intNumCar
        Dim intSamples(intSampleLen) As Int32
        Dim intLeader() As Int32 = GetTwoToneLeaderWithSync(intLeaderLenMS \ 10)
        Array.Copy(intLeader, 0, intSamples, 0, intLeader.Length) 'copy the leader + sync
        intSamplePtr = intLeader.Length
        ' Create the 10 symbols (16 bit + 4 bits parity ) 50 baud 4PSK frame type with RS correction and Implied SessionID
        ' No reference needed for 4FSK
        ' note revised To accomodate 1 parity symbol per byte (10 symbols total)
        For j As Integer = 0 To 1 ' for the 2 bytes of the frame type
            bytMask = &HC0
            For k As Integer = 0 To 4 ' for 5 symbols per byte (4 data + 1 parity)
                If k < 4 Then
                    bytSymToSend = (bytMask And bytEncodedData(j)) >> (2 * (3 - k))
                Else
                    bytSymToSend = objFrameInfo.ComputeTypeParity(bytEncodedData(0))
                End If
                For n As Integer = 0 To 239
                    If ((5 * j + k) Mod 2 = 0) Then
                        intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                    Else
                        intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n) ' -sign insures no phase discontinuity at symbol boundaries
                    End If
                Next n
                intSamplePtr += 240
                bytMask = bytMask >> 2
            Next k
        Next j
        If bytEncodedData.Length = 2 Then Return FSXmtFilter200_1500Hz(intSamples)
        intPeakAmp = 0
        intCarIndex = intCarStartIndex ' initialize to correct starting carrier
        ' No reference needed for 4FSK
        intDataPtr = 2
        For m As Integer = 0 To intDataBytesPerCar - 1  ' For each byte of input data 
            bytMask = &HC0 ' Initialize mask each new data byte
            For k As Integer = 0 To 3 ' for 4 symbol values per byte of data
                intCarIndex = intCarStartIndex ' initialize the carrrier index

                bytSymToSend = (bytMask And bytEncodedData(intDataPtr)) >> (2 * (3 - k)) ' Values 0-3

                For n As Integer = 0 To intSampPerSym - 1 ' Sum for all the samples of a symbols 
                    If intBaud = 50 And (k Mod 2 = 0) Then
                        intSamples(intSamplePtr + n) = intFSK50bdCarTemplate(bytSymToSend, n)
                    Else
                        intSamples(intSamplePtr + n) = -intFSK50bdCarTemplate(bytSymToSend, n)
                    End If
                Next n
                bytMask = bytMask >> 2
                intSamplePtr += intSampPerSym
            Next k
            intDataPtr += 1
        Next m
        Return FSXmtFilter200_1500Hz(intSamples)
    End Function ' Mod4FSKFrameID

    ' Function to extract an 8PSK symbol from an encoded data array
    Private Function GetSym8PSK(intDataPtr As Integer, k As Integer, intCar As Integer, ByRef bytEncodedData() As Byte, intDataBytesPerCar As Int32) As Byte

        Dim int3Bytes As Int32 = bytEncodedData(intDataPtr + intCar * intDataBytesPerCar)
        Dim intMask As Int32 = &H7
        Dim bytSym As Byte
        int3Bytes = int3Bytes << 8
        int3Bytes += bytEncodedData(intDataPtr + intCar * intDataBytesPerCar + 1)
        int3Bytes = int3Bytes << 8
        int3Bytes += bytEncodedData(intDataPtr + intCar * intDataBytesPerCar + 2) ' now have 3 bytes, 24 bits or 8 8PSK symbols 
        intMask = intMask << (3 * (7 - k))
        bytSym = (intMask And int3Bytes) >> (3 * (7 - k))
        Return bytSym
    End Function ' GetSym8PSK

    '  Function to encode ConnectRequest frame 
    Public Function EncodeARQConRequest(strMyCallsign As String, strTargetCallsign As String, strBandwidth As String, ByRef strFilename As String) As Byte()
        '  Encodes a 4FSK 200 Hz BW Connect Request frame ( ~ 1950 ms with default leader/trailer) 
        '  Returns a byte array ready for modulation using MOD500_4FSK. strFilename is updated by reference for display on TNC Form. 
        If strTargetCallsign.Trim.ToUpper = "CQ" Then ' skip syntax checking for psuedo call "CQ"
            If Not CheckValidCallsignSyntax(strMyCallsign) Then
                Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)
                Return Nothing
            End If
        Else
            If Not (CheckValidCallsignSyntax(strTargetCallsign) Or CheckValidCallsignSyntax(strMyCallsign)) Then
                Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)
                Return Nothing
            End If
        End If

        Dim bytReturn(2 + 12 + 2 - 1) As Byte ' 2 Frame type bytes + 12 Callsign bytes + 2 CRC bytes    
        ' check the bandwidth  
        If strBandwidth.ToUpper.StartsWith("200M") Then
            bytReturn(0) = &H31
        ElseIf strBandwidth.ToUpper.StartsWith("500M") Then
            bytReturn(0) = &H32
        ElseIf strBandwidth.StartsWith("1000M") Then
            bytReturn(0) = &H33
        ElseIf strBandwidth.StartsWith("2000M") Then
            bytReturn(0) = &H34
        ElseIf strBandwidth.StartsWith("200F") Then
            bytReturn(0) = &H35
        ElseIf strBandwidth.StartsWith("500F") Then
            bytReturn(0) = &H36
        ElseIf strBandwidth.StartsWith("1000F") Then
            bytReturn(0) = &H37
        ElseIf strBandwidth.StartsWith("2000F") Then
            bytReturn(0) = &H38
        Else
            Logs.Exception("[EncodeModulate.EncodeFSK500_1S] Bandwidth error.  Bandwidth = " & strBandwidth)
            Return Nothing
        End If

        strFilename = objFrameInfo.Name(bytReturn(0)) & "_" & strMyCallsign.Trim.ToUpper & ">" & strTargetCallsign.Trim.ToUpper
        strLastWavStream = strFilename
        bytReturn(1) = bytReturn(0) Xor &HFF 'Connect Request always uses session ID of &HFF
        ' Modified May 24, 2015 to use RS instead of 2 byte CRC. (same as ID frame)
        Dim bytToRS(11) As Byte
        Dim bytCalls() As Byte = CompressCallsign(strMyCallsign)
        Array.Copy(bytCalls, 0, bytToRS, 0, bytCalls.Length)
        bytCalls = CompressCallsign(strTargetCallsign)
        Array.Copy(bytCalls, 0, bytToRS, 6, bytCalls.Length)
        objRS8.MaxCorrections = 1 ' with two Parity bytes can correct 1 error. 
        Dim bytRSEncoded() As Byte = objRS8.RSEncode(bytToRS) 'Generate the RS encoding ...now 14 bytes total
        Array.Copy(bytRSEncoded, 0, bytReturn, 2, bytRSEncoded.Length)
        Return bytReturn
    End Function ' ConnectRequest

    '  Function to encodes ID frame 
    Public Function Encode4FSKIDFrame(strMyCallsign As String, strGridSquare As String, ByRef strFilename As String) As Byte()
        '  Encodes an ID frame ( ~ 1000 ms with default leader/trailer) 
        '  Returns a byte array ready for modulation using . strFilename is updated by reference for display on TNC Form. 
        If (Not CheckValidCallsignSyntax(strMyCallsign)) Then
            Logs.Exception("[EncodeModulate.EncodeIDFrame] Illegal Callsign syntax or Gridsquare length. MyCallsign = " & strMyCallsign & ", Gridsquare = " & strGridSquare)
            Return Nothing
        End If
        Try
            Dim bytReturn(2 + 6 + 6 + 2 - 1) As Byte ' 4 Frame type bytes + 6 Call sign (compressed)  + 8 Grid square bytes ( compressed)  +  2 bytes CRC 
            bytReturn(0) = &H30
            strFilename = objFrameInfo.Name(bytReturn(0)) & "_" & strMyCallsign.Trim.ToUpper & " [" & strGridSquare.Trim & "]"
            strLastWavStream = strFilename
            bytReturn(1) = bytReturn(0) Xor &HFF
            ' Modified May 9, 2015 to use RS instead of 2 byte CRC.
            Dim bytToRS(11) As Byte
            Dim bytCalls() As Byte = CompressCallsign(strMyCallsign)
            Array.Copy(bytCalls, 0, bytToRS, 0, bytCalls.Length)
            If Not IsNothing(strGridSquare) Then
                bytCalls = CompressGridSquare(strGridSquare.PadRight(8, " "))  ' this uses compression to accept 4, 6, or 8 character Grid squares.
                Array.Copy(bytCalls, 0, bytToRS, 6, bytCalls.Length)
            End If
            objRS8.MaxCorrections = 1 ' with two Parity bytes can correct 1 error. 
            Dim bytRSEncoded() As Byte = objRS8.RSEncode(bytToRS) 'Generate the RS encoding ...now 14 bytes total
            Array.Copy(bytRSEncoded, 0, bytReturn, 2, bytRSEncoded.Length)
            Return bytReturn
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.EncodeIDFrame] Err: " & ex.ToString)
            Return Nothing
        End Try
    End Function ' Encode4FSKIDFrame

    '  Funtion to encodes a short 4FSK 50 baud Control frame  (2 bytes total) BREAK, END, DISC, IDLE, ConRejBusy, ConRegBW  
    Public Function Encode4FSKControl(ByVal intFrameCode As Integer, ByRef strFileName As String, bytSessionID As Byte) As Byte()
        '  Encodes a short control frame (normal length ~320 ms with default 160 ms leader+trailer) 
        If Not objFrameInfo.IsShortControlFrame(intFrameCode) Then
            Logs.Exception("[EncodeModulate.EncodeFSKControl] Illegal control frame code: H" & Format(intFrameCode, "X"))
            Return Nothing
        End If
        Dim bytReturn(1) As Byte
        bytReturn(0) = intFrameCode
        bytReturn(1) = intFrameCode Xor bytSessionID
        Return bytReturn
    End Function  ' Encode4FSKControl

    '  Function to encode a CONACK frame with Timing data  (6 bytes total)  
    Public Function EncodeConACKwTiming(ByVal intFrameCode As Integer, intRcvdLeaderLenMs As Int32, ByRef strFileName As String, bytSessionID As Byte) As Byte()
        '  Encodes a Connect ACK with one byte Timing info. (Timing info repeated 2 times for redundancy) 
        If intFrameCode < &H39 Or intFrameCode > &H3C Then
            Logs.Exception("[EncodeConACKwTiming] Illegal Frame code: " & Format(intFrameCode, "X"))
            Return Nothing
        End If

        Dim bytTiming As Byte = CByte(Min(255, Round(intRcvdLeaderLenMs / 10))) ' convert to 10s of ms. 
        Dim bytReturn(4) As Byte ' 1 byte frame Type + Frame Type XOR ID + 1 byte Timing (repeated 2 times) 
        If intRcvdLeaderLenMs > 2550 Or intRcvdLeaderLenMs < 0 Then
            Logs.Exception("[EncodeConACKwTiming] Timing value out of range: " & intRcvdLeaderLenMs.ToString & " continue with forced value = 0")
            bytTiming = 0
        End If
        bytReturn(0) = intFrameCode
        strLastWavStream = objFrameInfo.Name(intFrameCode) & "_" & CInt(10 * bytTiming).ToString & " ms"
        strFileName = strLastWavStream
        bytReturn(1) = intFrameCode Xor bytSessionID
        bytReturn(2) = bytTiming ' Repeat timing information (leader length received in 10's of ms  0 - 2550 ms).  Normal range will be 100 - 1000 ms.
        bytReturn(3) = bytTiming
        bytReturn(4) = bytTiming
        Return bytReturn
    End Function  ' EncodeConACKwTiming

    ' Function to encode an ACK control frame  (2 bytes total) ...with 5 bit Quality code 
    Public Function EncodeDATAACK(intQuality As Integer, ByRef strFileName As String, bytSessionID As Byte) As Byte()
        '  Encodes intQuality and DataACK frame (normal length ~320 ms with default leader/trailer)
        Dim intScaledQuality As Int32
        Dim bytReturn(1) As Byte
        intScaledQuality = Max(0, CInt(intQuality / 2) - 19) ' scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
        bytReturn(0) = &HE0 + intScaledQuality
        bytReturn(1) = bytReturn(0) Xor bytSessionID
        strLastWavStream = "DataACK_Q" & intQuality.ToString
        strFileName = strLastWavStream
        Return bytReturn
    End Function  ' EncodeDATAACK

    '  Function to encode a NAK frame  (2 bytes total) ...with 5 bit Quality code 
    Public Function EncodeDATANAK(intQuality As Integer, ByRef strFileName As String, bytSessionID As Byte) As Byte()
        '  Encodes intQuality and DATANAK frame (normal length ~260 ms with default leader/trailer) 
        Dim bytReturn(1) As Byte
        Dim intScaledQuality As Int32
        intScaledQuality = Max(0, CInt(intQuality / 2) - 19) ' scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
        bytReturn(0) = &H0 + intScaledQuality
        bytReturn(1) = bytReturn(0) Xor bytSessionID
        strLastWavStream = "DataNAK_Q" & intQuality.ToString
        strFileName = strLastWavStream
        Return bytReturn
    End Function  ' EncodeDATANAK

    ' Subroutine to make a CW ID Wave File
    Public Sub CWID(ByVal strID As String, ByRef intSamples() As Int32, blnPlay As Boolean)
        ' This generates a phase synchronous FSK MORSE keying of strID
        ' FSK used to maintain VOX on some sound cards
        ' Sent at 90% of  max ampllitude
        Dim strAlphabet As String = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/"
        ' Look up table for strAlphabet...each bit represents one dot time, 3 adjacent dots = 1 dash
        ' one dot spacing between dots or dashes
        Dim intCW() As Integer = {&H17, &H1D5, &H75D, &H75, &H1, &H15D, _
           &H1DD, &H55, &H5, &H1777, &H1D7, &H175, _
           &H77, &H1D, &H777, &H5DD, &H1DD7, &H5D, _
           &H15, &H7, &H57, &H157, &H177, &H757, _
           &H1D77, &H775, &H77777, &H17777, &H5777, &H1577, _
           &H557, &H155, &H755, &H1DD5, &H7775, &H1DDDD, &H1D57, &H1D57}

        If strID.IndexOf("-") <> -1 Then
            ' strip off the -ssid for CWID
            strID = strID.Substring(0, strID.IndexOf("-"))
        End If

        Dim dblHiPhaseInc As Double = 2 * PI * 1609.375 / 12000 ' 1609.375 Hz High tone
        Dim dblLoPhaseInc As Double = 2 * PI * 1390.625 / 12000 ' 1390.625  low tone
        Dim dblHiPhase As Double
        Dim dblLoPhase As Double
        Dim intDotSampCnt As Integer = 768 ' about 12 WPM or so (should be a multiple of 256
        Dim intDot(intDotSampCnt - 1) As Int16
        Dim intSpace(intDotSampCnt - 1) As Int16

        ' Generate the dot samples (high tone) and space samples (low tone) 
        For i As Integer = 0 To intDotSampCnt - 1
            intSpace(i) = CShort(Sin(dblLoPhase) * 0.9 * intAmp)
            intDot(i) = CShort(Sin(dblHiPhase) * 0.9 * intAmp)
            dblHiPhase += dblHiPhaseInc
            If dblHiPhase > 2 * PI Then dblHiPhase -= 2 * PI
            dblLoPhase += dblLoPhaseInc
            If dblLoPhase > 2 * PI Then dblLoPhase -= 2 * PI
        Next i
        Dim intMask As Int32
        Dim intWav((6 * intDotSampCnt) - 1) As Int32
        ' Generate leader for VOX 6 dots long
        For k As Integer = 6 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k

        For j As Integer = 0 To strID.Length - 1 ' for each character in the string
            intMask = &H40000000
            Dim intIdx As Integer = strAlphabet.IndexOf(strID.ToUpper.Substring(j, 1))
            If intIdx = -1 Then
                ' process this as a space adding 6 dots worth of space to the wave file
                ReDim Preserve intWav(intWav.Length + (6 * intDotSampCnt) - 1)
                For k As Integer = 6 To 1 Step -1
                    Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
                Next k
            Else
                While intMask > 0 ' search for the first non 0 bit
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Exit While ' intMask is pointing to the first non 0 entry
                    Else
                        intMask = intMask \ 2 ' Right shift mask
                    End If
                End While
                While intMask > 0
                    ReDim Preserve intWav(intWav.Length + intDotSampCnt - 1)
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Array.Copy(intDot, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    Else
                        Array.Copy(intSpace, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    End If
                    intMask = intMask \ 2 ' Right shift mask
                End While
            End If
            ' add 3 dot spaces for inter letter spacing
            ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
            For k As Integer = 3 To 1 Step -1
                Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
            Next k
        Next
        ' add 3 spaces for the end tail
        ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
        For k As Integer = 3 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k
        If Not blnPlay Then
            intSamples = intWav
            Exit Sub
        End If
        ' Convert the integer array to bytes
        Dim aryWave(2 * intWav.Length - 1) As Byte
        For j As Integer = 0 To intWav.Length - 1
            aryWave(2 * j) = CByte(intWav(j) And &HFF) ' LSByte
            aryWave(1 + 2 * j) = CByte((intWav(j) And &HFF00) \ 256) ' MSbyte
        Next j
        ' *********************************
        ' Debug code to look at wave file 
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWave.WriteRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\CWID.wav", 12000, 16, aryWave)
        '' End of debug code
        '************************************
        objWave.WriteRIFFStream(memWaveStream, 12000, 16, aryWave)
    End Sub '  CWID

    ' Function to apply 200 Hz filter for transmit  
    Public Function FSXmtFilter200_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for PSK 200 Hz modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 3 100 Hz wide sections centered on 1500 Hz  (~200 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables

        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(18) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(18) As Double ' resonator outputs
        Dim dblZout_1(18) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(18) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        ' Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting

        ' Initialize the coefficients
        If dblCoef(15) = 0 Then
            For i As Integer = 14 To 16
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

                For j As Integer = 14 To 16   ' calculate output for 3 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 

                    If i >= intFilLen Then
                        If j = 14 Or j = 16 Then
                            intFilteredSamples(i - intFilLen) += 0.7389 * dblZout_0(j)
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
                End If
            Next i

            ' *********************************
            ' Debug code to look at filter output
            'Dim objWT As New WaveTools
            'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = False Then
            '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
            'End If
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered" & "_XmtFilter200.wav", 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered" & "_XmtFilter200.wav", 12000, 16, dblFilteredSamples)
            ' End of debug code
            '************************************

        Catch ex As Exception
            Logs.Exception("[EncodeModulate.FSXmtFilterPSK200_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
    End Function 'FSXmtFilter200_1500Hz

    ' Function to apply 500 Hz filter for transmit 
    Public Function FSXmtFilter500_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for FSK modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 7 100 Hz wide sections centered on 1500 Hz  (~500 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables
        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(18) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(18) As Double ' resonator outputs
        Dim dblZout_1(18) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(18) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        ' Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting
        Dim intPeakSample As Int32 = 0
        ' Initialize the coefficients
        If dblCoef(18) = 0 Then
            For i As Integer = 12 To 18
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

                For j As Integer = 12 To 18   ' calculate output for 7 resonators 
                    dblZout_0(j) = dblZComb + dblCoef(j) * dblZout_1(j) - dblR2 * dblZout_2(j)
                    dblZout_2(j) = dblZout_1(j)
                    dblZout_1(j) = dblZout_0(j)
                    ' scale each by transition coeff and + (Even) or - (Odd) 
                    ' Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
                    ' practical range of scaling .05 to .25
                    ' Scaling also accomodates for the filter "gain" of approx 60. 

                    If i >= intFilLen Then
                        If j = 12 Or j = 18 Then
                            intFilteredSamples(i - intFilLen) += 0.10601 * dblZout_0(j)
                        ElseIf j = 13 Or j = 17 Then
                            intFilteredSamples(i - intFilLen) -= 0.59383 * dblZout_0(j)
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
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\UnFiltered" & "_XmtFil500.wav", 12000, 16, dblUnfilteredSamples)
            'objWT.WriteFloatingRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\Filtered" & "_XmtFil500.wav", 12000, 16, dblFilteredSamples)
            'End of debug code
            '************************************
        Catch ex As Exception
            Logs.Exception("[EncodeModulate.FSXmtFilterFSK500_1500Hz] Exception: " & ex.ToString)
            Return Nothing
        End Try
        Return intFilteredSamples
    End Function 'FSXmtFilter500_1500Hz

    ' Function to apply 1000 Hz filter for transmit 
    Public Function FSXmtFilter1000_1500Hz(ByRef intNewSamples() As Int32) As Int32()

        ' Used for FSK modulation XMIT filter  
        ' assumes sample rate of 12000
        ' implements 11 100 Hz wide sections centered on 1500 Hz  (~1000 Hz wide @ - 30dB centered on 1500 Hz)
        ' FSF (Frequency Selective Filter) variables
        AddTrailer(intNewSamples) ' add the trailer before filtering

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(20) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(20) As Double ' resonator outputs
        Dim dblZout_1(20) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(20) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        'Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting
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

        Static dblR As Double = 0.9995  ' insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
        Static intN As Integer = 120  ' Length of filter 12000/100
        Static dblRn As Double = dblR ^ intN
        Static dblR2 As Double = dblR ^ 2
        Static dblCoef(25) As Double 'the coefficients
        Dim dblZin, dblZin_1, dblZin_2, dblZComb As Double  ' Used in the comb generator
        ' The resonators 
        Dim dblZout_0(25) As Double ' resonator outputs
        Dim dblZout_1(25) As Double ' resonator outputs delayed one sample
        Dim dblZout_2(25) As Double  ' resonator outputs delayed two samples
        Static intFilLen As Integer = intN \ 2
        Dim intFilteredSamples(intNewSamples.Length - 1) As Int32 '  Filtered samples
        'Dim dblUnfilteredSamples(intNewSamples.Length - 1) As Double 'for debug wave plotting
        ' Dim dblFilteredSamples(intNewSamples.Length - 1) As Double ' for debug wave plotting
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

    Protected Overrides Sub Finalize()
        MyBase.Finalize()
    End Sub
End Class
