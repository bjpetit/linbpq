
Imports System.Threading
Imports System
Imports System.Text

Public Class SerialLink

   Private Const GENERIC_WRITE As UInteger = &H40000000
   Private Const GENERIC_READ As Long = &H80000000
   Private Const FILE_ATTRIBUTE_NORMAL As UInteger = &H80
   Private Const CREATE_ALWAYS As UInteger = 2
   Private Const OPEN_ALWAYS As UInteger = 4
   Private Const INVALID_HANDLE_VALUE As Long = -1
   Const OPEN_EXISTING As UInteger = 3
   Const FILE_SHARE_READ As UInteger = &H1
   Const FILE_SHARE_WRITE As UInteger = &H2
   Const FILE_FLAG_NO_BUFFERING As UInteger = &H20000000

   Const FILE_DEVICE_SERIAL_PORT As UInteger = &H1B
   Const METHOD_BUFFERED As UInteger = 0
   Const FILE_ANY_ACCESS As UInteger = 0

   Const IOCTL_SERIAL_IS_COM_OPEN As UInteger = FILE_DEVICE_SERIAL_PORT << 16 Or &H800 << 2
   Const IOCTL_SERIAL_GETDATA As UInteger = (FILE_DEVICE_SERIAL_PORT << 16) Or (&H801 << 2)
   Const IOCTL_SERIAL_SETDATA As UInteger = (FILE_DEVICE_SERIAL_PORT << 16) Or (&H802 << 2)

   Private Declare Function CloseHandle Lib "kernel32" _
   (ByVal hObject As Long) As Long

   Private Declare Function WriteFile Lib "kernel32" _
      (ByVal hFile As Long, ByVal lpBuffer As String, _
      ByVal nNumberOfBytesToWrite As Long, _
      ByVal lpNumberOfBytesWritten As Long, _
      ByVal lpOverlapped As Long) As Long


   Private Declare Function CreateFile Lib "kernel32" Alias "CreateFileA" _
      (ByVal lpFileName As String, _
      ByVal dwDesiredAccess As UInteger, _
      ByVal dwShareMode As UInteger, _
      ByVal lpSecurityAttributes As UInteger, _
      ByVal dwCreationDisposition As UInteger, _
      ByVal dwFlagsAndAttributes As UInteger, _
      ByVal hTemplateFile As UInteger) As UInteger

   Private Declare Function DeviceIoControl Lib "kernel32" (ByVal hDevice As UInteger, ByVal dwIoControlCode As UInteger, _
      ByVal lpInBuffer As String, ByVal nInBufferSize As Integer, ByVal lpOutBuffer As String, _
      ByVal nOutBufferSize As Integer, ByRef lpBytesReturned As Integer, ByVal lpOverlapped As UInteger) As UInteger


   Dim TCPOpen As Boolean
   Dim VCOMOpen As Boolean

   Private Sub SerialLink_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      TCPHostName.Text = My.Settings.TCPHost
      TCPPort.Text = My.Settings.TCPPort

      OptSerial.Checked = My.Settings.OptSerial
      OptTCPMaster.Checked = My.Settings.OptTCPMaster

      COMTypeR.Checked = My.Settings.RealCOM
      COMTypeV.Checked = My.Settings.VirtualCOM

      OptTCPSlave.Checked = My.Settings.OptTCPSlave

      For Each sp As String In My.Computer.Ports.SerialPortNames
         LocalCOM.Items.Insert(0, sp)
         RemoteCOM.Items.Insert(0, sp)
      Next

      LocalCOM.SelectedIndex = LocalCOM.FindString(My.Settings.LocalCOM)
      RemoteCOM.SelectedIndex = RemoteCOM.FindString(My.Settings.RemoteCOM)

      LocallBaud.Items.Add("38400")
      LocallBaud.Items.Add("19200")
      LocallBaud.Items.Add("9600")
      LocallBaud.Items.Add("4800")
      LocallBaud.Items.Add("2400")
      LocallBaud.Items.Add("1200")

      LocallBaud.SelectedIndex = LocallBaud.FindString(My.Settings.LocalBaud)

      RemoteBAUD.Items.Add("38400")
      RemoteBAUD.Items.Add("19200")
      RemoteBAUD.Items.Add("9600")
      RemoteBAUD.Items.Add("4800")
      RemoteBAUD.Items.Add("2400")
      RemoteBAUD.Items.Add("1200")

      RemoteBAUD.SelectedIndex = RemoteBAUD.FindString(My.Settings.RemoteBaud)

   End Sub

   Private Sub SerialPort1_DataReceived(ByVal sender As Object, ByVal e As System.IO.Ports.SerialDataReceivedEventArgs) Handles SerialPort1.DataReceived

      Dim len As Integer = SerialPort1.BytesToRead
      Dim Msg(len) As Byte
      Dim i As Integer

      For i = 0 To len - 1
         Msg(i) = SerialPort1.ReadByte()
      Next
      SerialPort2.Write(Msg, 0, len)

   End Sub

   Private Sub SerialPort2_DataReceived(ByVal sender As Object, ByVal e As System.IO.Ports.SerialDataReceivedEventArgs) Handles SerialPort2.DataReceived

      Dim len As Integer
      Dim LastLen As Integer = SerialPort2.BytesToRead
