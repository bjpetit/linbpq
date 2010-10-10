Imports System.Threading
Imports System
Imports System.Text


Public Class RigControl

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

   Dim YaesuHandle As UInteger
   Dim ICOMHandle As UInteger
   Dim KenwoodHandle As UInteger

   Dim YaesuModes() As String = {"LSB", "USB", "CW", "CWR", "AM", "", "", "", "FM", "", "DIG", "", "PKT", "FMN", "????"}

   Private Sub RigControl_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim Access As Long = &HC0000000

      YaesuHandle = CreateFile("\\.\bpq62", Access And &HFFFFFFFF, _
       0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If YaesuHandle > 4294967290 Then

         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)

      End If

      ICOMHandle = CreateFile("\\.\bpq61", Access And &HFFFFFFFF, _
        0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If ICOMHandle > 4294967290 Then

         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)

      End If

      KenwoodHandle = CreateFile("\\.\bpq63", Access And &HFFFFFFFF, _
        0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If KenwoodHandle > 4294967290 Then

         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)

      End If

   End Sub


   Dim Msg(100) As Byte
   Dim Len As Integer





   Private Sub Send_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)

      Dim intHertz As Integer = 145500000
      Dim strHertz As String
      Dim intPa1 As Integer, intPa2 As Integer, intPa3 As Integer, intPa4 As Integer, intPa5 As Integer

      strHertz = Format(CInt(145500000), "000000000")
      intPa1 = 16 * CInt(strHertz.Substring(7, 1)) + CInt(strHertz.Substring(8, 1))
      intPa2 = 16 * CInt(strHertz.Substring(5, 1)) + CInt(strHertz.Substring(6, 1))
      intPa3 = 16 * CInt(strHertz.Substring(3, 1)) + CInt(strHertz.Substring(4, 1))
      intPa4 = 16 * CInt(strHertz.Substring(1, 1)) + CInt(strHertz.Substring(2, 1))
      intPa5 = CInt(strHertz.Substring(0, 1))

      Dim bytCommand(10) As Byte
      bytCommand(0) = &HFE
      bytCommand(1) = &HFE
      bytCommand(2) = &H4E
      bytCommand(3) = &HE0
      '   bytCommand(4) = &H3 ' Set frequency command
      '   bytCommand(5) = &HFD
      bytCommand(4) = &H5 ' Set frequency command
      bytCommand(5) = CByte(intPa1)
      bytCommand(6) = CByte(intPa2)
      bytCommand(7) = CByte(intPa3)
      bytCommand(8) = CByte(intPa4)
      bytCommand(9) = CByte(intPa5)
      bytCommand(10) = &HFD
      '    SerialPort1.Write(bytCommand, 0, bytCommand.Length)
      '   SerialPort2.Write(bytCommand, 0, bytCommand.Length)


   End Sub

   Sub ProcessMsg(ByVal Msg() As Byte, ByVal Len As Integer)

   End Sub


   Function GetYaesuMessage() As Byte()

      Dim BytesReturned As Integer
      Dim Msg As String = Space(1000)

      DeviceIoControl(YaesuHandle, IOCTL_SERIAL_GETDATA, 0, 0, Msg, Msg.Length, BytesReturned, 0)

      Msg = Mid(Msg, 1, BytesReturned)

      Return GetBytes(Msg)

   End Function

   Function GetKenwoodMessage() As Byte()

      Dim BytesReturned As Integer
      Dim Msg As String = Space(1000)

      DeviceIoControl(KenwoodHandle, IOCTL_SERIAL_GETDATA, 0, 0, Msg, Msg.Length, BytesReturned, 0)

      Msg = Mid(Msg, 1, BytesReturned)

      Return GetBytes(Msg)

   End Function
   Function GetICOMMessage() As Byte()

      Dim BytesReturned As Integer
      Dim Msg As String = Space(1000)

      DeviceIoControl(ICOMHandle, IOCTL_SERIAL_GETDATA, 0, 0, Msg, Msg.Length, BytesReturned, 0)

      Msg = Mid(Msg, 1, BytesReturned)

      Return GetBytes(Msg)

   End Function


   Sub PutMessage(ByVal Msg As String)

      Dim BytesReturned As Integer

      DeviceIoControl(YaesuHandle, IOCTL_SERIAL_SETDATA, Msg, Msg.Length, 0, 0, BytesReturned, 0)

   End Sub
   Sub PutKenwoodMessage(ByVal Msg As String)

      Dim BytesReturned As Integer

      DeviceIoControl(KenwoodHandle, IOCTL_SERIAL_SETDATA, Msg, Msg.Length, 0, 0, BytesReturned, 0)

   End Sub
   Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick

      Dim Msg() As Byte = GetYaesuMessage()

      If Msg.Length > 0 Then ProcessYaesuMessage(Msg)

      Msg = GetICOMMessage()

      If Msg.Length > 0 Then ProcessICOMMessage(Msg)

      Msg = GetKenwoodMessage()

      If Msg.Length > 0 Then ProcessKenwoodMessage(Msg)

   End Sub

   Dim KenwoodFreq As String = "00123456789"
   Dim KenwoodMode As String = "1"


   Sub ProcessKenwoodMessage(ByVal Msg As Byte())

      Dim Str As String = GetString(Msg)
      Dim Rep As String = "FA00001234567;"

      If Str = "FA;MD;" Then

         PutKenwoodMessage("FA" & KenwoodFreq & ";" & "MD" & KenwoodMode & ";")
         Return

      End If
      '    PutKenwoodMessage("FA00014103000;MD2;")
      ' PutKenwoodMessage("FA00014103000;")

      If Microsoft.VisualBasic.Left(Str, 2) = "FA" Then


         If Str.Length > 11 And Str.Length < 25 Then
            If Mid(Str, 3, 1) <> ";" Then
               KenwoodFreq = Mid(Str, 3, 11)
               KenwoodMode = Mid(Str, 17, 1)
               TextBox3.Text = KenwoodFreq & " " & KenwoodMode
            End If
         End If


         Return

      End If

      If Str = "FB;" Then
         PutKenwoodMessage("FB00001234567;")
         Return
      End If

      If Str = "PT;" Then
         PutKenwoodMessage("PT;")
         Return
      End If


   End Sub

   Dim Freq(5) As Byte
   Dim Mode As Byte



   Sub ProcessYaesuMessage(ByVal Msg As Byte())

      Dim Reply(5) As Byte
      Dim j As Integer, n As Integer, decdigit As Integer

      Dim freqf As Double = 0

      Dim Str As String = GetString(Msg)

      Debug.Print(Str)


      If Msg.Length < 5 Then Return

      If Msg(4) = 7 Then

         Mode = Msg(0)

         Reply(0) = 0

         ReDim Preserve Reply(0)
         SendFrame(Reply)
         Return

      End If

      If Msg(4) = 1 Then

         Freq(0) = Msg(0)
         Freq(1) = Msg(1)
         Freq(2) = Msg(2)
         Freq(3) = Msg(3)

         For j = 0 To 3

            n = Msg(j)
            decdigit = (n >> 4)
            decdigit *= 10
            decdigit += n And 15
            freqf = (freqf * 100) + decdigit

         Next

         freqf = freqf / 100000.0
         TextBox1.Text = freqf.ToString("f4") & "     " & YaesuModes(Mode)

         Reply(0) = 0

         ReDim Preserve Reply(0)
         SendFrame(Reply)
         Return

      End If

      If Msg(4) = 3 Then

         Reply(0) = Freq(0)
         Reply(1) = Freq(1)
         Reply(2) = Freq(2)
         Reply(3) = Freq(3)
         Reply(4) = Mode

         ReDim Preserve Reply(4)
         SendFrame(Reply)
         Return

      End If



      '		Poll = (UCHAR *)&buffptr[2];
      '
      '		*(Poll++) = 0xFE;
      '		*(Poll++) = 0xFE;
      '		*(Poll++) = RIG->RigAddr;
      '		*(Poll++) = 0xE0;
      '		*(Poll++) = 0x5;		// Set frequency command
      '
      '		// Need to convert two chars to bcd digit
      '
      '		*(Poll++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
      '		*(Poll++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
      '		*(Poll++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
      '		*(Poll++)  = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
      '		*(Poll++) = (FreqString[0] - 48);
      '		*(Poll++) = 0xFD;
      '
      '		*(Poll++) = 0xFE;
      '		*(Poll++) = 0xFE;
      '		*(Poll++) = RIG->RigAddr;
      '		*(Poll++) = 0xE0;
      '		*(Poll++) = 0x6;		// Set Mode
      '		*(Poll++) = ModeNo;
      '		*(Poll++) = Filter;
      '		*(Poll++) = 0xFD;
      '
      '		buffptr[1] = 19;



   End Sub

   Dim IFreq(5) As Byte
   Dim IMode As Byte
   Dim ICOMWidth As Byte
   Dim ICOMShift As Byte
   Dim freqf As Double = 0



   Sub ProcessICOMMessage(ByVal Msg As Byte())

      Dim Reply(20) As Byte
      Dim j As Integer, n As Integer, decdigit As Integer

      If Msg.Length > 50 Then Return

      SendICOMFrame(Msg)    ' Echo it

      Reply(0) = &HFE
      Reply(1) = &HFE
      Reply(3) = Msg(2)
      Reply(2) = Msg(3)

      If Msg(4) = 3 Then

         Reply(4) = 3
         Reply(5) = IFreq(0)
         Reply(6) = IFreq(1)
         Reply(7) = IFreq(2)
         Reply(8) = IFreq(3)
         Reply(9) = IFreq(4)
         Reply(10) = &HFD

         ReDim Preserve Reply(10)
         SendICOMFrame(Reply)
         Return


      End If

      If Msg(4) = 4 Then

         Reply(4) = 4
         Reply(5) = 1
         Reply(6) = 1
         Reply(7) = &HFD

         ReDim Preserve Reply(7)
         SendICOMFrame(Reply)
         Return

      End If



      If Msg(4) = 5 Then

         IFreq(0) = Msg(5)
         IFreq(1) = Msg(6)
         IFreq(2) = Msg(7)
         IFreq(3) = Msg(8)
         IFreq(4) = Msg(9)


         Reply(4) = &HFB
         Reply(5) = &HFD

         ReDim Preserve Reply(5)
         SendICOMFrame(Reply)

         freqf = 0

         For j = 4 To 0 Step -1

            n = IFreq(j)
            decdigit = (n >> 4)
            decdigit *= 10
            decdigit += n And 15
            freqf = (freqf * 100) + decdigit

         Next

         freqf = freqf / 1000000.0

         TextBox2.Text = freqf.ToString("f6") & " " & IMode.ToString & " " & ICOMWidth.ToString & " " & ICOMShift.ToString


         Return

      End If

      If Msg(4) = 6 Then

         ICOMWidth = Msg(6)
         IMode = Msg(5)


         Reply(4) = &HFB
         Reply(5) = &HFD

         ReDim Preserve Reply(5)
         SendICOMFrame(Reply)

         TextBox2.Text = freqf.ToString("f6") & " " & IMode.ToString & " " & ICOMWidth.ToString & " " & ICOMShift.ToString
         Return

      End If

      If Msg(4) = 15 Then

         IcomShift = Msg(5)

         Reply(4) = &HFB
         Reply(5) = &HFD

         ReDim Preserve Reply(5)
         SendICOMFrame(Reply)

         TextBox2.Text = freqf.ToString("f6") & " " & IMode.ToString & " " & ICOMWidth.ToString & " " & ICOMShift.ToString

         Return

      End If

      If Msg(4) = 26 Then

         ICOMShift = Msg(5)

         Reply(4) = &HFB
         Reply(5) = &HFD

         ReDim Preserve Reply(5)
         SendICOMFrame(Reply)

         TextBox2.Text = freqf.ToString("f6") & " " & IMode.ToString & " " & ICOMWidth.ToString & " " & ICOMShift.ToString & " Data"

         Return


      End If


      Reply(0) = 0

      ReDim Preserve Reply(0)
      SendFrame(Reply)
      Return



   End Sub

   Public Function GetBytes(ByVal strText As String) As Byte()
      ' Converts a text string to a byte array...

      Dim bytBuffer(strText.Length - 1) As Byte
      For intIndex As Integer = 0 To bytBuffer.Length - 1
         bytBuffer(intIndex) = CByte(Asc(strText.Substring(intIndex, 1)))
      Next
      Return bytBuffer
   End Function ' GetBytes

   Public Function GetString(ByVal bytBuffer() As Byte, _
     Optional ByVal intFirst As Integer = 0, Optional ByVal intLast As Integer = -1) As String
      ' Converts a byte array to a text string...

      If intFirst > bytBuffer.GetUpperBound(0) Then Return ""
      If intLast = -1 Or intLast > bytBuffer.GetUpperBound(0) Then intLast = bytBuffer.GetUpperBound(0)

      Dim sbdInput As New StringBuilder
      For intIndex As Integer = intFirst To intLast
         Dim bytSingle As Byte = bytBuffer(intIndex)
         sbdInput.Append(Chr(bytSingle))
      Next
      Return sbdInput.ToString
   End Function ' GetString

   Sub SendFrame(ByVal Msg() As Byte)

      PutMessage(GetString(Msg))

   End Sub

   Sub SendICOMFrame(ByVal Msg() As Byte)

      PutICOMMessage(GetString(Msg))

   End Sub
   Sub PutICOMMessage(ByVal Msg As String)

      Dim BytesReturned As Integer

      DeviceIoControl(ICOMHandle, IOCTL_SERIAL_SETDATA, Msg, Msg.Length, 0, 0, BytesReturned, 0)

   End Sub

   
End Class