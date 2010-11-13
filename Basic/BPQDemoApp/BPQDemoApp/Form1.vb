Public Class Form1



    ' Global Variables and Code for BPQ32 support

    '  John Wiseman G8BPQ October 2008

    Public BPQStream(64) As Integer           ' BPQ Stream Numbers
    Public BPQConnectionID(64) As Integer     ' Map BPQ Stream to ConnectionID

    Public BPQmsg As UInt16

    Public Declare Function GetNumberofPorts Lib "bpq32.dll" Alias "_GetNumberofPorts@0" () As Int32

    Public Declare Sub CheckTimer Lib "bpq32.dll" Alias "_CheckTimer@0" ()

    Public Declare Function GetPortDescription Lib "bpq32.dll" Alias _
       "_GetPortDescription@8" (ByVal Port As Int32, ByVal Desc As String) As Int32

    Public Declare Function SendRaw Lib "bpq32.dll" Alias _
     "_SendRaw@12" (ByVal Port As Int32, ByVal Msg As String, ByVal Len As Int32) As Int32


    Public Declare Function GetApplCallVB Lib "bpq32.dll" Alias _
       "_GetApplCallVB@8" (ByVal Appl As Int32, ByVal ApplCall As String) As Int32

    Public ConsoleStream As Integer

    Public NumberofRadioPorts As Integer

    Public Portmon(16) As Boolean

    Public Declare Function FindFreeStream Lib "bpq32.dll" Alias _
       "_FindFreeStream@0" () As Int32

    Public Declare Function NumberofPorts Lib "bpq32.dll" Alias _
       "_GetNumberofPorts@0" () As Int32

    Public Declare Function DeallocateStream Lib "bpq32.dll" Alias _
        "_DeallocateStream@4" (ByVal Stream As Int32) As Int32

    Public Declare Function BPQSetHandle Lib "bpq32.dll" Alias _
       "_BPQSetHandle@8" (ByVal Window As Int32, ByVal Handle As Int32) As Int32

    Public Declare Function SessionControl Lib "bpq32.dll" Alias _
       "_SessionControl@12" (ByVal Handle As Int32, ByVal action As Int32, ByVal flag As Int32) As Int32

    Public Declare Function SessionState Lib "bpq32.dll" Alias _
       "_SessionState@12" (ByVal Stream As Int32, ByRef State As Int32, ByRef Changed As Int32) As Int32

    Public Declare Function SessionStateNoAck Lib "bpq32.dll" Alias _
       "_SessionStateNoAck@8" (ByVal Stream As Int32, ByRef State As Int32) As Int32

    Public Declare Function SetAppl Lib "bpq32.dll" Alias _
      "_SetAppl@12" (ByVal Handle As Int32, ByVal Flags As Int32, ByVal Mask As Int32) As Int32

    Public Declare Function GetMsg Lib "bpq32.dll" Alias _
        "_GetMsg@16" (ByVal Stream As Int32, ByVal Buffer As String, _
        ByRef Len As Int32, ByRef count As Int32) As Int32

    Public Declare Function SendMsg Lib "bpq32.dll" Alias _
         "_SendMsg@12" (ByVal Stream As Int32, ByVal Buffer As String, _
         ByVal Len As Int32) As Int32


    Public Declare Function GetCallsign Lib "bpq32.dll" Alias _
         "_GetCallsign@8" (ByVal Stream As Int32, ByVal Buffer As String) As Int32

    Public Declare Function GetRaw Lib "bpq32.dll" Alias _
       "_GetRaw@16" (ByVal Stream As Int32, ByVal Buffer As String, _
       ByRef Len As Int32, ByRef count As Int32) As Int32

    Public Declare Function DecodeFrame Lib "bpq32.dll" Alias _
     "_DecodeFrame@12" (ByVal Buffer As String, ByVal buffer2 As String, ByVal Stamp As Int32) As Int32

    Public Declare Function RegisterWindowMessage Lib "user32.dll" Alias _
        "RegisterWindowMessageA" (ByVal Msg As String) As Int32

    Public Const BPQMonitorAvail = 1
    Public Const BPQDataAvail = 2
    Public Const BPQStateChange = 4

    Public Sub ConnecttoBPQ32()

        '  Load BPQ Control


    End Sub

    Public Function ConvToAX25(ByVal strCall As String, ByVal ax25call As Byte()) As Boolean

        Dim intindex As Integer
        Dim bytNext As Byte
        Dim intSSID As Integer

        For intindex = 0 To 5
            ax25call(intindex) = &H40 ' in case short
        Next intindex

        ax25call(6) = &H60

        strCall = strCall.Trim

        For intindex = 0 To strCall.Length - 1

            bytNext = CByte(Asc(strCall.Substring(intindex, 1)))

            If bytNext = Asc("-") Then

                '//	process ssid and return

                intSSID = CInt(strCall.Substring(intindex + 1))

                If intSSID < 16 Then

                    ax25call(6) = CByte(&H60 Or (intSSID + intSSID))
                    Return True

                End If

                Return False   ' Invalid ssid

            End If

            If intindex = 6 Then Return False ' Too Long

            ax25call(intindex) = bytNext + bytNext

        Next

        Return True

    End Function


    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Try

            GetNumberofPorts()   ' Will fail if BPQ32 not available

            CheckTimer()         ' Forces Initialisation

        Catch ex As Exception

            MsgBox(Err.Description)

        End Try

        BPQmsg = RegisterWindowMessage("BPQWindowMessage")

        Debug.Print(BPQmsg)
        Debug.Print(Hex(BPQmsg))

        '   If BPQmsg > 32767 Then BPQmsg = -(65536 - BPQmsg)

        Debug.Print(Hex(BPQmsg))
        Debug.Print(BPQmsg)

        ConsoleStream = FindFreeStream()

        If (ConsoleStream <> 255) Then

            BPQSetHandle(ConsoleStream, MyBase.Handle)

        End If

        '       SessionControl(ConsoleStream, 1, 0)

        SetAppl(ConsoleStream, 128 + 64 + 7, 1)




    End Sub


    Protected Overrides Sub WndProc(ByRef m As Message)
        ' Listen for operating system messages

        If m.Msg < 0 Or m.Msg > 1024 Then
            '      Debug.Print("xx")

        End If
        Select Case (m.Msg)
            ' The WM_ACTIVATEAPP message occurs when the application
            ' becomes the active application or becomes inactive.
            Case BPQmsg

                Dim Stream As Int32, Change As Int32, State As Int32, Changed As Int32

                Stream = m.WParam

                Change = m.LParam


                Select Case (Change)

                    Case BPQMonitorAvail

                        MonDataAvail(Stream)

                    Case BPQDataAvail

                        DataAvail(Stream)

                    Case BPQStateChange

                        '	Get current Session State. Any state changed is ACK'ed
                        '	automatically. See BPQHOST functions 4 and 5.

                        SessionState(Stream, State, Changed)

                        If (Changed = 1) Then

                            If (State = 1) Then

                                ' Connected

                                Connected(Stream)

                            Else

                                Disconnected(Stream)

                            End If

                        End If

                End Select


        End Select
        MyBase.WndProc(m)


    End Sub
    Sub Connected(ByVal Stream)

        Dim Callsign As String

        Callsign = Space(10)
        GetCallsign(Stream, Callsign)

        '    Form1.Caption = "BPQ Telnet Server V 0.4 OCX Version " & BPQCtrl1.GetVersion & " Stream " & stream & " Connected to " & BPQCtrl1.CallSign(stream)
        Debug.Print("BPQ Terminal Version 0.6x Beta Stream " & Stream & " Connected to " & Callsign)


    End Sub

    Sub DataAvail(ByVal Stream)

        Dim Buffer As String
        Dim start As Integer
        Dim MsgLen As Integer, Count As Integer


        Buffer = Space(350)

        GetMsg(Stream, Buffer, MsgLen, Count)

  
    End Sub

    Sub MonDataAvail(ByVal Stream)

        Dim Stamp As Integer, Count As Integer, Port As Integer, MsgLen As Integer
        Dim start As Integer
        Dim Buffer As String
        Dim Buffer2 As String
        Dim n As Integer

Monloop:

        Buffer = Space(350)

        Stamp = GetRaw(Stream, Buffer, MsgLen, Count)

 
    End Sub

    Sub Disconnected(ByVal Stream As Int32)

        Debug.Print("BPQ Terminal Version 0.6x Beta  Disconnected")

    End Sub

    Function ConnectState(ByVal Stream As Int32)

        Dim State As Int32

        SessionStateNoAck(Stream, State)

        Return State

    End Function


End Class
