
Imports System.Runtime.InteropServices
Public Class PortAudioVB


    ' May 14, 2014 Work in process .... eventually move direct X to PortAudio
    'Public Structure PaDeviceInfo
    '    Public structVersion As Integer
    '    Public name As Integer
    '    Public PaHostApiIndex As Integer
    '    Public intFrameTypeDecodes As Integer
    '    Public maxInputChannels As Integer
    '    Public maxOutputChannels As Integer
    '    Public defaultLowInputLatency As Double
    '    Public defaultLowOutputLatency As Double
    '    Public defaultHighInputLatency As Double
    '    Public DefaultHighOutplatency As Double
    '    Public defaultSampleRate As Double
    '    Public blnStatsValid As Boolean
    'End Structure

    Private Declare Sub Pa_Initialize Lib "PortAudio.dll" ()
    Private Declare Sub Pa_Terminate Lib "PortAudio.dll" ()
    'Private Declare Function Pa_GetVersionText Lib "PortAudio.dll" () As String
    Private Declare Function Pa_GetVersion Lib "PortAudio.dll" () As Int32
    Private Declare Sub Pa_Sleep Lib "PortAudio.dll" (intSleepMS As Int32)
    Private Declare Function Pa_GetDeviceCount Lib "PortAudio.dll" () As Int32
    Private Declare Function Pa_GetDefaultInputDevice Lib "PortAudio.dll" () As Int32
    Private Declare Function Pa_GetDefaultOutputDevice Lib "PortAudio.dll" () As Int32
    Private Declare Function Pa_GetDeviceInfo Lib "PortAudio.dll" (intIndex As Integer) As PaDeviceInfo

    ''to write to memory
    'Public Declare Sub CopyMemoryWrite Lib "kernel32" Alias _
    '    "RtlMoveMemory" (ByVal Destination As Long, Source As Any, _
    '    ByVal Length As Long)

    '' to read from memory
    'Public Declare Sub CopyMemoryRead Lib "kernel32" Alias _
    '    "RtlMoveMemory" (Destination As Any, ByVal Source As Long, _
    '    ByVal Length As Long)
    

    <StructLayout(LayoutKind.Sequential)> Public Class PaDeviceInfo
        Public structVersion As Integer
        Public name As Integer
        Public PaHostApiIndex As Integer
        Public intFrameTypeDecodes As Integer
        Public maxInputChannels As Integer
        Public maxOutputChannels As Integer
        Public defaultLowInputLatency As Double
        Public defaultLowOutputLatency As Double
        Public defaultHighInputLatency As Double
        Public DefaultHighOutplatency As Double
        Public defaultSampleRate As Double
        Public blnStatsValid As Boolean
    End Class
    'End Structure 'PaDeviceInfo

    '    int 	structVersion

    'const char * 	name

    'PaHostApiIndex 	hostApi

    'int 	maxInputChannels

    'int 	maxOutputChannels

    'PaTime 	defaultLowInputLatency

    'PaTime 	defaultLowOutputLatency

    'PaTime 	defaultHighInputLatency

    'PaTime 	defaultHighOutputLatency

    'double 	defaultSampleRate

    '?stcDeviceInfo
    '{ARDOP_Win1.PortAudioVB.PaDeviceInfo}
    '    blnStatsValid: True
    '    defaultHighInputLatency: 0.4
    '    DefaultHighOutplatency: 0.4
    '    defaultLowInputLatency: 0.2
    '    defaultLowOutputLatency: 0.2
    '    defaultSampleRate: 44100.0
    '    intFrameTypeDecodes: 2
    '    maxInputChannels: 0
    '    maxOutputChannels: 0
    '    name: 71260896
    '    PaHostApiIndex: 0
    '    structVersion: 2

    ' used for testing functions and subs
    Public Sub New()
        Try
            Dim stcDeviceInfo As PaDeviceInfo = Nothing
            Dim strName As String
            Pa_Initialize()
            Dim intVersion As Integer = Pa_GetVersion
            Dim intDevCount As Integer = Pa_GetDeviceCount
            Dim intDefaultInput As Integer = Pa_GetDefaultInputDevice
            Dim intDefaultOutput As Integer = Pa_GetDefaultOutputDevice

            For i As Integer = 0 To intDevCount - 1
                Try
                    stcDeviceInfo = Pa_GetDeviceInfo(i)
                    If stcDeviceInfo IsNot Nothing Then
                        strName = Marshal.PtrToStringAnsi(stcDeviceInfo.name)
                        Debug.WriteLine("Device:" & i.ToString & "  name:" & strName)
                    End If
                Catch
                End Try
            Next i
            Pa_Sleep(100)
            Pa_Terminate()


        Catch ex As Exception
            Debug.WriteLine(ex.ToString)
        End Try

    End Sub
End Class
