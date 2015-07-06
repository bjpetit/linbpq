#Region "Imports"
Imports System.Text.RegularExpressions
Imports System.IO.Ports
Imports System.IO
Imports System.Text
Imports System.Threading
Imports Microsoft.DirectX
Imports Microsoft.DirectX.DirectSound
#End Region ' Imports

Public Module Globals

    Public ALSA As Boolean = False


#Region "Structures"

    ' ModemControlBlock contains all TNC setup parametes (saved to ini file) 
    Public Structure ModemControlBlock
        Public Callsign As String
        Public GridSquare As String
        Public CaptureDevice As String
        Public PlaybackDevice As String
        Public HostTCPIP As Boolean
        Public TCPAddress As String
        Public TCPPort As Int16
        Public HostSerial As Boolean
        Public SerCOMPort As String
        Public SerBaud As Integer
        Public HostBlueTooth As Boolean
        Public HostPairing As String
        Public ARQBandwidth As String
        Public ARQTimeout As Integer
        Public ARQConReqRepeats As Integer
        Public ProtocolMode As String
        Public CWID As Boolean
        Public DriveLevel As Int16
        Public DebugLog As Boolean
        Public Squelch As Int16
        Public StartMinimized As Boolean
        Public CommandTrace As Boolean
        Public LeaderLength As Int16
        Public TrailerLength As Int16
        Public AccumulateStats As Boolean
        Public DisplayWaterfall As Boolean
        Public DisplaySpectrum As Boolean
        Public SecureHostLogin As Boolean
        Public Password As String
        Public TuningRange As Int16
        Public LinkedToHost As Boolean
        Public FECMode As String
        Public FECRepeats As Int16
        Public FECId As Boolean
        Public AuxCalls() As String
        Public DisplayFreq As Double
    End Structure  'ModemControlBlock

    '  Radio control block contains all Radio setup parametes (saved to ini file) 
    Public Structure RadioControlBlock
        Public RadioControl As Boolean 'True to enable radio control 
        Public Mode As String  '  "USB", "USBD", "FM"
        Public CtrlPort As String ' Radio COM port
        Public CtrlPortBaud As Integer ' 4800 
        Public CtrlPortRTS As Boolean ' Enable RTS on Radio Com Port
        Public CtrlPortDTR As Boolean ' Enable DTR on Radio Com Port
        Public Ant As Integer  ' 0 (no control) 1, or 2
        Public Filter As Integer ' bandwidth in Hz, 0 implies no bandwidth control 
        Public Frequency As Integer  ' Frequency in integer Hz
        Public Model As String ' Radio model from drop down list
        Public IcomAdd As String ' Icom address (Hex 00 - FF)
        Public InternalSoundCard As Boolean ' use internal sound card (Kenwood 590, Icom 7100, 7200, 7600, 9100)
        Public InternalTuner As Boolean ' Some radios which support tuner
        Public PTTPort As String ' Serial port or "External" for SignaLink type or VOX interface
        Public PTTRTS As Boolean '  Enable PTT Port RTS for PTT keying
        Public PTTDTR As Boolean '  Enable PTT Port DTR for PTT keying
    End Structure 'RadioControlBlock

    '  Structure for passing TNC status via synchronous queue
    Public Structure Status
        ' Structure for passing interface form control updates via thread safe synchronized queue... 
        Public ControlName As String
        Public Text As String
        Public Value As Integer
        Public BackColor As System.Drawing.Color
    End Structure  ' Status

    ' Structure for ARQ connection 
    Public Structure Connection
        ' Holds all the needed info on the current connection...
        Public strRemoteCallsign As String              ' remote station call sign
        Public intOBBytesToConfirm As Integer     ' remaining Outbound Buffer bytes to confirm  
        Public intBytesConfirmed As Integer      ' Outbound bytes confirmed by ACK and squenced
        Public intReceivedLeaderLen As Integer ' received leader length in ms. (0- 2550) RECEIVED by THIS station
        Public intReportedLeaderLen As Integer ' reported leader length in ms. (0- 2550) REPORTED by REMOTE station
        Public intCalcLeader As Integer        ' the computed leader to use based on the reported Leader Length
        Public bytSessionID As Byte            ' Session ID formed by 8 bit Hash of MyCallsign  and strRemoteCallsign always set to &HFF if not connected. 
        Public blnLastPSNPassed As Boolean       ' the last PSN passed True for Odd, False for even. 
        Public blnInitiatedConnection As Boolean  ' flag to indicate if this station initiated the connection
        Public dblAvgPECreepPerCarrier As Double  ' computed phase error creep
        Public dttLastIDSent As Date              ' date/time of last ID
        Public intTotalSymbols As Integer ' To compute the sample rate error
        Public strLocalCallsign As String ' the call sign used for this station
        Public intSessionBW As Integer
    End Structure ' Connection

    ' Structure used for statistics used to evaluate and measure performance
    Public Structure TuningStats
        Public intLeaderDetects As Integer
        Public intLeaderSyncs As Integer
        Public intAccumLeaderTracking As Integer
        Public dblFSKTuningSNAvg As Double
        Public intGoodFSKFrameTypes As Integer
        Public intFailedFSKFrameTypes As Integer
        Public intAccumFSKTracking As Integer
        Public intFSKSymbolCnt As Integer
        Public intGoodFSKFrameDataDecodes As Integer
        Public intFailedFSKFrameDataDecodes As Integer
        Public intAvgFSKQuality As Int32
        Public intFrameSyncs As Integer
        Public intGoodPSKSummationDecodes As Integer
        Public intGoodFSKSummationDecodes As Integer
        Public dblLeaderSNAvg As Double
        Public intAccumPSKLeaderTracking As Integer
        Public dblAvgPSKRefErr As Double
        Public intPSKTrackAttempts As Integer
        Public intAccumPSKTracking As Integer
        Public intPSKSymbolCnt As Integer
        Public intGoodPSKFrameDataDecodes As Integer
        Public intFailedPSKFrameDataDecodes As Integer
        Public intAvgPSKQuality As Int32
        Public dblAvgDecodeDistance As Double
        Public intDecodeDistanceCount As Int32
        Public intShiftUPs As Int32
        Public intShiftDNs As Int32
    End Structure 'TuningStats

    ' Structure used for Quality stats
    Public Structure QualityStats
        ' Stats used to compute quality 
        Public int4FSKQuality As Int32
        Public int4FSKQualityCnts As Int32
        Public int8FSKQuality As Int32
        Public int8FSKQualityCnts As Int32
        Public int16FSKQuality As Int32
        Public int16FSKQualityCnts As Int32
        Public intFSKSymbolsDecoded As Int32
        Public intPSKQuality() As Int32
        Public intPSKQualityCnts() As Int32
        Public intPSKSymbolsDecoded As Int32
    End Structure  '  QualityStats

    ' Structure for Session Statistics
    Public Structure SessionStats
        Public dttSessionStart As Date
        Public intTotalBytesSent As Integer
        Public intTotalBytesReceived As Integer
        Public intFrameTypeDecodes As Integer
        Public ModeQuality() As Double
        Public dblSearchForLeaderAvg As Double
        Public intMax1MinThruput As Integer
        Public intGearShifts As Integer
        Public blnStatsValid As Boolean
    End Structure '  SessionStats

    ' structure used for sound card devices
    Private Structure DeviceDescription
        Dim info As DeviceInformation
        Overrides Function ToString() As String
            Return info.Description
        End Function
        Sub New(ByVal d As DeviceInformation)
            info = d
        End Sub
    End Structure 'DeviceDescription

    ' Structure Assignments
    Public stcSessionStats As SessionStats
    Public MCB As New ModemControlBlock
    Public RCB As New RadioControlBlock
    Public stcTuningStats As TuningStats
    Public stcQualityStats As QualityStats
    Public stcConnection As Connection
