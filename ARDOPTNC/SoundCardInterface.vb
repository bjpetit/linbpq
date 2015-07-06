
Imports System.Runtime.InteropServices
Imports System.Threading
Imports System.IO


Public Module SoundCardInterface

    ' John Wiseman G8BPQ June 2015

    ' Isolates all ALSA soundcard handling code in one place.

    ' This is the managed interface to the unmanaged library that talks to ALSA

    <DllImport("libardop")> Public Function CheckifLoaded() As Integer
    End Function

    <DllImport("libardop")> Public Function GetInputDeviceCollection() As Integer
    End Function

    <DllImport("libardop")> Public Function GetOutputDeviceCollection() As Integer
    End Function

    <DllImport("libardop")> Public Function GetNextInputDevice(dest As System.Text.StringBuilder, len As Integer, n As Integer) As Integer
    End Function

    <DllImport("libardop")> Public Function GetNextOutputDevice(dest As System.Text.StringBuilder, len As Integer, n As Integer) As Integer
    End Function

    <DllImport("libardop")> Public Function OpenSoundCard(CaptureDevice As String, PlaybackDevice As String, m_sampleRate As Integer) As Boolean
    End Function

    <DllImport("libardop")> Public Function CloseSoundCard() As Boolean
    End Function

    <DllImport("libardop")> Public Function SoundCardWrite(input As Byte(), nSamples As Integer) As Integer
    End Function

    <DllImport("libardop")> Public Function SoundCardRead(input As IntPtr, len As Integer) As Integer
    End Function

    <DllImport("libardop")> Public Function SoundCardClearInput() As Integer
    End Function


    Private objCodecLock As New Object

    Private Playing As Boolean
    Public PlayComplete As Boolean


    Public Function CheckforALSA() As Boolean

        '   Return True if ALSA interface library is avaliable

        Try
            Dim n As Integer = CheckifLoaded()
        Catch ex As Exception
            Return False
        End Try
        Return True

    End Function
    Public Function AlsaStartCodec(ByRef strFault As String) As Boolean
        'Returns true if successful
        Thread.Sleep(100) ' This delay is necessary for reliable starup following a StopCodec


        SyncLock objCodecLock

            Main.dttLastSoundCardSample = Now
            Dim blnSpectrumSave As Boolean = MCB.DisplaySpectrum
            Dim blnWaterfallSave As Boolean = MCB.DisplayWaterfall
            Dim dttStartWait As Date = Now
            MCB.DisplayWaterfall = False
            MCB.DisplaySpectrum = False
            Dim strCaptureDevices() As String = EnumerateCaptureDevices()
            Dim strPlaybackDevices() As String = EnumeratePlaybackDevices()
            AlsaStartCodec = False
            '            Dim objDI As New DeviceInformation
            Dim intPtr As Integer = 0

            '            stcSCFormat.Channels = 1
            '            stcSCFormat.BitsPerSample = 16
            '            stcSCFormat.BlockAlign = 2
            '            stcSCFormat.AverageBytesPerSecond = 2 * 12000 ' 12000
            '                      stcSCFormat.FormatTag = WaveFormatTag.Pcm
            '                     objApplicationNotify = Nothing
            '                    objCapture = Nothing
            ' Set the buffer sizes
            Main.intCaptureBufferSize = 8192 ' I think this needs to be set
            ' Create the capture buffer
            '                     dscheckboxd.BufferBytes = intCaptureBufferSize
            '                   stcSCFormat.FormatTag = WaveFormatTag.Pcm
            '                    dscheckboxd.Format = stcSCFormat ' Set the format during creatation
            '                  If Not IsNothing(objCapture) Then
            'objCapture.Dispose()
            'objCapture = Nothing
            'End If
            '          intNextCaptureOffset = 0
            Main.dttCodecStarted = Now
            Main.WriteTextToSpectrum("CODEC Start OK", Brushes.LightGreen)
            Main.blnGraphicsCleared = False
   
            Main.blnCodecStarted = OpenSoundCard(MCB.CaptureDevice, MCB.PlaybackDevice, 12000)

            AlsaStartCodec = Main.blnCodecStarted
            If Not AlsaStartCodec Then
                strFault = "Start Codec:Failed " & MCB.CaptureDevice.ToUpper
                CloseSoundCard()            ' Make sure any handles are closed
            End If

            Thread.Sleep(100)

            If AlsaStartCodec Then
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StartCodec] Successful start of codec")
                Main.objProtocol.SetARDOPProtocolState(ProtocolState.DISC)
                If Not IsNothing(Main.objRadio) Then Main.objRadio.PTT(False) ' insurance for PTT off

                ' Create a thread to monitor the notify events
                If Nothing Is Main.thrNotify Then
                    Main.thrNotify = New Thread(New ThreadStart(AddressOf Main.CaptureWaitThread))
                    Main.thrNotify.Priority = ThreadPriority.Highest
                    ' Create a notification event, for when the sound stops playing
                    Main.objNotificationEvent = New AutoResetEvent(False)
                    Main.thrNotify.Start()
                End If

                strFault = ""
            Else
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StartCodec] CODEC Start Failed")
                Main.WriteTextToSpectrum("CODEC Start Failed", Brushes.Red)
                Main.blnGraphicsCleared = False
                Main.objProtocol.SetARDOPProtocolState(ProtocolState.OFFLINE)
                Main.tmrStartCODEC.Interval = 5000
                Main.tmrStartCODEC.Start()
                Main.blnCodecStarted = False
            End If
            MCB.DisplayWaterfall = blnWaterfallSave
            MCB.DisplaySpectrum = blnSpectrumSave

        End SyncLock


    End Function

    Public Function AlsaStopCodec(ByRef strFault As String) As Boolean
        ' Stop the capture


        SyncLock objCodecLock
            Try
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] Stop Main.thrNotify with blnSCCapturing = False")
                Main.blnSCCapturing = False ' this should end the wait thread if it is still running
                Thread.Sleep(200)
                If Main.thrNotify IsNot Nothing AndAlso Main.thrNotify.IsAlive Then
                    If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] Aborting Main.thrNotify")
                    Main.thrNotify.Abort()
                    Thread.Sleep(100)
                    Main.thrNotify.Join(3000)
                End If
                Main.thrNotify = Nothing

                CloseSoundCard()

                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] = True")
                AlsaStopCodec = True

                Main.WriteTextToSpectrum("CODEC Stopped", Brushes.Yellow)
                Main.objProtocol.SetARDOPProtocolState(ProtocolState.OFFLINE)
                Main.tmrStartCODEC.Stop()
                strFault = ""
            Catch ex As Exception
                Logs.Exception("[Main.StopCodec] Err: " & ex.ToString)
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] = False")
                strFault = ("StopCodec:" & Err.Number.ToString & "/" & Err.Description).Trim
                AlsaStopCodec = False
            End Try
        End SyncLock

    End Function

    Sub AlsaAbortSoundStream()

    End Sub

    Function ALSASoundIsPlaying()
        Return Playing
    End Function

    Function AlsaEnumeratePlaybackDevices() As String()
        ' Get the Windows enumerated Playback devices adding a "-n" tag (-1, -2, etc.)  if any duplicate names 

        Dim Count As Integer = GetOutputDeviceCollection()

        Dim strPlaybackDevices(Count - 1) As String
        Dim sb As System.Text.StringBuilder

        For n As Integer = 0 To Count - 1

            sb = New System.Text.StringBuilder(256)
            GetNextOutputDevice(sb, sb.Capacity, n)
            strPlaybackDevices(n) = sb.ToString
 
        Next

        Return strPlaybackDevices

    End Function

    Public Function AlsaEnumerateCaptureDevices() As String()

        Dim Count As Integer = GetInputDeviceCollection()          ' Get names into internal buffer in .dll

        Dim strCaptureDevices(Count - 1) As String
        Dim sb As System.Text.StringBuilder

        For n As Integer = 0 To Count - 1

            sb = New System.Text.StringBuilder(256)
            GetNextInputDevice(sb, sb.Capacity, n)
            strCaptureDevices(n) = sb.ToString
 
        Next

        Return strCaptureDevices

    End Function

    Private soundwritethread As Thread

    ' Plays the Wave stream, updates the form display and keys the PTT.

    Public Function AlsaPlaySoundStream() As Boolean
        '   Plays the .wav stream with the selected Playback device
        '   Returns True if no errors False otherwise...
        Dim intTrace As Integer = 0
        Dim dttStartPlay As Date
        Dim stcStatus As Status = Nothing

        If Not Main.blnCodecStarted Then Return False
        If IsNothing(memWaveStream) Then
            Logs.Exception("[Main.PlaySoundStream] memWaveStream is nothing")
            Console.WriteLine("PlaySoundStream memWaveStream is nothing")

            Return False
            '       ElseIf objPlayback IsNot Nothing AndAlso objPlayback.Status.Playing Then
            '          Logs.Exception("[Main.PlaySoundStream] objPlayback is Playing, Call AbortWaveStream")
            '          AbortSoundStream()
        End If
        intTrace = 1

        '   The main sound card write code is in a separate thread, as it is difficult to signal back from native code when the
        '   stream has been sent, so the thread runs until the native code returns, then drops PTT and signals TX Complete

        Playing = True

        dttTestStart = Now
        dttStartPlay = Now
        stcStatus.ControlName = "lblXmtFrame"
        stcStatus.Text = strLastWavStream
        stcStatus.BackColor = Color.LightSalmon
        queTNCStatus.Enqueue(stcStatus)

        AlsaPlaySoundStream = True

        soundwritethread = New Thread(AddressOf PlaySoundStreamThread)
        soundwritethread.Priority = ThreadPriority.Highest
        soundwritethread.IsBackground = False

        Main.KeyPTT(True) ' Activate PTT before starting sound play

        soundwritethread.Start()

    End Function

    Public Function PlaySoundStreamThread() As Boolean

        Dim intTrace As Integer = 0

        Try

            memWaveStream.Seek(0, SeekOrigin.Begin) ' reset the pointer to the origin

            Dim bytes() As Byte
            Dim nSamples As Integer = memWaveStream.Length

            ReDim bytes(nSamples + 100)

            memWaveStream.Read(bytes, 0, nSamples)

            Dim ret As Integer = SoundCardWrite(bytes, nSamples)

            SoundCardClearInput()           ' Clear capture buffer

            Playing = False
            PlayComplete = True         ' This will signal BG to drop PTT

            If MCB.DebugLog Then Logs.WriteDebug("[PlaySoundStream] Stream: " & strLastWavStream)

        Catch e As Exception
            Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  Kill PTT on exception: " & e.ToString)
            If Not IsNothing(memWaveStream) Then
                Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  memWaveStream Length =" & memWaveStream.Length.ToString & "  strLastWaveStream=" & strLastWavStream)
            Else
                Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  memWaveStream was Nothing  strLastWavStream=" & strLastWavStream)
            End If

            Playing = False
            PlayComplete = True
            Main.KeyPTT(False)              ' as a precaution

        End Try

        Return True

    End Function

    Public Function AlsaClearInputSamples() As Integer

        ' Discard input to prevent underrun

        Return SoundCardClearInput()

    End Function

    Public Function AlsaGetSamples(ByRef bytCaptureData() As Byte) As Integer

        ' Initialize unmanaged memory to hold the samples.

        Dim Count As Integer
        Dim Buffer As IntPtr = Marshal.AllocHGlobal(16384)      ' will normally only get 1024 +- timing jitter

        Count = SoundCardRead(Buffer, 16384)

        ReDim bytCaptureData(Count - 1)

        ' Copy the unmanaged array back to the managed array. 

        Marshal.Copy(Buffer, bytCaptureData, 0, Count)

        Marshal.FreeHGlobal(Buffer)

        Return Count

    End Function

End Module