waitloop:
      Thread.Sleep(10)
      len = SerialPort2.BytesToRead
      If len > LastLen Then
         LastLen = len
         GoTo waitloop
      End If

      Dim Msg(len - 1) As Byte
      Dim i As Integer

      For i = 0 To len - 1
         Msg(i) = SerialPort2.ReadByte

      Next

      If OptSerial.Checked Then
         SerialPort1.Write(Msg, 0, len)
      Else
         Try
            If TCPOpen Then AxWinsock1.SendData(Msg)
         Catch ex As Exception
         End Try
      End If

   End Sub


   Private Sub Open_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Open.Click

      My.Settings.TCPHost = TCPHostName.Text
      My.Settings.TCPPort = TCPPort.Text

      My.Settings.OptSerial = OptSerial.Checked
      My.Settings.OptTCPMaster = OptTCPMaster.Checked
      My.Settings.OptTCPSlave = OptTCPSlave.Checked

      SerialPort2.Close()

      Try

         If OptSerial.Checked Then

            SerialPort1.Open()
  
         ElseIf OptTCPSlave.Checked Then

            AxWinsock1.Close()

            AxWinsock1.RemoteHost = "0.0.0.0"
            AxWinsock1.RemotePort = 0
            AxWinsock1.LocalPort = CInt(TCPPort.Text)

            AxWinsock1.Listen()
            TCPState.Text = "Listen"


         ElseIf OptTCPMaster.Checked Then

            AxWinsock1.Close()
            AxWinsock1.RemoteHost = TCPHostName.Text
            AxWinsock1.LocalPort = 0
            AxWinsock1.RemotePort = CInt(TCPPort.Text)
            AxWinsock1.Connect()
            TCPState.Text = "Connecting"

         End If

         If COMTypeR.Checked Then
            SerialPort2.Open()
         Else
            OpenVCOM()
         End If

      Catch ex As Exception
         Debug.Print(ex.Message)

      End Try

      CheckBox1.Checked = SerialPort1.IsOpen

      If COMTypeR.Checked Then
         CheckBox2.Checked = SerialPort2.IsOpen
      Else
         CheckBox2.Checked = VCOMOpen
      End If

   End Sub

   Private Sub Close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CloseButton.Click

      If OptSerial.Checked Then

         SerialPort1.Close()

      ElseIf OptTCPSlave.Checked Then

         AxWinsock1.Close()
         TCPState.Text = "Closed"
         TCPOpen = False


      ElseIf OptTCPMaster.Checked Then

         Try
            AxWinsock1.Close()
         Catch ex As Exception
         End Try

         TCPState.Text = "Closed"
         TCPOpen = False

      End If

      CheckBox1.Checked = SerialPort1.IsOpen

      If COMTypeR.Checked Then
         SerialPort2.Close()
         CheckBox2.Checked = SerialPort2.IsOpen
      Else
         CloseVCOM()
         CheckBox2.Checked = VCOMOpen
      End If

   End Sub

   Private Sub AxWinsock1_ConnectEvent(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AxWinsock1.ConnectEvent

      TCPState.Text = "Connected"
      TCPOpen = True

   End Sub

   Private Sub AxWinsock1_ConnectionRequest(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_ConnectionRequestEvent) Handles AxWinsock1.ConnectionRequest

      ' Check if the control's State is closed. If not,
      ' close the connection before accepting the new
      ' connection.
      Try
         AxWinsock1.Close()
         ' Accept the request with the requestID
         ' parameter.
         AxWinsock1.Accept(e.requestID)

      Catch ex As Exception

      End Try
      TCPState.Text = "Connected"
      TCPOpen = True

   End Sub

   Private Sub AxWinsock1_DataArrival(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_DataArrivalEvent) Handles AxWinsock1.DataArrival

      Dim Len As Integer = e.bytesTotal
      Dim Msg(Len) As Byte
      Dim Msgstring As String

      Try
         AxWinsock1.GetData(Msg)

         If COMTypeR.Checked Then
            SerialPort2.Write(Msg, 0, Msg.Length)
         Else
            Msgstring = GetString(Msg)
loop1:
            If Msgstring.Length < 500 Then
               PutMessage(GetString(Msg))
            Else
               ' split it up
               PutMessage(Mid(Msgstring, 1, 500))
               Msgstring = Mid(Msgstring, 500)
               GoTo loop1

            End If

         End If

      Catch ex As Exception
      End Try

   End Sub

   Private Sub AxWinsock1_CloseEvent(ByVal sender As Object, ByVal e As System.EventArgs) Handles AxWinsock1.CloseEvent

      TCPState.Text = "Closed"
      TCPOpen = False

      Try
         AxWinsock1.Close()

         If OptTCPSlave.Checked Then

            AxWinsock1.Listen()
            TCPState.Text = "Listen"

         End If

      Catch ex As Exception

      End Try

   End Sub


   Private Sub LocalCOM_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LocalCOM.SelectedIndexChanged

      If SerialPort2.IsOpen Then SerialPort2.Close()

      SerialPort2.PortName = sender.SelectedItem()
      My.Settings.LocalCOM = sender.SelectedItem()

      CheckBox2.Text = sender.SelectedItem()

      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen


   End Sub

   Private Sub LocalBAUD_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LocallBaud.SelectedIndexChanged

      SerialPort2.BaudRate = sender.SelectedItem()
      My.Settings.LocalBaud = sender.SelectedItem()


   End Sub

   Private Sub ReemoteCOM_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoteCOM.SelectedIndexChanged

      If SerialPort1.IsOpen Then SerialPort1.Close()

      SerialPort1.PortName = sender.SelectedItem()
      CheckBox1.Text = sender.SelectedItem()
      My.Settings.RemoteCOM = sender.SelectedItem()


      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen


   End Sub

   Private Sub RemoteBAUD_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoteBAUD.SelectedIndexChanged

      SerialPort1.BaudRate = sender.SelectedItem()
      My.Settings.RemoteBaud = sender.SelectedItem()

   End Sub
   Dim SerHandle As UInteger


   Private Sub OpenVCOM()

      Dim Access As Long = &HC0000000

      Dim PortNum As String = Mid$(SerialPort2.PortName, 4)

      Dim PortName As String = "\\.\bpq" & PortNum

      SerHandle = CreateFile(PortName, Access And &HFFFFFFFF, _
       0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If SerHandle > 4294967290 Then
         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)
         VCOMOpen = False
         Timer1.Enabled = False
      Else
         VCOMOpen = True
         Timer1.Enabled = True
      End If

   End Sub


   Private Sub CloseVCOM()

      VCOMOpen = False
      CloseHandle(SerHandle)
      Timer1.Enabled = False

   End Sub

   Public Function GetBytes(ByVal strText As String) As Byte()

      ' Converts a text string to a byte array...

      Dim bytBuffer(strText.Length - 1) As Byte

      For intIndex As Integer = 0 To bytBuffer.Length - 1
         bytBuffer(intIndex) = CByte(Asc(strText.Substring(intIndex, 1)))
      Next

      Return bytBuffer

   End Function

   Public Function GetString(ByVal bytBuffer() As Byte) As String

      ' Converts a byte array to a text string...

      Dim sbdInput As New StringBuilder

      For intIndex As Integer = 0 To bytBuffer.Length - 1
         sbdInput.Append(Chr(bytBuffer(intIndex)))
      Next

      Return sbdInput.ToString

   End Function


   Function GetMessage() As Byte()

      Dim BytesReturned As Integer
      Dim Msg As String = Space(1000)

      DeviceIoControl(SerHandle, IOCTL_SERIAL_GETDATA, 0, 0, Msg, Msg.Length, BytesReturned, 0)

      Msg = Mid(Msg, 1, BytesReturned)

      Return GetBytes(Msg)

   End Function


   Sub PutMessage(ByVal Msg As String)

      Dim BytesReturned As Integer

      DeviceIoControl(SerHandle, IOCTL_SERIAL_SETDATA, Msg, Msg.Length, 0, 0, BytesReturned, 0)

   End Sub

   Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick

      Dim Msg() As Byte = GetMessage()

      If Msg.Length > 0 Then

         If OptSerial.Checked Then
            SerialPort1.Write(Msg, 0, Msg.Length)
         Else
            Try
               If TCPOpen Then AxWinsock1.SendData(Msg)
            Catch ex As Exception
            End Try
         End If

      End If


   End Sub

   Private Sub COMTypeR_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles COMTypeR.CheckedChanged

      My.Settings.RealCOM = COMTypeR.Checked

   End Sub

   Private Sub COMTypeV_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles COMTypeV.CheckedChanged

      My.Settings.VirtualCOM = COMTypeV.Checked

   End Sub
End Class