#End Region ' Structures

#Region "Objects"
    ' Objects...
    Public objIniFile As INIFile
    Public objIniFileLock As New Object
    Public memWaveStream As MemoryStream
#End Region ' Objects

#Region "Integers"
    ' Integers...
    Public intSavedTuneLineLow As Integer = 0
    Public intSavedTuneLineHi As Integer = 0
    Public intBreakCounts As Integer = 0  ' for testing
    Public intTestFrameCorrectCnt As Integer = 0
    Public intFreqSentToHost As Int32 = 0
#End Region ' Integers

#Region "Doubles"
    ' Doubles
    Public dblMaxLeaderSN As Double
    Public dblOffsetHz As Double
    Public dblOffsetLastGoodDecode As Double
#End Region ' Doubles

#Region "Arrays"
    Public bytDataToSend(-1) As Byte ' Data receivied correctly from host and waiting to send via TNC 
    Public bytReceivedDataQueue(-1) As Byte ' Data captured by TNC and waiting trans mission to the Host. Note for FEC mode this may buffer 
    ' several repeated frames 
#End Region


#Region "Date/Time"
    ' Date/Time
    Public dttTestStart As Date
#End Region ' Date/Time

#Region "Strings"
    ' Strings...
    Public strExecutionDirectory As String = Application.StartupPath & "/"
    Public strProductVersion As String = Application.ProductVersion
    Public strWavDirectory As String
    Public strLastWavStream As String
    Public strRcvFrameTag As String
    Public strDecodeCapture As String
