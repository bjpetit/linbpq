Imports System.Threading
Imports System.Timers

Public Class ARDOPprotocol
    '  This is the class that actually implements the ARDOP protocol (FEC and ARQ) 

    ' Integers
    Public intFrameRepeatInterval As Int32 = 2000 ' delay time in ms between rpeated frames 
    Public intNegotiatedARQBW As Int32 = 200
    Public intARQRTmeasuredMs As Int32 = 0
    Dim intNAKctr As Int32 = 0
    Dim intRcvdSamples(-1) As Int32
    Dim intRcvdSamplesRPtr As Int32 = 0
    Dim intCurrentFrameSamples() As Int32
    Dim intFrameType As Int32
    Dim intFECFramesSent As Int32
    Dim intNumCar, intBaud, intDataLen, intRSLen As Integer
    Dim intSamplesToCompleteFrame As Int32 ' The number of samples  to capture (beyond Frame ID) to capture complete frame.
    Dim intLastFrameIDToHost As Int32 = 0
    Dim intLastFailedFrameID As Int32 = 0
    Dim intRepeatCount As Int32
    Dim intLastARQDataFrameToHost As Int32 = -1
    Dim intARQDefaultDlyMs As Integer = 300 ' Not sure if this really need with optimized leader length. 100 ms doesn't add much overhead.
    Dim intAvgQuality As Int32 ' the filtered average reported quality (0 to 100) practical range 50 to 96 
    Dim intShiftUpDn As Int32 = 0
    Dim intFrameTypePtr As Int32 = 0 ' Pointer to the current data mode in bytFrameTypesForBW() 
    Dim intRmtLeaderMeas As Int32 = 0
    Dim intDataToSend As Int32
    Dim intTrackingQuality As Int32 = -1

    ' Booleans
    Public blnARQDisconnect As Boolean = False
    Dim blnLeaderFound As Boolean = False
    Dim blnFrameSyncFound As Boolean = False
    Dim blnSymbolSyncFound As Boolean = False
    Dim blnOdd As Boolean
    Dim blnSendIDFrame As Boolean
    Dim blnAbort As Boolean = False
    Dim blnLastFECFrameRcvdStatus As Boolean
    Dim blnEnbARQRpt As Boolean
    Dim blnTimeoutTriggered As Boolean = False
    Dim blnLastFrameSentData As Boolean = False
    Dim blnListen As Boolean = True
    Dim blnDISCRepeating As Boolean = False
    Dim blnARQConnected As Boolean = False
    Dim blnPending As Boolean = False

    ' Bytes
    Dim bytFrameType, bytQualThres As Byte
    Dim bytLastFECDataPassedToHost() As Byte
    Dim bytLastFECDataFrameSent As Byte = 0  ' initialize to an improper data frame 
    Dim bytLastARQDataFrameSent As Byte = 0  ' initialize to an improper data frame
    Dim bytFailedData(-1) As Byte
    Dim bytDataToMod(-1) As Byte
    Dim bytLastSessionID As Byte ' used in DISC to be able to reply to END.
    Dim bytFrameData(-1) As Byte ' Data for the curent frame being sent. 
    Public bytLastARQSessionID As Byte ' used to allow answering DISC frame from last session while in DISC state 
    Dim bytCurrentFrameType As Byte = 0 ' The current frame type used for sending
    Dim bytFrameTypesForBW(-1) As Byte  ' Holds the byte array for Data modes for a session bandwidth. First are most robust, last are fastest
    Dim bytPendingSessionID As Byte ' Holds the pending Session ID Until ConACK received from ISS

    ' Strings
    Dim strCurrentFrameFilename
    Dim strMod As String = ""
    Dim strType As String = ""
    Dim strFrameComponents() As String
    Dim strAllDataModes() As String = {"8FSK.200.25", "4FSK.200.50S", "4FSK.200.50", "4PSK.200.100S", "4PSK.200.100", "8PSK.200.100", _
                                       "16FSK.500.25S", "16FSK.500.25", "4FSK.500.100S", "4FSK.500.100", "4PSK.500.100", "8PSK.500.100", "4PSK.500.167", "8PSK.500.167", _
                                       "4FSK.1000.100", "4PSK.1000.100", "8PSK.1000.100", "4PSK.1000.167", "8PSK.1000.167", _
                                   "4FSK.2000.600S", "4FSK.2000.600", "4FSK.2000.100", "4PSK.2000.100", "8PSK.2000.100", "4PSK.2000.167", "8PSK.2000.167"}
    Private strFECMode As String
    Private strIDGridSq As String
    Private strLastFECFrameRcvd As String
    Private strCurrentFrame As String
    Private strFinalIDCallsign As String = ""

    ' Doubles
    Dim dblOffsetHz As Double = 0

    ' objects
    Dim objFrameInfo As New FrameInfo
    Dim objMain As Main
    Private objDataToSendLock As New Object

    ' Dates/Times
    Dim dttLastLeaderDetect As Date = Now
    Dim dttLastFECIDSent As Date = Now
    Dim dttLastFrameSent As Date
    Dim dttTimeoutTrip As Date = Now
    Public dttStartRTMeasure As Date = Now

    ' Timers
    Private WithEvents tmrPollOBQueue As New Timers.Timer
    Private WithEvents tmrSendTimeout As New Timers.Timer
    Private WithEvents tmrIRSPendingTimeout As New Timers.Timer
    Private WithEvents tmrFinalID As New Timers.Timer

    'Enum of ARQ Substates
    Enum ARQSubStates
        ISSConReq
        ISSConAck
        ISSData
        ISSId
        IRSConAck
        IRSData
        IRSBreak
        IRSfromISS
        DISCArqEnd
        None
    End Enum

    Dim ARQState As ARQSubStates

    Public Property Listen As Boolean
        Get
            Return blnListen
        End Get
        Set(value As Boolean)
            ' do other things if session in progress (e.g Dirty disconnect etc)
            blnListen = value
            State = ReceiveState.SearchingForLeader
            SetARDOPProtocolState(ProtocolState.DISC)
            InitializeConnection()
        End Set
    End Property

    Public ReadOnly Property Connected As Boolean
        Get
            Return blnARQConnected
        End Get
    End Property

    Public ReadOnly Property Pending As Boolean
        Get
            Return blnPending
        End Get
    End Property

    'Subroutine to add data to outbound queue (bytDataToSend)
    Public Sub AddDataToDataToSend(bytNewData() As Byte)
        If bytNewData.Length = 0 Then Exit Sub
        SyncLock objDataToSendLock ' Prevents any use of data by other sub/function during an append operation.
            AppendDataToBuffer(bytNewData, bytDataToSend) ' Append data here to the end of the inbound queue (bytDataToSend)
            intDataToSend = bytDataToSend.Length
        End SyncLock
        objMain.objHI.QueueCommandToHost("BUFFER " & intDataToSend.ToString)
        tmrPollOBQueue.Stop()
        tmrPollOBQueue.Start()
    End Sub 'AddDataToDataToSend

    ' Subroutine to get intDataLen bytes from outbound queue (bytDataToSend)
    Public Sub GetDataFromQueue(ByRef bytData() As Byte, intDataLen As Int32)

        If intDataLen = 0 Then
            ReDim bytData(-1)
            Exit Sub
        End If
        SyncLock objDataToSendLock ' Prevents any use of data by other sub/function during an this operation.
            If bytDataToSend.Length >= intDataLen Then
                ReDim bytData(intDataLen - 1)
                Array.Copy(bytDataToSend, 0, bytData, 0, intDataLen)
                Dim bytTemp(bytDataToSend.Length - intDataLen - 1) As Byte
                If bytTemp.Length > 0 Then
                    Array.Copy(bytDataToSend, intDataLen, bytTemp, 0, bytTemp.Length)
                    bytDataToSend = bytTemp
                Else
                    ReDim bytDataToSend(-1)
                End If
                intDataToSend = bytDataToSend.Length
            Else
                ReDim bytData(bytDataToSend.Length - 1)
                Array.Copy(bytDataToSend, 0, bytData, 0, bytData.Length)
                ReDim bytDataToSend(-1)
                intDataToSend = 0
            End If
        End SyncLock
        objMain.objHI.QueueCommandToHost("BUFFER " & intDataToSend.ToString)
        tmrPollOBQueue.Stop()
        tmrPollOBQueue.Start()
    End Sub 'GetDataFromQueue

    ' Subroutine to safely restore data to queue to be used after a failed attempt to send data and a change of mode requiring a different data size
    Public Sub RestoreDataToQueue(ByRef bytData() As Byte)
        If bytData.Length = 0 Then Exit Sub
        SyncLock objDataToSendLock ' Prevents any use of data by other sub/function during an append operation.
            If bytDataToSend.Length = 0 Then
                bytDataToSend = bytData
            Else
                Dim bytTemp(bytData.Length + bytDataToSend.Length - 1) As Byte
                Array.Copy(bytData, 0, bytTemp, 0, bytData.Length)
                Array.Copy(bytDataToSend, 0, bytTemp, bytData.Length, bytDataToSend.Length)
                bytDataToSend = bytTemp
            End If
            intDataToSend = bytDataToSend.Length
        End SyncLock
        objMain.objHI.QueueCommandToHost("BUFFER " & intDataToSend.ToString)
        tmrPollOBQueue.Stop()
        tmrPollOBQueue.Start()
    End Sub 'RestoreDataToQueue

    ' Property to return number of bytes in Outbound Queue (bytDataToSend)
    Public ReadOnly Property DataToSend As Integer
        Get
            Return intDataToSend
        End Get
    End Property 'DataToSend

    ' Property to Read the protocol state
    Public ReadOnly Property GetARDOPProtocolState As ProtocolState
        Get
            Return ARDOPState
        End Get
    End Property 'GetARDOPProtocolState

    ' Property to Read the Pending Session ID
    Public ReadOnly Property PendingSessionID As Byte
        Get
            Return bytPendingSessionID
        End Get
    End Property 'PendingSessionID

    '  Subroutine to Set the protocol state 
    Public Sub SetARDOPProtocolState(ByVal value As ProtocolState)

        If ARDOPState = value Then Exit Sub
        ARDOPState = value
        Dim stcStatus As Status
        stcStatus.ControlName = "lblState"
        stcStatus.Text = ARDOPState.ToString
        Select Case ARDOPState
            Case ProtocolState.DISC
                blnARQDisconnect = False ' always clear the ARQ Disconnect Flag from host.
                stcStatus.BackColor = System.Drawing.Color.White
                blnARQConnected = False
                blnPending = False
            Case ProtocolState.FECRcv
                stcStatus.BackColor = System.Drawing.Color.PowderBlue
            Case ProtocolState.FECSend
                InitializeConnection()
                intLastFrameIDToHost = -1
                intLastFailedFrameID = -1
                ReDim bytFailedData(-1)
                stcStatus.BackColor = System.Drawing.Color.Orange
            Case ProtocolState.IRS
                stcStatus.BackColor = System.Drawing.Color.LightGreen
            Case ProtocolState.ISS
                stcStatus.BackColor = System.Drawing.Color.LightSalmon
            Case ProtocolState.IDLE
                stcStatus.BackColor = System.Drawing.Color.NavajoWhite
            Case ProtocolState.OFFLINE
                stcStatus.BackColor = System.Drawing.Color.Silver
        End Select
        queTNCStatus.Enqueue(stcStatus)
        objMain.objHI.QueueCommandToHost("NEWSTATE " & ARDOPState.ToString.ToUpper)
    End Sub 'SetARDOPProtocolState

    ' Function to abort an FEC or ARQ transmission 
    Public Function Abort() As Boolean
        blnAbort = True
        Return True
    End Function 'Abort

    ' Function to start sending FEC data 
    Public Function StartFEC(bytData() As Byte, strDataMode As String, intRepeats As Integer, blnSendID As Boolean) As Boolean
        'Return True if OK false if problem
        MCB.FECRepeats = intRepeats
        Dim blnModeOK As Boolean
        Dim bytEncodedBytes() As Byte

        If GetARDOPProtocolState = ProtocolState.FECSend Then ' If already sending FEC data simply add to the OB queue
            AddDataToDataToSend(bytData) ' add new data to queue
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.StartFEC] " & bytData.Length.ToString & " bytes received while in FECSend state...append to data to send.")
            Return True
        End If
        ' Check to see that there is data in the buffer.
        If bytData.Length = 0 And intDataToSend = 0 Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.StartFEC] No data to send!")
            Return False
        End If
        ' Check intRepeates
        If intRepeats < 0 Or intRepeats > 5 Then
            Logs.Exception("[ARDOPprotocol.StartFEC] Repeats out of range: " & intRepeats.ToString)
            Return False
        End If
        'check call sign
        If Not CheckValidCallsignSyntax(MCB.Callsign) Then
            Logs.Exception("[ARDOPprotocol.StartFEC] Invalid call sign: " & MCB.Callsign)
            Return False
        End If
        ' Check to see that strDataMode is correct
        For i As Integer = 0 To strAllDataModes.Length - 1
            If strDataMode.ToUpper = strAllDataModes(i) Then
                strFECMode = strAllDataModes(i)
                blnModeOK = True
                Exit For
            End If
        Next
        If blnModeOK = False Then
            Logs.Exception("[ARDOPprotocol.StartFEC] Illegal FEC mode: " & strDataMode)
            Return False
        End If
        While objMain.SoundIsPlaying
            Thread.Sleep(100)
        End While
        blnAbort = False
        intFrameRepeatInterval = 400 ' should be a safe number for FEC...perhaps could be shortened down to 200 -300 ms
        AddDataToDataToSend(bytData) ' add new data to queue
        SetARDOPProtocolState(ProtocolState.FECSend) ' set to FECSend state
        blnSendIDFrame = blnSendID
        If blnSendID Then
            bytEncodedBytes = objMain.objMod.Encode4FSKIDFrame(MCB.Callsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytEncodedBytes)
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename)
            strCurrentFrameFilename = strFECMode & ".E"
            intFECFramesSent = 0
            dttLastFECIDSent = Now
        Else
            Dim bytFrameData(-1) As Byte
            strFrameComponents = strFECMode.Split(".")
            bytFrameType = objFrameInfo.FrameCode(strFECMode & ".E")
            If bytFrameType = bytLastFECDataFrameSent Then ' Added 0.3.4.1 
                bytFrameType = bytFrameType Xor &H1 ' insures a new start is on a different frame type. 
            End If
            objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThres, strType)
            GetDataFromQueue(bytFrameData, intDataLen * intNumCar)
            ' If bytFrameData.Length < (intDataLen * intNumCar) Then ReDim Preserve bytFrameData((intDataLen * intNumCar) - 1)
            'Logs.WriteDebug("[ARDOPprotocol.StartFEC]  Frame Data (string) = " & GetString(bytFrameData))
            If strMod = "4FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytFrameData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytFrameType, bytFrameData)
            ElseIf strMod = "16FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytFrameData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytFrameType, bytFrameData)
            ElseIf strMod = "8FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytFrameData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytFrameType, bytFrameData)
            Else
                bytFrameData = objMain.objMod.EncodePSK(bytFrameType, bytFrameData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.ModPSK(bytFrameType, bytFrameData)
            End If
            bytLastFECDataFrameSent = bytFrameType
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename)
            intFECFramesSent = 1
        End If
        Return True
    End Function  'StartFEC

    ' Subroutine to clear the Outbound Queue (bytDataToSend) 
    Public Sub ClearDataToSend()
        SyncLock objDataToSendLock ' Prevents any use of data by other sub/function during an append operation.
            ReDim bytDataToSend(-1)
            intDataToSend = 0
        End SyncLock
        objMain.objHI.QueueCommandToHost("BUFFER 0")
        tmrPollOBQueue.Stop()
        tmrPollOBQueue.Start()
    End Sub 'ClearDataToSend

    '  Subroutine to append byte data to a buffer
    Private Sub AppendDataToBuffer(ByRef bytNewData() As Byte, ByRef bytBuffer() As Byte)
        If bytNewData.Length = 0 Then Exit Sub
        Dim intStartPtr As Integer = bytBuffer.Length
        ReDim Preserve bytBuffer(bytBuffer.Length + bytNewData.Length - 1)
        Array.Copy(bytNewData, 0, bytBuffer, intStartPtr, bytNewData.Length)
    End Sub 'AppendDataToBuffer

    ' Subroutine to process new samples as received from the sound card via Main.ProcessCapturedData
    ' Only called when not transmitting
    Public Sub ProcessNewSamples(bytSamples() As Byte)
        Dim intRcvdSamplesWPtr As Int32 = 0
        Dim stcStatus As Status = Nothing
        Dim blnFrameDecodedOK As Boolean = False
        Dim bytData(-1) As Byte

        Try
            intRcvdSamplesWPtr = intRcvdSamples.Length
            ReDim Preserve intRcvdSamples(intRcvdSamples.Length + bytSamples.Length \ 2 - 1)
            For i As Integer = 0 To bytSamples.Length \ 2 - 1
                intRcvdSamples(intRcvdSamplesWPtr) = System.BitConverter.ToInt16(bytSamples, 2 * i)
                intRcvdSamplesWPtr += 1
            Next i
            If (GetARDOPProtocolState = ProtocolState.FECSend) Then
                DiscardOldSamples()
                Exit Sub
            End If

            ' Searching for leader
            If State = ReceiveState.SearchingForLeader Then ' Search for leader as long as 960 samples (8  symbols) available
                While State = ReceiveState.SearchingForLeader And (intRcvdSamplesRPtr + 960 <= intRcvdSamples.Length)
                    blnLeaderFound = objMain.objDemod.SearchFor2ToneLeader(intRcvdSamples, intRcvdSamplesRPtr, dblOffsetHz)
                    If blnLeaderFound Then
                        dttLastLeaderDetect = Now
                        stcStatus.ControlName = "lblOffset"
                        stcStatus.Text = "Offset: " & (Format(dblOffsetHz, "#0.0").PadLeft(6)) & " Hz"
                        queTNCStatus.Enqueue(stcStatus)
                        objMain.objDemod.InitializeMixedSamples()
                        State = ReceiveState.AcquireSymbolSync
                        objMain.objDemod.ResetSNPwrs()
                    End If
                End While
                If State = ReceiveState.SearchingForLeader Then
                    DiscardOldSamples()
                    Exit Sub
                End If
            End If

            ' Acquire Symbol Sync 
            If State = ReceiveState.AcquireSymbolSync Then
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

    ' Subroutine to discard all sampled prior to current intRcvdSamplesRPtr
    Private Sub DiscardOldSamples()
        ' This restructures the intRcvdSamples array discarding all samples prior to intRcvdSamplesRPtr
        If intRcvdSamples.Length - intRcvdSamplesRPtr <= 0 Then
            ReDim intRcvdSamples(-1)
            intRcvdSamplesRPtr = 0
        Else
            Dim intTemp((intRcvdSamples.Length - intRcvdSamplesRPtr) - 1) As Int32
            Array.Copy(intRcvdSamples, intRcvdSamplesRPtr, intTemp, 0, intTemp.Length)
            ReDim intRcvdSamples(intTemp.Length - 1)
            Array.Copy(intTemp, intRcvdSamples, intTemp.Length)
            intRcvdSamplesRPtr = 0
        End If
    End Sub 'DiscardOldSamples

    ' Subroutine for initializatino upon instance
    Public Sub New(objRef As Main)
        objMain = objRef ' establish the back reference
        tmrPollOBQueue.Interval = 10000 ' 10 seconds
        tmrPollOBQueue.Start()
        tmrSendTimeout.Interval = 100
        tmrIRSPendingTimeout.Interval = 15000 ' 15 seconds (possible to later make less)
        tmrFinalID.Interval = 5000 ' 5 seconds.
    End Sub 'New

    ' Function to get the next FEC data frame 
    Private Function GetNextFECFrame() As Boolean
        Dim bytData(-1) As Byte
        If blnAbort Then
            ClearDataToSend()
            If MCB.DebugLog Then Logs.WriteDebug("[GetNextFECFrame] FECAbort. Going to DISC state")
            objMain.KeyPTT(False) ' insurance for PTT off
            SetARDOPProtocolState(ProtocolState.DISC)
            Return False
        ElseIf intFECFramesSent = -1 Then
            If MCB.DebugLog Then Logs.WriteDebug("[GetNextFECFrame] intFECFramesSent = -1.  Going to DISC state")
            SetARDOPProtocolState(ProtocolState.DISC)
            objMain.KeyPTT(False) ' insurance for PTT off
            Return False
        ElseIf (DataToSend = 0) And ((intFECFramesSent Mod (MCB.FECRepeats + 1)) = 0) And GetARDOPProtocolState = ProtocolState.FECSend Then
            If MCB.DebugLog Then Logs.WriteDebug("[GetNextFECFrame] All data and repeats sent.  Going to DISC state")
            SetARDOPProtocolState(ProtocolState.DISC)
            objMain.KeyPTT(False) ' insurance for PTT off
            Return False
        ElseIf (GetARDOPProtocolState <> ProtocolState.FECSend) Then
            Return False
        ElseIf intFECFramesSent = 0 Then   ' Initialize the first FEC Data frame (even) from the queue and compute the Filtered samples and filename
            strFrameComponents = strFECMode.Split(".")
            bytFrameType = objFrameInfo.FrameCode(strFECMode & ".E")
            If bytFrameType = bytLastFECDataFrameSent Then ' Added 0.3.4.1 
                bytFrameType = bytFrameType Xor &H1 ' insures a new start is on a different frame type. 
            End If
            objFrameInfo.FrameInfo(bytFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThres, strType)
            GetDataFromQueue(bytData, intDataLen * intNumCar)
            'If bytData.Length < (intDataLen * intNumCar) Then ReDim Preserve bytData((intDataLen * intNumCar) - 1)
            If strMod = "4FSK" Then
                bytData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                If bytFrameType >= &H7A And bytFrameType <= &H7D Then
                    intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytData(0), bytData, stcConnection.intCalcLeader)  ' Modulate Data frame 
                Else
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytData(0), bytData, stcConnection.intCalcLeader)  ' Modulate Data frame 
                End If
            ElseIf strMod = "16FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytFrameType, bytFrameData)
            ElseIf strMod = "8FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytFrameType, bytFrameData)
            Else
                bytData = objMain.objMod.EncodePSK(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.ModPSK(bytFrameType, bytData)
            End If
            objMain.objMod.CreateWaveStream(intCurrentFrameSamples)
            intFECFramesSent += 1
            bytLastFECDataFrameSent = bytFrameType
            Return True
        ElseIf (intFECFramesSent Mod (MCB.FECRepeats + 1)) = 0 Then
            GetDataFromQueue(bytData, intDataLen * intNumCar)
            'If bytData.Length < (intDataLen * intNumCar) Then ReDim Preserve bytData((intDataLen * intNumCar) - 1)
            ' toggle the frame type Even/Odd
            bytFrameType = bytFrameType Xor &H1
            If strMod = "4FSK" Then
                bytData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                If bytFrameType >= &H7A And bytFrameType <= &H7D Then
                    intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytData(0), bytData, stcConnection.intCalcLeader)  ' Modulate Data frame 
                Else
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytData(0), bytData, stcConnection.intCalcLeader)  ' Modulate Data frame 
                End If
            ElseIf strMod = "16FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytFrameType, bytFrameData)
            ElseIf strMod = "8FSK" Then
                bytFrameData = objMain.objMod.EncodeFSKData(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytFrameType, bytFrameData)
            Else
                bytData = objMain.objMod.EncodePSK(bytFrameType, bytData, strCurrentFrameFilename)
                intCurrentFrameSamples = objMain.objMod.ModPSK(bytFrameType, bytData)
            End If
            objMain.objMod.CreateWaveStream(intCurrentFrameSamples)
            intFECFramesSent += 1
            bytLastFECDataFrameSent = bytFrameType
            Return True
        ElseIf Now.Subtract(dttLastFECIDSent).TotalMinutes > 10 Then
            Dim strIDFrameFIlename As String = ""
            Dim bytIDData() As Byte = objMain.objMod.Encode4FSKIDFrame(MCB.Callsign, MCB.GridSquare, strIDFrameFIlename)
            Dim intIDSamples() As Int32 = objMain.objMod.Mod4FSKData(&H30, bytIDData)
            objMain.objMod.CreateWaveStream(intIDSamples)
            dttLastFECIDSent = Now
            Return True
        Else ' just a repeat of the last frame so no changes to samples...just inc frames Sent
            intFECFramesSent += 1
            Return True
        End If
    End Function 'GetNextFECFrame

    ' Subroutine to process Received FEC data 
    Private Sub ProcessRcvdFECDataFrame(intFrameType As Integer, bytData() As Byte, blnFrameDecodedOK As Boolean)
        ' Determine if this frame should be passed to Host.

        If blnFrameDecodedOK Then
            Dim blnDataMatchesLastToHost As Boolean = CompareByteArrays(bytData, bytLastFECDataPassedToHost)
            If (intFrameType = intLastFrameIDToHost) And blnDataMatchesLastToHost Then
                If MCB.CommandTrace Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdFECDataFrame] Same Frame ID: " & intFrameType.ToString & " and matching data, not passed to Host")
                Exit Sub ' frame data was already passed to host
            Else
                If (bytFailedData.Length > 0) And (intLastFailedFrameID <> intFrameType) Then
                    AddTagToDataAndSendToHost(bytFailedData, "ERR")
                    If MCB.CommandTrace Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdFECDataFrame] Pass failed frame ID " & intLastFailedFrameID.ToString & " to Host (" & bytFailedData.Length.ToString & " bytes)")
                    ReDim bytFailedData(-1)
                    intLastFailedFrameID = -1
                End If
                AddTagToDataAndSendToHost(bytData, "FEC")
                bytLastFECDataPassedToHost = bytData
                intLastFrameIDToHost = intFrameType
                If intLastFailedFrameID = intFrameType Then
                    ReDim bytFailedData(-1)
                    intLastFailedFrameID = -1
                End If
                If MCB.CommandTrace Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdFECDataFrame] Pass good data frame  ID " & intFrameType.ToString & " to Host (" & bytData.Length.ToString & " bytes)")
            End If
        Else
            If (bytFailedData.Length > 0) And (intLastFailedFrameID >= 0) And (intLastFailedFrameID <> intFrameType) Then
                AddTagToDataAndSendToHost(bytFailedData, "ERR")
                intLastFrameIDToHost = intLastFailedFrameID
                If MCB.CommandTrace Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdFECDataFrame] Pass failed data frame ID " & intLastFailedFrameID.ToString & " to Host (" & bytFailedData.Length.ToString & " bytes)")
            End If
            bytFailedData = bytData ' capture the current data and frame type 
            intLastFailedFrameID = intFrameType
        End If
    End Sub 'ProcessRcvdFECDataFrame

    ' Function to determine if call is to MCB.Callsign or one of the MCB.AuxCalls
    Private Function IsCallToMe(strCallsigns() As String, ByRef bytReplySessionID As Byte) As Boolean
        ' Returns true and sets bytReplySessionID if is to me.

        With stcConnection
            If strCallsigns(1) = MCB.Callsign Then
                bytReplySessionID = GenerateSessionID(strCallsigns(0), strCallsigns(1))
                Return True
            ElseIf (Not (IsNothing(MCB.AuxCalls))) AndAlso MCB.AuxCalls.Length > 0 Then
                For i As Integer = 0 To MCB.AuxCalls.Length - 1
                    If MCB.AuxCalls(i) = strCallsigns(1) Then
                        bytReplySessionID = GenerateSessionID(strCallsigns(0), strCallsigns(1))
                        Return True
                    End If
                Next
            End If
            Return False
        End With
    End Function 'IsCallToMe

    'This is the main subroutine for processing ARQ frames 
    Private Sub ProcessRcvdARQFrame(intFrameType As Integer, bytData() As Byte, blnFrameDecodedOK As Boolean)
        ' blnFrameDecodedOK should always be true except in the case of a failed data frame ...Which is then NAK'ed if in IRS Data state
        Dim intReply As Int32
        Dim bytDataToMod() As Byte
        Static strCallsigns() As String
        Dim intReportedLeaderMS As Int32 = 0

        Select Case GetARDOPProtocolState

            'DISC State *******************************************************************************************
            Case ProtocolState.DISC

                If blnFrameDecodedOK And intFrameType = &H29 Then ' Special case to process DISC from previous connection (Ending station must have missed END reply to DISC) Handles protocol rule 1.5
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState DISC, Send END with SessionID=" & Format(bytLastARQSessionID, "X") & " Stay in DISC state")
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, bytLastARQSessionID) ' Send END using SessionID of previous ARQ session. 
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, MCB.LeaderLength)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    tmrFinalID.Stop() : tmrFinalID.Start()
                    blnEnbARQRpt = False
                    Exit Sub
                End If
                If Not blnListen Then Exit Sub ' ignore connect request if not blnListen
                ' Process Connect request to MyCallsign or Aux Call signs  (Handles protocol rule 1.2)
                If (Not blnFrameDecodedOK) Or (intFrameType < &H31) Or (intFrameType > &H38) Then Exit Sub ' No decode or not a ConReq
                strCallsigns = GetString(bytData).Split(" ") 'strCallerCallsign & " " & strTargetCallsign
                ' see if connect request is to MyCallsign or any Aux call sign
                If IsCallToMe(strCallsigns, bytPendingSessionID) Then ' (Handles protocol rules 1.2, 1.3)
                    Logs.WriteDebug("[ProcessRcvdARQFrame]1 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
                    intReply = IRSNegotiateBW(intFrameType) ' NegotiateBandwidth
                    With stcConnection
                        If intReply <> &H2E Then ' If not ConRejBW the bandwidth is compatible so answer with correct ConAck frame
                            objMain.objHI.QueueCommandToHost("TARGET " & strCallsigns(1))
                            blnPending = True
                            tmrIRSPendingTimeout.Start() ' Triggers a 10 second timeout before auto abort from pending
                            '(Handles protocol rule 1.2)
                            dttTimeoutTrip = Now
                            SetARDOPProtocolState(ProtocolState.IRS) : ARQState = ARQSubStates.IRSConAck ' now connected 
                            intLastARQDataFrameToHost = -1 ' precondition to an illegal frame type
                            .strRemoteCallsign = strCallsigns(0) : .strLocalCallsign = strCallsigns(1) : strFinalIDCallsign = .strLocalCallsign
                            intAvgQuality = 0 ' initialize avg quality 
                            .intReceivedLeaderLen = objMain.objDemod.intLeaderRcvdMs ' capture the received leader from the remote ISS's ConReq (used for timing optimization)
                            bytDataToMod = objMain.objMod.EncodeConACKwTiming(intReply, objMain.objDemod.intLeaderRcvdMs, strCurrentFrameFilename, bytPendingSessionID)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConAck based on negotiated Bandwidth with extended leader and Received leader timing
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 0) ' No delay to allow ISS to measure its TX>RX delay 
                        ElseIf intReply = &H2E Then ' ConRejBW  (Incompatible bandwidths)
                            ' (Handles protocol rule 1.3)
                            bytDataToMod = objMain.objMod.Encode4FSKControl(intReply, strCurrentFrameFilename, bytPendingSessionID)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConRejBW based on negotiated Bandwidth 
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                            objMain.objHI.QueueCommandToHost("REJECTEDBW " & strCallsigns(0))
                            objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION FROM " & strCallsigns(0) & " REJECTED, INCOMPATIBLE BANDWIDTHS.")
                        End If
                    End With
                Else
                    objMain.objHI.QueueCommandToHost("CANCELPENDING")
                    ProcessUnconnectedConReqFrame(intFrameType, bytData) ' displays data if not connnected.  
                End If
                blnEnbARQRpt = False
                Exit Sub

                'IRS State ****************************************************************************************
            Case ProtocolState.IRS  ' Process ConReq, ConAck, DISC, END, Host DISCONNECT, DATA, IDLE, BREAK 

                If ARQState = ARQSubStates.IRSConAck Then   ' Process ConAck or ConReq if reply ConAck sent above in Case ProtocolState.DISC was missed by ISS
                    If (Not blnFrameDecodedOK) Then Exit Sub ' no reply if no correct decode

                    'ConReq processing (to handle case of ISS missing initial ConAck from IRS)
                    If intFrameType >= &H31 And intFrameType <= &H38 Then ' Process Connect request to MyCallsign or Aux Call signs as for DISC state above (ISS must have missed initial ConACK from ProtocolState.DISC state)
                        If Not blnListen Then Exit Sub
                        ' see if connect request is to MyCallsign or any Aux call sign
                        strCallsigns = GetString(bytData).Split(" ") 'strCallerCallsign & " " & strTargetCallsign
                        If IsCallToMe(strCallsigns, bytPendingSessionID) Then ' Establishes the stcConnection Remote, Target Calls and session ID
                            Logs.WriteDebug("[ProcessRcvdARQFrame]2 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
                            intRmtLeaderMeas = 0 ' insure remote leader measure is 0 
                            intReply = IRSNegotiateBW(intFrameType) ' NegotiateBandwidth
                            With stcConnection
                                If intReply <> &H2E Then ' If not ConRejBW
                                    ' Note: CONNECTION and STATUS notices were already sent from  Case ProtocolState.DISC above...no need to duplicate
                                    SetARDOPProtocolState(ProtocolState.IRS) : ARQState = ARQSubStates.IRSConAck
                                    intLastARQDataFrameToHost = -1
                                    InitializeConnection()
                                    dttTimeoutTrip = Now
                                    tmrIRSPendingTimeout.Stop() 'Stop and restart the Pending timer upon each ConReq received to ME
                                    tmrIRSPendingTimeout.Start()
                                    .strRemoteCallsign = strCallsigns(0) : .strLocalCallsign = strCallsigns(1) : strFinalIDCallsign = .strLocalCallsign
                                    .intReceivedLeaderLen = objMain.objDemod.intLeaderRcvdMs ' capture the received leader from the ISS's ConReq
                                    bytDataToMod = objMain.objMod.EncodeConACKwTiming(intReply, objMain.objDemod.intLeaderRcvdMs, strCurrentFrameFilename, bytPendingSessionID)
                                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConAck based on negotiated Bandwidth and Received leader timing
                                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 0) ' No delay to allow ISS to measure its TX>RX delay 
                                ElseIf intReply = &H2E Then ' ConRejBW
                                    If MCB.DebugLog Then Logs.WriteDebug("[ProcessRcvdARQFrame] Incompatible bandwidth connect request. Frame type: " & objFrameInfo.Name(intFrameType) & "   MCB.ARQBandwidth:  " & MCB.ARQBandwidth)
                                    bytDataToMod = objMain.objMod.Encode4FSKControl(intReply, strCurrentFrameFilename, bytPendingSessionID)
                                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConRejBW based on negotiated Bandwidth 
                                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                                    objMain.objHI.QueueCommandToHost("REJECTEDBW " & strCallsigns(0))
                                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION FROM " & strCallsigns(0) & " REJECTED, INCOMPATIBLE BANDWIDTHS.")
                                End If
                            End With
                        Else
                            ' this normally shouldn't happen but is put here in case another Connect request to a different station also on freq...may want to change or eliminate this
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Call to another target while in ProtocolState.IRS, ARQSubStates.IRSConAck...Ignore")
                            Exit Sub
                        End If

                        ' ConAck processing from ISS
                    ElseIf intFrameType >= &H39 And intFrameType <= &H3C Then ' Process ConACK frames from ISS confirming Bandwidth and providing ISS's received leader info.
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                        With stcConnection
                            Select Case intFrameType
                                Case &H39 : .intSessionBW = 200
                                Case &H3A : .intSessionBW = 500
                                Case &H3B : .intSessionBW = 1000
                                Case &H3C : .intSessionBW = 2000
                            End Select
                            CalculateOptimumLeader(10 * bytData(0), MCB.LeaderLength)
                            .bytSessionID = bytPendingSessionID ' This sets the session ID now 
                            bytDataToMod = objMain.objMod.EncodeDATAACK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, .bytSessionID) ' Send ACK
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send ACK... compatible bandwidth 
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default delay 
                            blnARQConnected = True
                            blnPending = False
                            tmrIRSPendingTimeout.Stop()
                            objMain.objHI.QueueCommandToHost("CONNECTED " & .strRemoteCallsign & " " & .intSessionBW.ToString)
                            objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION FROM " & strCallsigns(0) & ": SESSION BW = " & .intSessionBW.ToString & " HZ")
                            ' Initialize the frame type and pointer based on bandwidth (added 0.3.1.3)
                            Dim bytDummy As Byte
                            Dim blnDummy As Boolean
                            GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, True) ' just sets the initial data, frame type, and sets intShiftUpDn= 0
                            ' Update the main form menu status lable 
                            Dim stcStatus As Status = Nothing
                            stcStatus.ControlName = "mnuBusy"
                            stcStatus.Text = "Connected " & .strRemoteCallsign
                            queTNCStatus.Enqueue(stcStatus)
                            dttTimeoutTrip = Now
                            ARQState = ARQSubStates.IRSData
                            intLastARQDataFrameToHost = -1
                            intTrackingQuality = -1
                            intNAKctr = 0
                        End With
                    End If
                    blnEnbARQRpt = False
                    Exit Sub

                ElseIf ARQState = ARQSubStates.IRSData Then  'Process Data or ConAck if ISS failed to receive ACK confirming bandwidth so ISS repeated ConAck
                    ' ConAck processing from ISS
                    If blnFrameDecodedOK And intFrameType >= &H39 And intFrameType <= &H3C Then ' Process ConACK frames (ISS failed to receive prior ACK confirming session bandwidth so repeated ConACK)
                        With stcConnection
                            Select Case intFrameType
                                Case &H39 : .intSessionBW = 200
                                Case &H3A : .intSessionBW = 500
                                Case &H3B : .intSessionBW = 1000
                                Case &H3C : .intSessionBW = 2000
                            End Select
                            bytDataToMod = objMain.objMod.EncodeDATAACK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, .bytSessionID) ' Send ACK
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send ACK... compatible bandwidth 
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default delay 
                            dttTimeoutTrip = Now
                        End With
                        Exit Sub

                        ' handles DISC from ISS
                    ElseIf blnFrameDecodedOK And intFrameType = &H29 Then '  IF DISC received from ISS Handles protocol rule 1.5
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState IRS, IRSData...going to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send END
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        bytLastARQSessionID = stcConnection.bytSessionID ' capture this session ID to allow answering DISC from DISC state if ISS missed Sent END
                        ClearDataToSend() : tmrFinalID.Stop() : tmrFinalID.Start() : blnDISCRepeating = False
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()
                        blnEnbARQRpt = False
                        Exit Sub

                        ' handles END from ISS
                    ElseIf blnFrameDecodedOK And intFrameType = &H2C Then '  IF END received from ISS 
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END frame received in ProtocolState IRS, IRSData...going to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        blnDISCRepeating = False
                        ClearDataToSend()
                        If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod)
                            dttLastFECIDSent = Now
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        End If
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()
                        blnEnbARQRpt = False
                        Exit Sub

                        ' handles DISCONNECT command from host
                    ElseIf CheckForDisconnect() Then
                        Exit Sub
                        ' This handles normal data frames
                    ElseIf blnFrameDecodedOK And objFrameInfo.IsDataFrame(intFrameType) Then ' Frame even/odd toggling will prevent duplicates in case of missed ACK
                        If intRmtLeaderMeas = 0 Then
                            intRmtLeaderMeas = objMain.objDemod.intRmtLeaderMeasure ' capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving data) RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
                        End If
                        If intFrameType <> intLastARQDataFrameToHost Then ' protects against duplicates if ISS missed IRS's ACK and repeated the same frame  
                            AddTagToDataAndSendToHost(bytData, "ARQ") ' only correct data in proper squence passed to host
                            intLastARQDataFrameToHost = intFrameType
                            dttTimeoutTrip = Now
                        End If
                        ' Always ACK if it is a data frame ...ISS may have missed last ACK
                        bytDataToMod = objMain.objMod.EncodeDATAACK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, stcConnection.bytSessionID) '
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send ACK... With quality 
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default  delay  
                        blnEnbARQRpt = False
                        Exit Sub

                        ' handles IDLE from ISS
                    ElseIf blnFrameDecodedOK And intFrameType = &H26 Then '  IF IDLE received from ISS indicating ISS has no more data to send
                        If intRmtLeaderMeas = 0 Then
                            intRmtLeaderMeas = objMain.objDemod.intRmtLeaderMeasure ' capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving IDLE) RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
                        End If
                        If intDataToSend = 0 Then ' no data pending at IRS so send ACK
                            bytDataToMod = objMain.objMod.EncodeDATAACK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send ACK
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 500)
                            blnEnbARQRpt = False
                            'blnIRSBreakSent = False
                        Else ' Data pending so send BREAK
                            '  This implements the tricky and important IRS>ISS changeover...may have to adjust parameters here for reliability 
                            dttTimeoutTrip = Now
                            bytDataToMod = objMain.objMod.Encode4FSKControl(&H23, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send BREAK
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 500) ' 2 * default delay 
                            intFrameRepeatInterval = 2000 'keep IDLE/BREAK Repeats fairly long
                            ARQState = ARQSubStates.IRSBreak   '(ONLY IRS State where repeats are used)
                            blnEnbARQRpt = True ' setup for repeats until changeover 
                        End If
                        Exit Sub

                        ' This handles the final transition from IRS to ISS
                    ElseIf blnFrameDecodedOK And intFrameType = &H23 Then ' if BREAK (Can only come from the ISS that has now transitioned to IRS)
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd in IRS ARQState IRSData... indicates remote side has transitioned to IRS")
                        bytDataToMod = objMain.objMod.EncodeDATAACK(100, strCurrentFrameFilename, stcConnection.bytSessionID) '
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send ACK... With quality 
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default  delay 
                        blnEnbARQRpt = False
                        ' handles Data frame which did not decode correctly (Failed CRC)
                    ElseIf (Not blnFrameDecodedOK) And objFrameInfo.IsDataFrame(intFrameType) Then ' Incorrectly decoded frame. Send NAK with Quality
                        bytDataToMod = objMain.objMod.EncodeDATANAK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send ACK
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default delay 
                        blnEnbARQRpt = False
                    End If
                    Exit Sub

                ElseIf ARQState = ARQSubStates.IRSBreak Then
                    If blnFrameDecodedOK And intFrameType >= &HE0 Then ' If ACK
                        ' ACK received while in IRSBreak state completes transition to ISS
                        blnEnbARQRpt = False ' stops repeat and force new data frame or IDLE
                        intLastARQDataFrameToHost = -1 ' initialize to illegal value to capture first new ISS frame and pass to host
                        If bytCurrentFrameType = 0 Then ' hasn't been initialized yet
                            Dim bytDummy As Byte ' Initialize the frame type based on bandwidth
                            Dim blnDummy As Boolean
                            GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, True) ' just sets the initial data, frame type, and sets intShiftUpDn= 0
                        End If
                        If intDataToSend > 0 Then
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend > 0,  IRS > ISS, Substate ISSData")
                            SetARDOPProtocolState(ProtocolState.ISS) : ARQState = ARQSubStates.ISSData
                            SendDataOrIDLE(False)
                            intNAKctr = 0
                        Else
                            If MCB.DebugLog Then
                                Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend = 0,  IRS > IDLE, Substate ISSData")
                                SetARDOPProtocolState(ProtocolState.IDLE) : ARQState = ARQSubStates.ISSData
                            End If
                            SendDataOrIDLE(False)
                            intNAKctr = 0
                        End If
                        Exit Sub
                    End If
                End If


                'IDLE State  *********************************************************************************************
            Case ProtocolState.IDLE  ' The state where the ISS has no data to send and is looking for an ACK or BREAK from the IRS
                If Not blnFrameDecodedOK Then Exit Sub ' No decode so continue repeating IDLE
                ' process ACK, or  BREAK here Send ID if over 10 min. 
                If intFrameType >= &HE0 Then ' if ACK
                    SendDataOrIDLE(False)

                ElseIf intFrameType = &H23 Then ' if BREAK
                    ' Initiate the transisiton to IRS
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = False
                    bytDataToMod = objMain.objMod.EncodeDATAACK(100, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from IDLE, Go to IRS, Substate IRSData")
                    SetARDOPProtocolState(ProtocolState.IRS) : ARQState = ARQSubStates.IRSData
                    intLastARQDataFrameToHost = -1 ' precondition to an illegal frame type (insures the new IRS does not reject a frame)

                ElseIf intFrameType = &H29 Then '  IF DISC received from IRS Handles protocol rule 1.5
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
                    If MCB.AccumulateStats Then LogStats()
                    objMain.objHI.QueueCommandToHost("DISCONNECTED")
                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send END
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    bytLastARQSessionID = stcConnection.bytSessionID ' capture this session ID to allow answering DISC from DISC state
                    tmrFinalID.Stop() : tmrFinalID.Start()
                    blnDISCRepeating = False
                    ClearDataToSend()
                    SetARDOPProtocolState(ProtocolState.DISC)
                    InitializeConnection()
                    blnEnbARQRpt = False

                ElseIf intFrameType = &H2C Then ' if END
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state")
                    If MCB.AccumulateStats Then LogStats()
                    objMain.objHI.QueueCommandToHost("DISCONNECTED")
                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                    ClearDataToSend()
                    If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                        bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod, stcConnection.intCalcLeader)
                        dttLastFECIDSent = Now
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    End If
                    SetARDOPProtocolState(ProtocolState.DISC)
                    InitializeConnection()
                    blnEnbARQRpt = False
                    blnDISCRepeating = False
                End If

                ' ISS state **************************************************************************************
            Case ProtocolState.ISS
                If ARQState = ARQSubStates.ISSConReq Then ' The ISS is sending Connect requests waiting for a ConAck from the remote IRS
                    ' Session ID should be correct already (set by the ISS during first Con Req to IRS)
                    ' Process IRS Conack and capture IRS received leader for timing optimization

                    ' Process ConAck from IRS (Handles protocol rule 1.4)
                    If blnFrameDecodedOK And intFrameType >= &H39 And intFrameType <= &H3C Then ' Process ConACK frames from IRS confirming BW is compatible and providing received leader info.
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                        With stcConnection
                            Select Case intFrameType
                                Case &H39 : .intSessionBW = 200
                                Case &H3A : .intSessionBW = 500
                                Case &H3B : .intSessionBW = 1000
                                Case &H3C : .intSessionBW = 2000
                            End Select
                            CalculateOptimumLeader(10 * bytData(0), MCB.LeaderLength)
                            ARQState = ARQSubStates.ISSConAck
                            ' Initialize the frame type based on bandwidth
                            Dim bytDummy As Byte
                            Dim blnDummy As Boolean
                            GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, True) ' just sets the initial data frame type and sets intShiftUpDn = 0
                            ' prepare the ConACK answer with received leader length
                            .intReceivedLeaderLen = objMain.objDemod.intLeaderRcvdMs
                            bytDataToMod = objMain.objMod.EncodeConACKwTiming(intFrameType, .intReceivedLeaderLen, strCurrentFrameFilename, stcConnection.bytSessionID)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, MCB.LeaderLength)  ' Send ConAck to confirm negotiated Bandwidth and Send Received leader timing
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 0) ' No delay to allow IRS to measure its TX>RX delay and calc leader received 
                            intFrameRepeatInterval = 2000
                            blnEnbARQRpt = True ' Setup for repeats of the ConACK if no answer from IRS
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Compatible bandwidth received from IRS ConAck: " & stcConnection.intSessionBW.ToString & " Hz")
                            ARQState = ARQSubStates.ISSConAck
                            dttLastFECIDSent = Now
                        End With
                        Exit Sub
                    End If

                ElseIf ARQState = ARQSubStates.ISSConAck Then
                    If blnFrameDecodedOK And intFrameType >= &HE0 Then  ' if ACK received then IRS correctly received the ISS ConACK 
                        If intRmtLeaderMeas = 0 Then
                            intRmtLeaderMeas = objMain.objDemod.intRmtLeaderMeasure ' capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
                        End If

                        intAvgQuality = 0 ' initialize avg quality
                        blnEnbARQRpt = False  ' stop the repeats of ConAck and enables SendDataOrIDLE to get next IDLE or Data frame
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ACK received in ARQState " & ARQState.ToString)
                        SendDataOrIDLE(False) ' this should start a repeat of either IDLE (if no data to send) or Data (if outbound queue not empty) 
                        blnARQConnected = True
                        blnPending = False
                        objMain.objHI.QueueCommandToHost("CONNECTED " & stcConnection.strRemoteCallsign & " " & stcConnection.intSessionBW)
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ESTABLISHED WITH " & stcConnection.strRemoteCallsign & ", SESSION BW = " & stcConnection.intSessionBW.ToString & " HZ")
                        ARQState = ARQSubStates.ISSData
                        ' Update the main form menu status lable 
                        Dim stcStatus As Status = Nothing
                        stcStatus.ControlName = "mnuBusy"
                        stcStatus.Text = "Connected " & stcConnection.strRemoteCallsign
                        queTNCStatus.Enqueue(stcStatus)
                        intTrackingQuality = -1 'initialize tracking quality to illegal value
                        intNAKctr = 0
                    ElseIf blnFrameDecodedOK And intFrameType = &H2D Then ' ConRejBusy 
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBusy received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")
                        objMain.objHI.QueueCommandToHost("REJECTEDBUSY " & strCallsigns(0))
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION REJECTED BY " & stcConnection.strRemoteCallsign & ", REMOTE STATION BUSY.")
                        SetARDOPProtocolState(ProtocolState.DISC) : InitializeConnection()
                    ElseIf blnFrameDecodedOK And intFrameType = &H2E Then ' ConRejBW 
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBW received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")
                        objMain.objHI.QueueCommandToHost("REJECTEDBW " & strCallsigns(0))
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION REJECTED BY " & stcConnection.strRemoteCallsign & ", INCOMPATIBLE BW.")
                        SetARDOPProtocolState(ProtocolState.DISC) : InitializeConnection()
                    End If
                    Exit Sub

                ElseIf ARQState = ARQSubStates.ISSData Then
                    ' Process a Host command to DISCONNECT
                    If CheckForDisconnect() Then Exit Sub
                    If Not blnFrameDecodedOK Then Exit Sub ' No decode so continue repeating either data or idle
                    ' process ACK, NAK, DISC, END or BREAK here. Send ID if over 10 min. 
                    If intFrameType >= &HE0 Then ' if ACK
                        dttTimeoutTrip = Now
                        If blnLastFrameSentData Then
                            ComputeQualityAvg(38 + 2 * (intFrameType - &HE0)) ' Average ACK quality to exponential averager.
                            Gearshift_5() ' gear shift based on average quality
                        End If
                        intNAKctr = 0
                        blnEnbARQRpt = False ' stops repeat and forces new data frame or IDLE
                        SendDataOrIDLE(False) '       Send new data from outbound queue and set up repeats
                    ElseIf intFrameType <= &H1F Then ' if NAK
                        If blnLastFrameSentData Then
                            intNAKctr += 1
                            ComputeQualityAvg(38 + 2 * intFrameType) 'Average in NAK quality to exponential averager.  
                            Gearshift_5() ' gear shift based on average quality or Shift Down if intNAKcnt >= 10
                            If intShiftUpDn <> 0 Then
                                dttTimeoutTrip = Now ' Retrigger the timeout on a shift and clear the NAK counter
                                intNAKctr = 0
                                SendDataOrIDLE(True) '  Added 0.3.5.2     Restore the last frames data, Send new data from outbound queue and set up repeats
                            End If
                        End If
                        '     For now don't try and change the current data frame the simple gear shift will change it on the next frame     '           add data being transmitted back to outbound queue
                    ElseIf intFrameType = &H29 Then ' if DISC  Handles protocol rule 1.5
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send END
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        bytLastARQSessionID = stcConnection.bytSessionID ' capture this session ID to allow answering DISC from DISC state
                        blnDISCRepeating = False
                        tmrFinalID.Stop() : tmrFinalID.Start()
                        ClearDataToSend()
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()
                        blnEnbARQRpt = False
                    ElseIf intFrameType = &H2C Then ' if END
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        ClearDataToSend()
                        blnDISCRepeating = False
                        If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod, stcConnection.intCalcLeader)
                            dttLastFECIDSent = Now
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        End If
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()

                    ElseIf intFrameType = &H23 Then ' if BREAK
                        ' Initiate the transisiton to IRS
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from ARQState ISSData, Go to ProtocolState IDLE, send IDLE")
                        bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                        dttTimeoutTrip = Now
                        SetARDOPProtocolState(ProtocolState.IDLE) ': ARQState = ARQSubStates.ISSIdle
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 500) '
                        intRepeatCount = 0
                        intFrameRepeatInterval = 2000
                        blnEnbARQRpt = True ' setup for repeats if no IRS answer
                    End If
                    Exit Sub
                End If
            Case Else
                Logs.Exception("[ARDOPprotocol.ProcessRcvdARQFrame]  Unhandled Protocol state=" & GetARDOPProtocolState.ToString & "  ARQState=" & ARQState.ToString)
        End Select
    End Sub 'ProcessRcvdARQFrame

    ' Subroutine to determine the next data frame to send (or IDLE if none) 
    Private Sub SendDataOrIDLE(blnRestoreQueue As Boolean)

        Dim blnPSK As Boolean
        Dim bytFrameToSend As Byte
        Dim bytDataToMod() As Byte
        Static bytQDataInProcess(-1) As Byte

        ' Check for ID frame required (every 10 minutes)
        If blnDISCRepeating Then Exit Sub
        If blnRestoreQueue Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] Restoring " & bytQDataInProcess.Length.ToString & " bytes to OB Queue")
            RestoreDataToQueue(bytQDataInProcess)
            ReDim bytQDataInProcess(-1)
        End If
        Select Case GetARDOPProtocolState
            Case ProtocolState.IDLE
                If CheckForDisconnect() Then Exit Sub
                If intDataToSend = 0 And blnEnbARQRpt Then Exit Sub 'let repeats (Data or ILDE) continue
                If intDataToSend = 0 And Not blnEnbARQRpt Then
                    Send10MinID() ' Send ID if 10 minutes since last
                    '     Send First IDLE and setup repeat
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                    blnLastFrameSentData = False
                    If GetARDOPProtocolState <> ProtocolState.IDLE Then
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, Go to ProtocolState IDLE")
                        SetARDOPProtocolState(ProtocolState.IDLE)
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrame, intARQDefaultDlyMs)
                    intFrameRepeatInterval = 2000 ' Keep IDLE/BREAK repeats fairly long 
                    blnEnbARQRpt = True
                    dttTimeoutTrip = Now
                    Exit Sub
                End If
                If intDataToSend > 0 Then
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = " & intDataToSend.ToString & " bytes, ProtocolState IDLE > ISS")
                    SetARDOPProtocolState(ProtocolState.ISS) : ARQState = ARQSubStates.ISSData
                    Send10MinID() ' Send ID if 10 minutes since last
                    bytQDataInProcess = GetNextFrameData(intShiftUpDn, bytFrameToSend, blnPSK, False)
                    If blnPSK Then
                        bytDataToMod = objMain.objMod.EncodePSK(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.ModPSK(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("4FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        If bytFrameToSend >= &H7A And bytFrameToSend <= &H7D Then
                            intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        Else
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        End If
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("16FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("8FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    blnLastFrameSentData = True
                    intFrameRepeatInterval = 2000 ' set up 2000 ms repeat interval for Data may be able to shorten
                    'intFrameRepeatInterval = ComputeInterFrameInterval(1300) 'Fairly conservative...evaluate (based on measured leader from remote.
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = True
                    Exit Sub
                End If

            Case ProtocolState.ISS
                If CheckForDisconnect() Then Exit Sub
                Send10MinID() ' Send ID if 10 minutes since last
                If intDataToSend > 0 Then
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = " & intDataToSend.ToString & " bytes, In ProtocolState ISS")
                    ' Get the data from the buffer here based on current data frame type
                    ' (Handles protocol Rule 2.1)
                    bytQDataInProcess = GetNextFrameData(intShiftUpDn, bytFrameToSend, blnPSK, False)
                    If blnPSK Then
                        bytDataToMod = objMain.objMod.EncodePSK(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.ModPSK(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("4FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        If bytFrameToSend >= &H7A And bytFrameToSend <= &H7D Then
                            intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        Else
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        End If
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("16FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4, 8 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("8FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4, 8 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) '
                    blnLastFrameSentData = True
                    intFrameRepeatInterval = 2000 ' set up 2000 ms repeat interval for Data may be able to shorten
                    'intFrameRepeatInterval = ComputeInterFrameInterval(1300) ' fairly conservative based on measured leader from remote end 
                    ARQState = ARQSubStates.ISSData ' Should not be necessary
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = True
                Else
                    '     Send First IDLE and setup repeat
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, ProtocolState ISS > IDLE")
                    SetARDOPProtocolState(ProtocolState.IDLE)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrame, intARQDefaultDlyMs)
                    blnLastFrameSentData = False
                    intFrameRepeatInterval = 2000 ' use long interval for IDLE repeats
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = True
                    ReDim bytQDataInProcess(-1) ' added 0.3.1.3
                End If
                Exit Sub
        End Select

    End Sub  'SendDataOrIDLE

    ' Function to determine the IRS ConAck to reply based on intConReqFrameType received and local MCB.ARQBandwidth setting
    Private Function IRSNegotiateBW(intConReqFrameType As Int32) As Int32
        ' Returns the correct ConAck frame number to establish the session bandwidth to the ISS or the ConRejBW frame number if incompatible 
        ' if acceptable bandwidth sets stcConnection.intSessionBW

        With stcConnection
            Select Case MCB.ARQBandwidth.ToUpper
                Case "200FORCED"
                    If (intConReqFrameType >= &H31 And intConReqFrameType <= &H34) Or intConReqFrameType = &H35 Then
                        .intSessionBW = 200 : Return &H39 ' ConAck200
                    End If
                Case "500FORCED"
                    If (intConReqFrameType >= &H32 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H36) Then
                        .intSessionBW = 500 : Return &H3A ' ConAck500
                    End If
                Case "1000FORCED"
                    If (intConReqFrameType >= &H33 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H37) Then
                        .intSessionBW = 1000 : Return &H3B ' ConAck1000
                    End If
                Case "2000FORCED"
                    If (intConReqFrameType = &H34) Or (intConReqFrameType = &H38) Then
                        .intSessionBW = 2000 : Return &H3C ' ConAck2000
                    End If
                Case "200MAX"
                    If (intConReqFrameType >= &H31 And intConReqFrameType <= &H34) Or intConReqFrameType = &H35 Then
                        .intSessionBW = 200 : Return &H39 ' ConAck200
                    End If
                Case "500MAX"
                    If (intConReqFrameType >= &H32 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H36) Then
                        .intSessionBW = 500 : Return &H3A ' ConAck500
                    ElseIf (intConReqFrameType = &H31) Then
                        .intSessionBW = 200 : Return &H39 ' ConAck200
                    End If
                Case "1000MAX"
                    If (intConReqFrameType >= &H33 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H37) Then
                        .intSessionBW = 1000 : Return &H3B ' ConAck1000
                    ElseIf intConReqFrameType = &H31 Then
                        .intSessionBW = 200 : Return &H39 ' ConAck200
                    ElseIf intConReqFrameType = &H32 Then
                        .intSessionBW = 500 : Return &H3A ' ConAck500
                    End If
                Case "2000MAX"
                    If (intConReqFrameType = &H34) Or (intConReqFrameType = &H38) Then
                        .intSessionBW = 2000
                        Return &H3C ' ConAck2000
                    ElseIf intConReqFrameType = &H31 Then
                        .intSessionBW = 200 : Return &H39 ' ConAck200
                    ElseIf intConReqFrameType = &H32 Then
                        .intSessionBW = 500 : Return &H3A ' ConAck500
                    ElseIf intConReqFrameType = &H33 Then
                        .intSessionBW = 1000 : Return &H3B ' ConAck1000
                    End If
            End Select
        End With
        Return &H2E 'ConRejBW
    End Function 'IRSNegotiateBandwidth

    ' Subroutine to add a short 3 byte tag (ARQ, FEC, ERR, or IDF) to data and send to the host 
    Public Sub AddTagToDataAndSendToHost(bytData() As Byte, strTag As String)
        Try
            If Not (strTag = "ARQ" Or strTag = "FEC" Or strTag = "ERR" Or strTag = "IDF") Then Exit Sub
            Dim bytReturn() As Byte = GetBytes(strTag)
            ReDim Preserve bytReturn(bytData.Length + 3 - 1)
            Array.Copy(bytData, 0, bytReturn, 3, bytData.Length)
            objMain.objHI.QueueDataToHost(bytReturn)
            If MCB.CommandTrace Then Logs.WriteDebug("[AddTagTpDataAndSendToHost] bytes=" & bytReturn.Length.ToString & " Tag=" & strTag)
        Catch ex As Exception
            Logs.Exception("[AddTagTpDataAndSendToHost] Err: " & ex.ToString)
        End Try
    End Sub 'AddTagToDataAndSendToHost

    ' Function to send and ARQ connect request for the current MCB.ARQBandwidth
    Public Function SendARQConnectRequest(strMycall As String, strTargetCall As String) As Boolean
        ' Psuedo Code:
        ' Determine the proper bandwidth and target call
        ' Go to the ISS State and ISSConREq sub state
        ' Endode the connect frame with extended Leader
        ' initialize the ConReqCount and set the Frame repeat interval
        ' (Handles protocol rule 1.1) 
        Try
            InitializeConnection()
            intRmtLeaderMeas = 0
            stcConnection.strRemoteCallsign = strTargetCall.ToUpper.Trim
            stcConnection.strLocalCallsign = strMycall.ToUpper.Trim
            strFinalIDCallsign = stcConnection.strLocalCallsign
            Dim bytEncodedData() As Byte = objMain.objMod.EncodeARQConRequest(strMycall, strTargetCall, MCB.ARQBandwidth.ToUpper, strCurrentFrameFilename)
            If bytEncodedData.Length = 0 Then Return False
            ' generate the modulation with 2 x the default FEC leader length...Should insure reception at the target
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytEncodedData(0), bytEncodedData, MCB.LeaderLength)
            blnAbort = False
            dttTimeoutTrip = Now
            SetARDOPProtocolState(ProtocolState.ISS)
            ARQState = ARQSubStates.ISSConReq
            objMain.objMod.CreateWaveStream(intCurrentFrameSamples) ' This created with a bytSessionID of &HFF
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 0) ' use zero delay to allow IRS measurement of received leader. 
            intRepeatCount = 1
            stcConnection.bytSessionID = GenerateSessionID(strMycall, strTargetCall) ' Now set bytSessionID to receive ConAck (note the calling staton is the first entry in GenerateSessionID) 
            bytPendingSessionID = stcConnection.bytSessionID
            Logs.WriteDebug("[SendARQConnectRequest] strMycall=" & strMycall & "  strTargetCall=" & strTargetCall & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
            blnPending = True
            blnARQConnected = False
            intFrameRepeatInterval = 2000 ' ms ' Finn reported 7/4/2015 that 1600 was too short ...need further evaluation but temporarily moved to 2000 ms
            ' Update the main form menu status lable 
            Dim stcStatus As Status = Nothing
            stcStatus.ControlName = "mnuBusy"
            stcStatus.Text = "Calling " & strTargetCall
            queTNCStatus.Enqueue(stcStatus)
            Return True
        Catch ex As Exception
            Logs.Exception("[ARDOPprotocol.SendARQConnectRequest] Err: " & ex.ToString)
            Return False
        End Try
    End Function 'SendARQConnectRequest

    ' Function polled by Main polling loop to see if time to play next wave stream
    Public Function GetNextFrame() As Boolean
        ' Returning True sets frame pending in Main
        If MCB.ProtocolMode = "FEC" Or GetARDOPProtocolState = ProtocolState.FECSend Then
            If GetARDOPProtocolState = ProtocolState.FECSend Or GetARDOPProtocolState = ProtocolState.FECRcv Or GetARDOPProtocolState = ProtocolState.DISC Then
                Return GetNextFECFrame()
            Else
                Return False
            End If
        ElseIf MCB.ProtocolMode = "ARQ" Then
            If ARQState = ARQSubStates.None Then Return False
            Return GetNextARQFrame()
        End If
        Return False
    End Function

    ' Function to Get the next ARQ frame Returns True if frame repeating is enable 
    Private Function GetNextARQFrame() As Boolean
        Dim bytToMod(-1) As Byte

        If blnAbort Then ' handles ABORT (aka Dirty Disconnect)
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.GetNextARQFrame] ABORT...going to ProtocolState DISC, Return False")
            ClearDataToSend()
            SetARDOPProtocolState(ProtocolState.DISC)
            InitializeConnection()
            blnAbort = False
            blnEnbARQRpt = False
            blnDISCRepeating = False
            Return False
        End If
        If blnDISCRepeating Then ' handle the repeating DISC reply 
            intRepeatCount += 1
            blnEnbARQRpt = False
            If intRepeatCount > 5 Then ' do 5 tries then force disconnect 
                objMain.objHI.QueueCommandToHost("STATUS END NOT RECEIVED CLOSING ARQ SESSION WITH " & stcConnection.strRemoteCallsign)
                blnDISCRepeating = False
                blnEnbARQRpt = False
                ClearDataToSend()
                SetARDOPProtocolState(ProtocolState.DISC)
                InitializeConnection() : Return False 'indicates end repeat
            Else
                Return True ' continue with DISC repeats
            End If
        End If
        If GetARDOPProtocolState = ProtocolState.ISS And ARQState = ARQSubStates.ISSConReq Then ' Handles Repeating ConReq frames 
            intRepeatCount += 1
            If intRepeatCount > MCB.ARQConReqRepeats Then
                ClearDataToSend()
                SetARDOPProtocolState(ProtocolState.DISC)
                If stcConnection.strRemoteCallsign.Trim <> "" Then
                    objMain.objHI.QueueCommandToHost("STATUS CONNECT TO " & stcConnection.strRemoteCallsign & " FAILED!")
                    InitializeConnection() : Return False 'indicates end repeat
                Else
                    objMain.objHI.QueueCommandToHost("STATUS END ARQ CALL")
                    InitializeConnection() : Return False 'indicates end repeat
                End If
                ' Clear the mnuBusy status on the main form
                Dim stcStatus As Status = Nothing
                stcStatus.ControlName = "mnuBusy"
                queTNCStatus.Enqueue(stcStatus)
            Else
                Return True ' continue with repeats
            End If

        ElseIf GetARDOPProtocolState = ProtocolState.ISS And ARQState = ARQSubStates.IRSConAck Then ' Handles ISS repeat of ConAck
            intRepeatCount += 1
            If intRepeatCount <= MCB.ARQConReqRepeats Then
                Return True
            Else
                SetARDOPProtocolState(ProtocolState.DISC) : ARQState = ARQSubStates.DISCArqEnd
                objMain.objHI.QueueCommandToHost("STATUS CONNECT TO " & stcConnection.strRemoteCallsign & " FAILED!") : InitializeConnection() : Return False
            End If

            ' Handles a timeout from an ARQ conneceted State
        ElseIf (GetARDOPProtocolState = ProtocolState.ISS Or GetARDOPProtocolState = ProtocolState.IDLE Or GetARDOPProtocolState = ProtocolState.IRS) And _
          Now.Subtract(dttTimeoutTrip).TotalSeconds > MCB.ARQTimeout Then ' (Handles protocol rule 1.7)
            If Not blnTimeoutTriggered Then
                If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.GetNexARQFrame] Timeout setting SendTimeout timer to start.")
                blnEnbARQRpt = False
                blnTimeoutTriggered = True ' prevents a retrigger
                tmrSendTimeout.Start()
            End If
            Return False

            ' Handles the DISC state (no repeats)
        ElseIf GetARDOPProtocolState = ProtocolState.DISC Then ' never repeat in DISC state
            blnARQDisconnect = False
            Return False
        Else   ' Handles all other possibly repeated Frames
            Return blnEnbARQRpt ' not all frame types repeat...blnEnbARQRpt is set/cleared in ProcessRcvdARQFrame
        End If
        Return False
    End Function  'GetNextARQFrame

    ' This insures a report of outbound Q count of 0 is repeated at least every 10 sec to handle the case of missed OBQueue = 0 value
    Private Sub tmrPollOBQueue_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrPollOBQueue.Elapsed
        tmrPollOBQueue.Stop()
        If intDataToSend = 0 Then objMain.objHI.QueueCommandToHost("BUFFER " & intDataToSend.ToString)
        tmrPollOBQueue.Start()
    End Sub 'tmrPollOBQueue_Elapsed

    ' This sub processes a correctly decoded ConReq frame, decodes it an passed to host for display if it doesn't duplicate the prior passed frame. 
    Private Sub ProcessUnconnectedConReqFrame(intFrameType As Integer, bytData() As Byte)
        Static strLastStringPassedToHost As String = ""
        If Not (intFrameType >= &H31 And intFrameType <= &H38) Then Exit Sub
        Dim strDisplay As String = " [" & objFrameInfo.Name(intFrameType) & ": "
        Dim strCallsigns() As String = GetString(bytData).Split(" ")
        strDisplay &= strCallsigns(0) & " > " & strCallsigns(1) & "] "
        If strDisplay <> strLastStringPassedToHost Then ' suppresses repeats
            AddTagToDataAndSendToHost(GetBytes(strDisplay), "ARQ")
            strLastStringPassedToHost = strDisplay
        End If
    End Sub 'ProcessUnconnectedConReqFrame

    ' Function to compute the optimum leader based on the Leader sent and the reported Received leader
    Private Sub CalculateOptimumLeader(intReportedReceivedLeaderMS As Int32, intLeaderSentMS As Int32)
        stcConnection.intCalcLeader = Math.Max(160, 120 + intLeaderSentMS - intReportedReceivedLeaderMS) '  This appears to work well on HF sim tests May 31, 2015
        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.CalcualteOptimumLeader] Leader Sent=" & intLeaderSentMS.ToString & "  ReportedReceived=" & intReportedReceivedLeaderMS.ToString & "  Calculated=" & stcConnection.intCalcLeader.ToString)
    End Sub 'CalculateOptimumLeader

    ' a simple function to get an available frame type for the session bandwidth. 
    Function GetNextFrameData(ByRef intUpDn As Int32, ByRef bytFrameTypeToSend As Int32, ByRef blnPSK As Boolean, Optional blnInitialize As Boolean = False) As Byte()
        ' Initialize if blnInitialize = true
        '  Then call with intUpDn and blnInitialize = false:
        '       intUpDn = 0 ' use the current mode pointed to by intFrameTypePtr
        '       intUpdn < 0    ' Go to a more robust mode if available limited to the most robust mode for the bandwidth 
        '       intUpDn > 0    ' Go to a less robust (faster) mode if avaialble, limited to the fastest mode for the bandwidth

        Dim blnOdd As Boolean
        Dim intNumCar, intBaud, intDataLen, intRSLen As Int32
        Dim bytQualThresh As Byte
        Dim strType As String = ""
        Dim strShift As String = ""
        If blnInitialize Then  ' Get the array of supported frame types in order of Most robust to least robust
            bytFrameTypesForBW = objFrameInfo.GetDataModes(stcConnection.intSessionBW)
            intFrameTypePtr = 0
            bytCurrentFrameType = bytFrameTypesForBW(intFrameTypePtr)
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.GetNextFrameData] Initial Frame Type: " & objFrameInfo.Name(bytCurrentFrameType))
            intUpDn = 0
            Return Nothing ' no frame type returned on initialization
        End If

        If intUpDn < 0 Then ' go to a more robust mode
            If intFrameTypePtr > 0 Then
                intFrameTypePtr = Math.Max(0, intFrameTypePtr + intUpDn)
                bytCurrentFrameType = bytFrameTypesForBW(intFrameTypePtr)
                strShift = "Shift Down"
            End If
            intUpDn = 0
        ElseIf intUpDn > 0 Then ' go to a faster mode
            If intFrameTypePtr < (bytFrameTypesForBW.Length - 1) Then
                intFrameTypePtr = Math.Min(bytFrameTypesForBW.Length - 1, intFrameTypePtr + intUpDn)
                bytCurrentFrameType = bytFrameTypesForBW(intFrameTypePtr)
                strShift = "Shift Up"
            End If
            intUpDn = 0
        End If
        If Not objFrameInfo.IsDataFrame(bytCurrentFrameType) Then
            Logs.Exception("[ARDOPprotocol.GetNextFrameData] Frame Type " & Format(bytCurrentFrameType, "X") & " not a data type.")
            Return Nothing
        End If
        If (bytCurrentFrameType And &H1) = (bytLastARQDataFrameSent And &H1) Then
            bytFrameTypeToSend = bytCurrentFrameType Xor &H1 ' This insures toggle of  Odd and Even 
            bytLastARQDataFrameSent = bytFrameTypeToSend
        Else
            bytFrameTypeToSend = bytCurrentFrameType
            bytLastARQDataFrameSent = bytFrameTypeToSend
        End If
        If MCB.DebugLog Then
            If strShift = "" Then
                Logs.WriteDebug("[ARDOPprotocol.GetNextFrameData] No shift, Frame Type: " & objFrameInfo.Name(bytCurrentFrameType))
            Else
                Logs.WriteDebug("ARDOPprotocol.GetNextFrameData] " & strShift & ", Frame Type: " & objFrameInfo.Name(bytCurrentFrameType))
            End If
        End If
        blnPSK = objFrameInfo.Name(bytFrameTypeToSend).IndexOf("PSK") <> -1
        objFrameInfo.FrameInfo(bytCurrentFrameType, blnOdd, intNumCar, strMod, intBaud, intDataLen, intRSLen, bytQualThresh, strType)
        Dim bytReturn(-1) As Byte
        GetDataFromQueue(bytReturn, intDataLen * intNumCar)
        Return bytReturn
    End Function 'GetNextFrameData

    ' Subroutine to provide exponential averaging for reported received quality from ACK/NAK to data frame.
    Private Sub ComputeQualityAvg(intReportedQuality As Int32)
        Dim dblAlpha As Double = 0.5 ' adjust this for exponential averaging speed.  smaller alpha = slower response & smoother averages but less rapid shifting. 
        If intAvgQuality = 0 Then
            intAvgQuality = intReportedQuality
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ComputeQualityAvg] Initialize AvgQuality=" & intAvgQuality.ToString)
        Else
            intAvgQuality = Math.Round(intAvgQuality * (1 - dblAlpha) + (dblAlpha * intReportedQuality)) ' exponential averager 
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ComputeQualityAvg] Reported Quality=" & intReportedQuality.ToString & "  New Avg Quality=" & intAvgQuality.ToString)
        End If
    End Sub 'ComputeQualityAvg


    ' Subroutine to shift up to the next higher throughput or down to the next more robust data modes based on average reported quality 
    Private Sub Gearshift_5()
        ' More complex mechanism to gear shift based on intAvgQuality, current state and bytes remaining.
        ' This can be refined later with different or dynamic Trip points etc. 

        Dim intTripHi As Integer = 79 ' Modified in revision 0.4.0 (was 82)
        Dim intTripLow As Integer = 69 ' Modified in revision 0.4.0 (was 72)
        Dim intBytesRemaining As Int32 = intDataToSend
        If intNAKctr >= 5 And intFrameTypePtr > 0 Then  ' NAK threshold changed from 10 to 6 on rev 0.3.5.2
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.Gearshift_5] intNAKCtr=" & intNAKctr.ToString & " ShiftUpDn =-1")
            intShiftUpDn = -1  ' Shift down if 5 NAKs without ACK.
            intAvgQuality = (intTripHi + intTripLow) \ 2 ' init back to mid way
            intNAKctr = 0
        ElseIf intAvgQuality > intTripHi And intFrameTypePtr < (bytFrameTypesForBW.Length - 1) Then ' if above Hi Trip setup so next call of GetNextFrameData will select a faster mode if one is available 
            intShiftUpDn = 0
            If MCB.TuningRange = 0 Then
                Select Case intFrameTypePtr
                    Case 0
                        If intBytesRemaining > 64 Then
                            intShiftUpDn = 2
                        ElseIf intBytesRemaining > 32 Then
                            intShiftUpDn = 1
                        End If
                    Case 1
                        If intBytesRemaining > 200 Then
                            intShiftUpDn = 2
                        ElseIf intBytesRemaining > 64 Then
                            intShiftUpDn = 1
                        End If
                    Case 2
                        If intBytesRemaining > 400 Then
                            intShiftUpDn = 2
                        ElseIf intBytesRemaining > 200 Then
                            intShiftUpDn = 1
                        End If
                    Case 3
                        If intBytesRemaining > 600 Then intShiftUpDn = 1
                    Case 4
                        If intBytesRemaining > 512 Then intShiftUpDn = 1
                End Select
            ElseIf stcConnection.intSessionBW = 200 Then
                intShiftUpDn = 1
            Else
                If intFrameTypePtr = 0 And intBytesRemaining > 32 Then
                    intShiftUpDn = 2
                Else
                    intShiftUpDn = 1
                End If
            End If
            'If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.Gearshift_5] ShiftUpDn =" & intShiftUpDn.ToString & ", AvgQuality=" & intAvgQuality.ToString & "  Resetting to  to " & ((intTripHi + intTripLow) \ 2).ToString & " New Mode: " & objFrameInfo.Name(bytFrameTypesForBW(intFrameTypePtr + intShiftUpDn)))
            intAvgQuality = (intTripHi + intTripLow) \ 2 ' init back to mid way
            intNAKctr = 0
        ElseIf intAvgQuality < intTripLow And intFrameTypePtr > 0 Then 'if below Low Trip setup so next call of GetNextFrameData will select a more robust mode if one is available 
            intShiftUpDn = 0
            If MCB.TuningRange = 0 Then
                Select Case intFrameTypePtr
                    Case 1
                        If intBytesRemaining < 33 Then intShiftUpDn = -1
                    Case 2
                        intShiftUpDn = -1
                    Case 3
                        intShiftUpDn = -2
                    Case 4
                        intShiftUpDn = -1
                    Case 5
                        intShiftUpDn = -1
                End Select
            ElseIf stcConnection.intSessionBW = 200 Then
                intShiftUpDn = -1
            Else
                If intFrameTypePtr = 2 And intBytesRemaining < 17 Then
                    intShiftUpDn = -2
                Else
                    intShiftUpDn = -1
                End If
            End If
            ' If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.Gearshift_5] ShiftUpDn =" & intShiftUpDn.ToString & ", AvgQuality=" & intAvgQuality.ToString & "  Resetting to  to " & ((intTripHi + intTripLow) \ 2).ToString & " New Mode: " & objFrameInfo.Name(bytFrameTypesForBW(intFrameTypePtr + intShiftUpDn)))
            intAvgQuality = (intTripHi + intTripLow) \ 2 ' init back to mid way
            intNAKctr = 0
        End If
        If intShiftUpDn < 0 Then
            stcTuningStats.intShiftDNs += 1
        ElseIf intShiftUpDn > 0 Then
            stcTuningStats.intShiftUPs += 1
        End If
    End Sub 'Gearshift_5

    ' Function to check for and initiate disconnect from a Host DISCONNECT command
    Private Function CheckForDisconnect() As Boolean
        If blnARQDisconnect Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.CheckForDisconnect]  ARQ Disconnect ...Sending DISC (repeat)")
            objMain.objHI.QueueCommandToHost("STATUS INITIATING ARQ DISCONNECT")
            bytDataToMod = objMain.objMod.Encode4FSKControl(&H29, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send DISC
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
            intFrameRepeatInterval = 2000
            intRepeatCount = 1
            blnARQDisconnect = False
            blnDISCRepeating = True
            blnEnbARQRpt = False
            Return True
        End If
        Return False
    End Function 'CheckForDisconnect

    ' Function to optimize the inter frame interval based on the measured leader of remote end. 
    Private Function ComputeInterFrameInterval(intRequestedIntervalMS As Int32) As Int32
        Return CInt(Math.Max(600, intRequestedIntervalMS + intRmtLeaderMeas))
    End Function 'ComputeInterFrameInterval

    '  Event triggered by tmrSendTimeout elapse. Ends an ARQ session and sends a DISC frame 
    Private Sub tmrSendTimeout_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrSendTimeout.Elapsed
        tmrSendTimeout.Stop()
        Dim bytEncodedBytes(-1) As Byte

        ' (Handles protocol rule 1.7)
        Dim dttStartWait As Date = Now
        While objMain.blnLastPTT And Now.Subtract(dttStartWait).TotalSeconds < 10
            Thread.Sleep(50)
        End While
        If MCB.DebugLog Then Logs.WriteDebug("ARDOPprotocol.tmrSendTimeout]  ARQ Timeout from ProtocolState: " & GetARDOPProtocolState.ToString & "  Going to DISC state")
        ' Confirmed proper operation of this timeout and rule 4.0 May 18, 2015
        ' Send an ID frame (Handles protocol rule 4.0)
        If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
            bytEncodedBytes = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytEncodedBytes, stcConnection.intCalcLeader)
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
            dttLastFECIDSent = Now
        End If
        If MCB.AccumulateStats Then LogStats()
        objMain.objHI.QueueCommandToHost("DISCONNECTED")
        objMain.objHI.QueueCommandToHost("STATUS ARQ Timeout from Protocol State: " & GetARDOPProtocolState.ToString & " @ " & TimestampEx())
        blnEnbARQRpt = False
        Thread.Sleep(2000)
        ClearDataToSend()
        bytDataToMod = objMain.objMod.Encode4FSKControl(&H29, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send DISC
        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
        intFrameRepeatInterval = 2000
        SetARDOPProtocolState(ProtocolState.DISC)
        InitializeConnection() ' reset all Connection data
        ' Clear the mnuBusy status on the main form
        Dim stcStatus As Status = Nothing
        stcStatus.ControlName = "mnuBusy"
        stcStatus.Text = "FALSE"
        queTNCStatus.Enqueue(stcStatus)
        blnTimeoutTriggered = False ' prevents a retrigger
    End Sub 'tmrSendTimeout_Elapsed

    ' Elapsed Subroutine for Pending timeout
    Private Sub tmrIRSPendingTimeout_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrIRSPendingTimeout.Elapsed
        tmrIRSPendingTimeout.Stop()
       
        If MCB.DebugLog Then Logs.WriteDebug("ARDOPprotocol.tmrIRSPendingTimeout]  ARQ Timeout from ProtocolState: " & GetARDOPProtocolState.ToString & "  Going to DISC state")
        objMain.objHI.QueueCommandToHost("DISCONNECTED")
        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECT REQUEST TIMEOUT FROM PROTOCOL STATE: " & GetARDOPProtocolState.ToString & " @ " & TimestampEx())
        blnEnbARQRpt = False
        Thread.Sleep(2000)
        SetARDOPProtocolState(ProtocolState.DISC)
        InitializeConnection() ' reset all Connection data
        ' Clear the mnuBusy status on the main form
        Dim stcStatus As Status = Nothing
        stcStatus.ControlName = "mnuBusy"
        stcStatus.Text = "FALSE"
        queTNCStatus.Enqueue(stcStatus)
    End Sub  'tmrIRSPendingTimeout_Elapsed

    ' Subroutine for tmrFinalIDElapsed
    Private Sub tmrFinalID_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrFinalID.Elapsed
        Dim bytDataToMod() As Byte

        tmrFinalID.Stop()
        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.tmrFinalID_Elapsed]  Send Final ID (" & strFinalIDCallsign & ", [" & MCB.GridSquare & "])")
        If CheckValidCallsignSyntax(strFinalIDCallsign) Then
            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(strFinalIDCallsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod)
            dttLastFECIDSent = Now
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 1000)
        End If
    End Sub  ' tmrFinalIDElapsed

    ' Function to send 10 minute ID
    Private Function Send10MinID() As Boolean
        Dim dttSafetyBailout As Date = Now
        If Now.Subtract(dttLastFECIDSent).TotalMinutes > 10 And Not blnDISCRepeating Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPptocol.Send10MinID] Send ID Frame")
            ' Send an ID frame (Handles protocol rule 4.0)
            blnEnbARQRpt = False
            Dim bytEncodedBytes() As Byte = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytEncodedBytes)
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
            dttLastFECIDSent = Now
            ' Hold until PTT goes true
            While objMain.blnLastPTT = False And Now.Subtract(dttSafetyBailout).TotalMilliseconds < 4000
                Thread.Sleep(100)
            End While
            ' Now hold until PTT goes false
            While objMain.blnLastPTT = True And Now.Subtract(dttSafetyBailout).TotalMilliseconds < 4000
                Thread.Sleep(100)
            End While
            If Now.Subtract(dttSafetyBailout).TotalMilliseconds > 4000 Then
                Logs.Exception("[ARDOPprotocol.Send10MinID] Safety bailout!")
                Return True
            Else
                Thread.Sleep(200)
                Return True
            End If
        Else
            Return False
        End If
    End Function  'Send10MinID()
End Class