#End Region ' Strings

#Region "Graphics"
    'Graphics
    Public bmpConstellation As Bitmap
#End Region ' Graphics

#Region "Queues"
    ' Queues...
    Public queTNCStatus As Queue = queue.Synchronized(New Queue)

#End Region ' Queues

#Region "Enums"
    Public Enum ReceiveState ' used for initial receive testing...later put in correct protocol states
        SearchingForLeader
        AcquireSymbolSync
        AcquireFrameSync
        AcquireFrameType
        DecodeFrameType
        AcquireFrame
        DecodeFrame
    End Enum
    Public State As ReceiveState ' used for initial testing

    Public Enum ProtocolState
        OFFLINE
        DISC
        ISS
        IRS
        IDLE
        FECSend
        FECRcv
    End Enum
    Public ARDOPState As ProtocolState

#End Region ' Enums


#Region "Public Subs/Functions"

    'Clear all Tuning Stats
    Public Sub ClearTuningStats()
        With stcTuningStats
            .intLeaderDetects = 0
            .intLeaderSyncs = 0
            .intFrameSyncs = 0
            .intAccumFSKTracking = 0
            .intFSKSymbolCnt = 0
            .intAccumPSKTracking = 0
            .intPSKSymbolCnt = 0
            .intGoodFSKFrameTypes = 0
            .intFailedFSKFrameTypes = 0
            .intGoodFSKFrameDataDecodes = 0
            .intFailedFSKFrameDataDecodes = 0
            .intGoodPSKFrameDataDecodes = 0
            .intGoodPSKSummationDecodes = 0
            .intGoodFSKSummationDecodes = 0
            .intFailedPSKFrameDataDecodes = 0
            .intAvgFSKQuality = 0
            .intAvgPSKQuality = 0
            .dblFSKTuningSNAvg = 0
            .dblLeaderSNAvg = 0
            .dblAvgPSKRefErr = 0
            .intPSKTrackAttempts = 0
            .dblAvgDecodeDistance = 0
            .intDecodeDistanceCount = 0
            .intShiftDNs = 0
            .intShiftUPs = 0
        End With
    End Sub 'ClearTuningStats

    ' Sub to Initialize before a new Connection
    Public Sub InitializeConnection()
        With stcConnection
            .strRemoteCallsign = ""              ' remote station call sign
            .intOBBytesToConfirm = 0     ' remaining bytes to confirm  
            .intBytesConfirmed = 0     ' Outbound bytes confirmed by ACK and squenced
            .intReceivedLeaderLen = 0 ' Zero out received leader length (the length of the leader as received by the local station
            .intReportedLeaderLen = 0 ' Zero out the Reported leader length the length reported to the remote station 
            .bytSessionID = &HFF            ' Session ID 
            .blnLastPSNPassed = False       ' the last PSN passed True for Odd, False for even. 
            .blnInitiatedConnection = False  ' flag to indicate if this station initiated the connection
            .dblAvgPECreepPerCarrier = 0  ' computed phase error creep
            .dttLastIDSent = Now              ' date/time of last ID
            .intTotalSymbols = 0 ' To compute the sample rate error
            .strLocalCallsign = "" ' this stations call sign
            .intSessionBW = 0  ' ExtractARQBandwidth()
            .intCalcLeader = MCB.LeaderLength
        End With
        ClearQualityStats()
        ClearTuningStats()
    End Sub  ' InitializeConnection

    ' Sub to Clear the Quality Stats
    Public Sub ClearQualityStats()
        With stcQualityStats
            .int4FSKQuality = 0
            .int4FSKQualityCnts = 0
            .int8FSKQuality = 0
            .int8FSKQualityCnts = 0
            .int16FSKQuality = 0
            .int16FSKQualityCnts = 0
            ReDim .intPSKQuality(1) ' Quality for 4PSK, 8PSK  modulation modes
            ReDim .intPSKQualityCnts(1) ' Counts for 4PSK, 8PSK modulation modes 
            'need to get total quantity of PSK modes
            .intFSKSymbolsDecoded = 0
            .intPSKSymbolsDecoded = 0
        End With
    End Sub 'ClearQualityStats

    ' Sub to Write Tuning Stats to the Debug Log 
    Public Sub LogStats()
        If Not MCB.DebugLog Then Exit Sub ' only log if debug logging enabled
        Try
            Dim intTotFSKDecodes As Int32 = stcTuningStats.intGoodFSKFrameDataDecodes + stcTuningStats.intFailedFSKFrameDataDecodes
            Dim intTotPSKDecodes As Int32 = stcTuningStats.intGoodPSKFrameDataDecodes + stcTuningStats.intFailedPSKFrameDataDecodes

            With stcTuningStats
                Logs.WriteDebug(" ")
                Logs.WriteDebug("************************* ARQ session stats with " & stcConnection.strRemoteCallsign & " ****************************")
                Logs.WriteDebug("     LeaderDetects=" & .intLeaderDetects.ToString & "   AvgLeader S+N:N(dBpwr)=" & Format(.dblLeaderSNAvg, "#.0") & "  LeaderSyncs=" & .intLeaderSyncs.ToString)
                Logs.WriteDebug("     FrameSyncs=" & .intFrameSyncs.ToString & "   Good Frame Type Decodes=" & .intGoodFSKFrameTypes.ToString & "  Failed Frame Type Decodes =" & .intFailedFSKFrameTypes.ToString)
                Logs.WriteDebug("     Avg Frame Type decode distance=" & Format(.dblAvgDecodeDistance, "0.000") & " over " & .intDecodeDistanceCount.ToString & " decodes")

                If (.intGoodFSKFrameDataDecodes + .intFailedFSKFrameDataDecodes + .intGoodFSKSummationDecodes) > 0 Then
                    Logs.WriteDebug(" ")
                    Logs.WriteDebug("  FSK:")
                    Logs.WriteDebug("     Good FSK Data Frame Decodes=" & .intGoodFSKFrameDataDecodes.ToString & "  RecoveredFSKCarriers with Summation=" & .intGoodFSKSummationDecodes.ToString & "  Failed FSK Data Frame Decodes=" & .intFailedFSKFrameDataDecodes.ToString)
                    Logs.WriteDebug("     AccumFSKTracking=" & .intAccumFSKTracking.ToString & "  over " & .intFSKSymbolCnt.ToString & " symbols   Good Data Frame Decodes=" & .intGoodFSKFrameDataDecodes.ToString & "   Failed Data Frame Decodes=" & .intFailedFSKFrameDataDecodes.ToString)
                End If

                If (.intGoodPSKFrameDataDecodes + .intFailedPSKFrameDataDecodes + .intGoodPSKSummationDecodes) > 0 Then
                    Logs.WriteDebug(" ")
                    Logs.WriteDebug("  PSK:")
                    Logs.WriteDebug("     Good PSK Data Frame Decodes=" & .intGoodPSKFrameDataDecodes.ToString & "  RecoveredPSKCarriers with Summation=" & .intGoodPSKSummationDecodes.ToString & "  Failed PSK Data Frame Decodes=" & .intFailedPSKFrameDataDecodes.ToString)
                    Logs.WriteDebug("     AccumPSKTracking=" & .intAccumPSKTracking.ToString & "/" & .intPSKTrackAttempts.ToString & " attempts  over " & .intPSKSymbolCnt.ToString & " total PSK Symbols")
                End If
                Logs.WriteDebug(" ")
                Logs.WriteDebug("  Shift UPs=" & .intShiftUPs.ToString & "   Shift DNs=" & .intShiftDNs.ToString)
            End With
            With stcQualityStats
                Logs.WriteDebug(" ")
                Logs.WriteDebug("  Received Frame Quality:")
                If .int4FSKQualityCnts > 0 Then
                    Logs.WriteDebug("     Avg 4FSK Quality=" & Format(.int4FSKQuality / .int4FSKQualityCnts, "#") & "  on " & .int4FSKQualityCnts.ToString & " frame(s)")
                End If
                If .int8FSKQualityCnts > 0 Then
                    Logs.WriteDebug("     Avg 8FSK Quality=" & Format(.int8FSKQuality / .int8FSKQualityCnts, "#") & "  on " & .int8FSKQualityCnts.ToString & " frame(s)")
                End If
                If .int16FSKQualityCnts > 0 Then
                    Logs.WriteDebug("     Avg 16FSK Quality=" & Format(.int16FSKQuality / .int16FSKQualityCnts, "#") & "  on " & .int16FSKQualityCnts.ToString & " frame(s)")
                End If
                If .intPSKQualityCnts(0) > 0 Then
                    Logs.WriteDebug("     Avg 4PSK Quality=" & Format(.intPSKQuality(0) / .intPSKQualityCnts(0), "#") & "  on " & .intPSKQualityCnts(0).ToString & " frame(s)")
                End If
                If .intPSKQualityCnts(1) > 0 Then
                    Logs.WriteDebug("     Avg 8PSK Quality=" & Format(.intPSKQuality(1) / .intPSKQualityCnts(1), "#") & "  on " & .intPSKQualityCnts(1).ToString & " frames")
                End If

            End With
            Logs.WriteDebug("************************************************************************************")
        Catch
        End Try
    End Sub ' LogStats

    ' Function to convert string Text (ASCII) to byte array
    Public Function GetBytes(ByVal strText As String) As Byte()
        ' Converts a text string to a byte array...

        Dim bytBuffer(strText.Length - 1) As Byte
        For intIndex As Integer = 0 To bytBuffer.Length - 1
            bytBuffer(intIndex) = CByte(Asc(strText.Substring(intIndex, 1)))
        Next
        Return bytBuffer
    End Function 'GetBytes

    ' Function to Get ASCII string from a byte array
    Public Function GetString(ByVal bytBuffer() As Byte, _
       Optional ByVal intFirst As Integer = 0, Optional ByVal intLast As Integer = -1) As String
        ' Converts a byte array to a text string...

        If intFirst > bytBuffer.GetUpperBound(0) Then Return ""
        If intLast = -1 Or intLast > bytBuffer.GetUpperBound(0) Then intLast = bytBuffer.GetUpperBound(0)

        Dim sbdInput As New StringBuilder
        For intIndex As Integer = intFirst To intLast
            Dim bytSingle As Byte = bytBuffer(intIndex)
            If bytSingle <> 0 Then sbdInput.Append(Chr(bytSingle))
        Next
        Return sbdInput.ToString
    End Function  'GetString

    ' Function to generate a UTC Timestamp string "yyyy/MM/dd HH:mm"
    Public Function Timestamp() As String
        ' This function returns the current time/date in 
        ' 2004/08/24 05:33 format string...
        Return Format(Date.UtcNow, "yyyy/MM/dd HH:mm")
    End Function 'Timestamp

    ' Function to generate an Extended UTC Timestamp string "yyyy/MM/dd HH:mm:ss"
    Public Function TimestampEx() As String
        ' This function returns the current time/date in 
        ' 2004/08/24 05:33:12 format string...
        Return Format(DateTime.UtcNow, "yyyy/MM/dd HH:mm:ss")
    End Function

    ' Function to compare two Byte Arrays
    Public Function CompareByteArrays(ary1() As Byte, ary2() As Byte) As Boolean
        If IsNothing(ary1) And IsNothing(ary2) Then Return True
        If IsNothing(ary1) Or IsNothing(ary2) Then Return False
        If ary1.Length <> ary2.Length Then Return False
        For i As Integer = 0 To ary1.Length - 1
            If ary1(i) <> ary2(i) Then Return False
        Next
        Return True
    End Function 'CompareByteArrays

    ' function to generate 8 bit session ID
    Public Function GenerateSessionID(strCallingCallSign As String, strTargetCallsign As String) As Byte
        Dim bytToCRC() As Byte = GetBytes(strCallingCallSign.ToUpper.Trim & strTargetCallsign.ToUpper.Trim)
        GenCRC8(bytToCRC)
        If bytToCRC(bytToCRC.Length - 1) <> &HFF Then
            GenerateSessionID = bytToCRC(bytToCRC.Length - 1)
        Else ' rare case where the computed session ID woudl be FF
            GenerateSessionID = 0 'Remap a SessionID of FF to 0...FF reserved for FEC mode
        End If
        If MCB.DebugLog Then Logs.WriteDebug("[GenerateSessionID] Caller=" & strCallingCallSign & ",  Target=" & strTargetCallsign & " ID= H" & Format(GenerateSessionID, "X"))
    End Function

    ' Subroutine to compute a 8 bit CRC value and append it to the Data...
    Private Sub GenCRC8(ByRef Data() As Byte)
        ' For  CRC-8-CCITT =    x^8 + x^7 +x^3 + x^2 + 1  intPoly = 1021 Init FFFF

        Dim intPoly As Integer = &HC6 ' This implements the CRC polynomial  x^8 + x^7 +x^3 + x^2 + 1
        Dim intRegister As Int32 = &HFF

        For Each bytSingle As Byte In Data
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (bytSingle And CByte(2 ^ i)) <> 0
                If (intRegister And &H80) = &H80 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFF And (2 * intRegister)
                    End If
                End If
            Next
        Next
        ReDim Preserve Data(Data.Length) 'make the Data array one byte larger
        Data(Data.Length - 1) = CByte(intRegister And &HFF) ' LS 8 bits of Register
    End Sub 'GenCRC8

    ' Subroutine to compute a 16 bit CRC value and append it to the Data... With LS byte XORed by bytFrameType
    Public Sub GenCRC16FrameType(ByRef Data() As Byte, intStartIndex As Int32, intStopIndex As Int32, Optional bytFrameType As Byte = 0)
        ' For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
        ' intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

        Dim intPoly As Integer = &H8810 ' This implements the CRC polynomial  x^16 + x^12 +x^5 + 1
        Dim intRegister As Int32 = &HFFFF 'initialize the register to all 1's 

        For j As Integer = intStartIndex To intStopIndex
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (Data(j) And CByte(2 ^ i)) <> 0
                If (intRegister And &H8000) = &H8000 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                End If
            Next i
        Next j
        ' Put the two CRC bytes after the stop index
        Data(intStopIndex + 1) = CByte((intRegister And &HFF00) \ 256) ' MS 8 bits of Register
        Data(intStopIndex + 2) = (CByte(intRegister And &HFF) Xor bytFrameType) ' LS 8 bits of Register
    End Sub 'GenCRC16

    ' Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) XORing LS byte with bytFrameType..
    Public Function CheckCRC16FrameType(ByRef Data() As Byte, Optional bytFrameType As Byte = 0) As Boolean
        ' Returns True if CRC matches, else False
        ' For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
        ' intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

        Dim intPoly As Integer = &H8810 ' This implements the CRC polynomial  x^16 + x^12 +x^5 + 1
        Dim intRegister As Int32 = &HFFFF ' initialize the register to all 1's

        For j As Integer = 0 To Data.Length - 3 ' 2 bytes short of data length
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (Data(j) And CByte(2 ^ i)) <> 0
                If (intRegister And &H8000) = &H8000 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                End If
            Next i
        Next j

        ' Compare the register with the last two bytes of Data (the CRC) 
        If Data(Data.Length - 2) = CByte((intRegister And &HFF00) \ 256) Then
            If Data(Data.Length - 1) = (CByte(intRegister And &HFF) Xor bytFrameType) Then
                Return True
            End If
        End If
        Return False
    End Function 'CheckCRC16FrameType

    ' Subroutine to Update FSK Frame Decoding Stats
    Public Sub UpdateFSKFrameDecodeStats(blnDecodeSuccess As Boolean)
        If Not MCB.AccumulateStats Then Exit Sub
        If blnDecodeSuccess Then
            stcTuningStats.intGoodFSKFrameTypes += 1
        Else
            stcTuningStats.intFailedFSKFrameTypes += 1
        End If
    End Sub 'UpdateFSKFrameDecodeStats

    Public Function EnumeratePlaybackDevices() As String()

        If ALSA Then
            Return AlsaEnumeratePlaybackDevices()
        End If
        ' Get the Windows enumerated Playback devices adding a "-n" tag (-1, -2, etc.)  if any duplicate names 
        Dim cllPlaybackDevices As New Microsoft.DirectX.DirectSound.DevicesCollection
        Dim objDI As New DeviceInformation
        Dim intCtr As Integer = 0
        Dim intDupeDeviceCnt As Integer

        Dim strPlaybackDevices(cllPlaybackDevices.Count - 1) As String
        For Each objDI In cllPlaybackDevices
            Dim objDD As New DeviceDescription(objDI)
            strPlaybackDevices(intCtr) = objDD.ToString.Trim
            intCtr += 1
        Next

        For i As Integer = 0 To strPlaybackDevices.Length - 1
            intDupeDeviceCnt = 1
            For j As Integer = i + 1 To strPlaybackDevices.Length - 1
                If strPlaybackDevices(j) = strPlaybackDevices(i) Then
                    intDupeDeviceCnt += 1
                    strPlaybackDevices(j) = strPlaybackDevices(i) & "-" & intDupeDeviceCnt.ToString
                End If
            Next j
        Next i
        Return strPlaybackDevices
    End Function

    Public Function EnumerateCaptureDevices() As String()
        If ALSA Then
            Return AlsaEnumerateCaptureDevices()
        End If
        ' Get the Windows enumerated Capture devices adding a "-n" tag (-1, -2, etc.) if any duplicate names 
        Dim cllCaptureDevices As New CaptureDevicesCollection
        Dim objDI As New DeviceInformation
        Dim intCtr As Integer = 0
        Dim intDupeDeviceCnt As Integer

        Dim strCaptureDevices(cllCaptureDevices.Count - 1) As String
        For Each objDI In cllCaptureDevices
            Dim objDD As New DeviceDescription(objDI)
            strCaptureDevices(intCtr) = objDD.ToString.Trim
            intCtr += 1
        Next

        For i As Integer = 0 To strCaptureDevices.Length - 1
            intDupeDeviceCnt = 1
            For j As Integer = i + 1 To strCaptureDevices.Length - 1
                If strCaptureDevices(j) = strCaptureDevices(i) Then
                    intDupeDeviceCnt += 1
                    strCaptureDevices(j) = strCaptureDevices(i) & "-" & intDupeDeviceCnt.ToString
                End If
            Next j
        Next i
        Return strCaptureDevices
    End Function

    Public Function ASCIIto6Bit(ByVal bytASCII() As Byte) As Byte()
        ' Input must be 8 bytes which will convert to 6 bytes of packed 6 bit characters and
        ' inputs must be the ASCII character set values from 32 to 95....
        Dim intSum As Int64
        Dim intMask As Int64
        Dim bytReturn(5) As Byte
        For i As Integer = 0 To 3
            intSum = (64 * intSum) + bytASCII(i) - 32
        Next i
        For j As Integer = 0 To 2
            intMask = CInt(255 * (256 ^ (2 - j)))
            bytReturn(j) = CByte((intSum And intMask) \ CInt((256 ^ (2 - j))))
        Next
        intSum = 0
        For i As Integer = 0 To 3
            intSum = (64 * intSum) + bytASCII(i + 4) - 32
        Next i
        For j As Integer = 0 To 2
            intMask = CInt(255 * (256 ^ (2 - j)))
            bytReturn(j + 3) = CByte((intSum And intMask) \ CInt((256 ^ (2 - j))))
        Next
        Return bytReturn
    End Function

    Public Function Bit6ToASCII(ByVal byt6Bit() As Byte) As Byte()
        ' Input must be 6 bytes which represent packed 6 bit characters that well 
        ' result will be 8 ASCII character set values from 32 to 95...

        Dim intSum As Int64
        Dim intMask As Int64
        Dim bytReturn(7) As Byte

        For i As Integer = 0 To 2
            intSum = 256 * intSum + byt6Bit(i)
        Next i
        For j As Integer = 0 To 3
            intMask = CInt(63 * (64 ^ (3 - j)))
            bytReturn(j) = CByte(32 + (intSum And intMask) \ CInt((64 ^ (3 - j))))
        Next
        For i As Integer = 0 To 2
            intSum = 256 * intSum + byt6Bit(i + 3)
        Next i
        For j As Integer = 0 To 3
            intMask = CInt(63 * (64 ^ (3 - j)))
            bytReturn(j + 4) = CByte(32 + (intSum And intMask) \ CInt((64 ^ (3 - j))))
        Next
        Return bytReturn
    End Function

    ' Function for checking valid call sign syntax
    Public Function CheckValidCallsignSyntax(strCallsign As String) As Boolean

        Dim strTestCS As String = strCallsign.Trim.ToUpper
        Dim intDashIndex As Integer = strTestCS.IndexOf("-")
        Dim strValidChar As String = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

        If intDashIndex = -1 Then ' no -SSID
            If strTestCS.Length > 7 Or strTestCS.Length < 3 Then Return False
            For i As Integer = 0 To strCallsign.Trim.Length - 1
                If (strValidChar.IndexOf(strTestCS.Substring(i, 1)) = -1) Then Return False
            Next
            Return True
        Else
            Dim strCallNoSSID As String = strTestCS.Substring(0, intDashIndex).Trim
            If strCallNoSSID.Length > 7 Or strCallNoSSID.Length < 3 Then Return False
            Dim strSSID As String = strTestCS.Substring(intDashIndex + 1).Trim
            If IsNumeric(strSSID) Then
                If CInt(strSSID) < 0 Or CInt(strSSID) > 15 Then
                    Return False
                Else
                    Return True
                End If

            End If
            If strSSID.Length <> 1 Then Return False
            If "ABCDEFHIJKLMNOPQRSTUVWXYZ".IndexOf(strSSID) = -1 Then Return False
        End If
        Return True
    End Function ' CheckValidCallsignSyntax

    ' Function to compress callsign (up to 7 characters + optional "-"SSID   (-0 to -15 or -A to -Z) 
    Public Function CompressCallsign(strCallsign As String) As Byte()
        Dim intDashIndex As Integer = strCallsign.IndexOf("-")
        If intDashIndex = -1 Then ' if No SSID
            strCallsign = strCallsign.PadRight(7).ToUpper & "0" ' "0" indicates no SSID
            Return ASCIIto6Bit(GetBytes(strCallsign)) ' compress to 8 6 bit characters   6 bytes total
        Else
            Dim strSSID As String = strCallsign.Substring(intDashIndex + 1)
            If IsNumeric(strSSID) AndAlso (CInt(strSSID) >= 10) Then ' handles special case of -10 to -15 
                ' special codes used for -SSID of -10 through -15  ' : ; < = > ? '
                strCallsign = strCallsign.Substring(0, intDashIndex).ToUpper.PadRight(7).ToUpper & ":;<=>?".Substring((CInt(strSSID) - 10), 1)
            Else
                strCallsign = strCallsign.Substring(0, intDashIndex).ToUpper.PadRight(7).ToUpper & strCallsign.Substring(intDashIndex + 1).Trim.ToUpper
            End If
            Dim bytReturn() As Byte = ASCIIto6Bit(GetBytes(strCallsign)) ' compress to 6 bit ascii (upper case)
            Return bytReturn ' 6 bytes total
        End If
    End Function  '  CompressCallsign

    ' Function to compress Gridsquare (up to 8 characters)
    Public Function CompressGridSquare(strGS As String) As Byte()
        If strGS.Length > 8 Then Return Nothing
        Dim bytReturn() As Byte = ASCIIto6Bit(GetBytes(strGS.PadRight(8))) ' compress to 6 bit ascii (upper case)
        Return bytReturn ' 6 bytes total
    End Function  ' CompressGridSquare

    ' Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
    Public Function DeCompressCallsign(bytCallsign() As Byte) As String
        If IsNothing(bytCallsign) Then Return ""
        If bytCallsign.Length <> 6 Then Return ""
        Dim bytTest() As Byte = Bit6ToASCII(bytCallsign)
        Dim strWithSSID As String
        If bytTest(7) = 48 Then ' Value of "0" so No SSID
            ReDim Preserve bytTest(6)
            Return GetString(bytTest).Trim
        ElseIf (bytTest(7) >= 58 And bytTest(7) <= 63) Then ' handles special case for -10 to -15
            DeCompressCallsign = GetString(bytTest).Substring(0, 7).Trim & "-"
            DeCompressCallsign &= (bytTest(7) - 48).ToString
        Else
            strWithSSID = GetString(bytTest)
            Return strWithSSID.Substring(0, 7).Trim & "-" & strWithSSID.Substring(strWithSSID.Length - 1)
        End If
    End Function ' DeCompressCallsign

    ' Function to decompress 6 byte Grid square to 4, 6 or 8 characters
    Public Function DeCompressGridSquare(bytGS() As Byte) As String
        If IsNothing(bytGS) Then Return ""
        If bytGS.Length <> 6 Then Return ""
        Dim bytTest() As Byte = Bit6ToASCII(bytGS)
        Return GetString(bytTest).Trim
    End Function ' DeCompressGridSquare

    '  Functino to extract bandwidth from MCB.ARQBandwidth
    Public Function ExtractARQBandwidth() As Integer
        If MCB.ARQBandwidth = "" Then Return 200
        If MCB.ARQBandwidth.ToUpper.IndexOf("M") <> -1 Then
            Return CInt(MCB.ARQBandwidth.Substring(0, MCB.ARQBandwidth.ToUpper.IndexOf("M")))
        ElseIf MCB.ARQBandwidth.ToUpper.IndexOf("F") <> -1 Then
            Return CInt(MCB.ARQBandwidth.Substring(0, MCB.ARQBandwidth.ToUpper.IndexOf("F")))
        Else
            Return 200
        End If
    End Function ' ExtractARQBandwidth
#End Region ' Public Subs/Functions
End Module
