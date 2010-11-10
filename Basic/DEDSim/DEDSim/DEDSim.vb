Imports System
Imports System.Text


Public Class DEDSIM

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

   Declare Function UPDCRC Lib "C:\VBSUPPORT.DLL" (ByVal value As Byte, ByVal CRC As Integer) As Integer

   Private Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)


   Dim SerHandle As UInteger

   Dim Toggle As Integer


   Private Sub DEDSIM_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim Access As Long = &HC0000000

      SerHandle = CreateFile("\\.\bpq3", Access And &HFFFFFFFF, _
       0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If SerHandle > 4294967290 Then

         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)

      End If

   End Sub

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
 
      If Msg.Length > 0 Then ProcessMessage(Msg)

   End Sub

   Private Sub AddStuffingBytes(ByRef bytPendingFrame() As Byte)
      ' Begin byte stuffing at location 2 (skip 170, 170 header)
      ' This insures two sequential bytes of 170 identifies Start of header

      Dim bytNewFrame(1000) As Byte
      Dim intPosition As Integer = 2
      Dim blnAAFound As Boolean
      Dim intIndex As Integer

      For intIndex = 2 To bytPendingFrame.Length - 1
         If bytPendingFrame(intIndex) <> &HAA Then ' No change use exisiting data...
            bytNewFrame(intPosition) = bytPendingFrame(intIndex)
            intPosition += 1
         Else ' Add a 0 byte stuff after the &HAA to insure never two adjacent &HAA bytes... 
            blnAAFound = True
            bytNewFrame(intPosition) = bytPendingFrame(intIndex)
            bytNewFrame(intPosition + 1) = 0
            intPosition += 2
         End If
      Next

      If blnAAFound = False Then
         Return
      Else
         ReDim bytPendingFrame(intPosition - 1)
         bytNewFrame(0) = &HAA
         bytNewFrame(1) = &HAA
         Array.Copy(bytNewFrame, bytPendingFrame, intPosition)
      End If
   End Sub ' AddStuffingBytes

   Sub ProcessMessage(ByVal Msg() As Byte)

      ' The emulator will get full packets, so long as the node sends them
      ' There could be more that one, but it shouldn't really happen

      If Msg(Msg.Length - 1) = 13 Then
         Sleep(1500)
         PutMessage("cmd: " & vbCr)
         Exit Sub
      End If


      If Msg(1) = &H4A Then

         ' JHOST4 Send OK Reponse

         SendOK(0)

         Exit Sub
      End If


      If Msg.Length > 3 Then
         If Msg(0) = 170 And Msg(1) = 170 Then
            Msg = RemoveStuffing(Msg)
            If CheckCRC(Msg, Msg.Length - 1) Then
               ProcessHostMessage(Msg)
            Else
               Debug.Print("crc error")
            End If

         End If
         Exit Sub
      End If

      '     For i = 0 To Msg.Length - 1
      'Debug.Print(Msg(i))
      'N'ex


   End Sub

   Sub ProcessHostMessage(ByVal Msg() As Byte)

      Dim Response(260) As Byte

      Dim Ctl As Integer = Msg(3)
      Dim Channel As Integer = Msg(2)
      Dim Seq As Integer = Ctl And &H80

      If (Seq = Toggle And ((Ctl And &H40) = 0)) Then

         ' Sequence Error

         '        Exit Sub

      End If

      Toggle = Ctl And &H80
      Ctl = Ctl And &H3F

      If (Ctl = 0) Then

         'Data Frame

         If Msg(2) = 253 Then

            If Msg(4) > 2 Then

               TextBox1.AppendText(GetString(Msg, 5, Msg.Length - 3) & vbCrLf)

            End If


            Response(0) = 170
            Response(1) = 170
            Response(2) = 253 ' Channel
            Response(3) = 7      'OK, Null Terminated follows
            Response(4) = 2
            Response(5) = 65
            Response(6) = 66
            Response(7) = &H3B

            Array.Resize(Response, 10)
            SendFrame(Response)
            Return

         End If


         SendOK(Channel)
         Exit Sub

      End If

      '   If Msg(5) = &H47 Then

      ' Poll

      Response(0) = 170
      Response(1) = 170
      Response(2) = Msg(2) ' Channel
      Response(3) = 1      'OK, Null Terminated follows
      Response(4) = 0

      Array.Resize(Response, 7)
      SendFrame(Response)

      Exit Sub

      '     End If

      If Msg(5) = &H4A Then

         ' JHOST Send OK Reponse

         SendOK(Channel)

         Exit Sub

      End If

      If Msg(5) = 170 Then

         ' 170 170 170 ??

         Debug.Print(CStr(Msg(2)))


      End If


   End Sub

   Sub SendOK(ByVal Channel As Integer)

      Dim Response(20) As Byte

      Response(0) = 170
      Response(1) = 170
      Response(2) = CByte(Channel)
      Response(3) = 0      'OK, nothing follows

      Array.Resize(Response, 6)
      SendFrame(Response)

   End Sub

   Function RemoveStuffing(ByVal Msg() As Byte) As Byte()

      Dim Len As Integer = Msg.Length
      Dim i As Integer, j As Integer
      Dim Unstuffed(Len) As Byte

      Unstuffed(0) = Msg(0)
      Unstuffed(1) = Msg(1)
      j = 2

      For i = 2 To Len - 1
         Unstuffed(j) = Msg(i)
         If Msg(i) = &HAA Then i = i + 1
         j = j + 1
      Next

      Array.Resize(Unstuffed, j)

      Return Unstuffed

   End Function

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

   Private Sub AddCRC(ByRef bytPendingFrame() As Byte)
      ' Calculates and sets the CRC values into the last two bytes of the
      ' bytPendingFrame...
      Try
         Dim intCRC As Int32 = &HFFFF
         For intIndex As Integer = 2 To bytPendingFrame.Length - 3
            intCRC = UPDCRC(bytPendingFrame(intIndex), intCRC)
         Next
         intCRC = Not intCRC
         bytPendingFrame(bytPendingFrame.Length - 2) = CByte(intCRC And &HFF)
         bytPendingFrame(bytPendingFrame.Length - 1) = CByte((intCRC And &HFF00) \ 256)

      Catch
         Debug.Print("[SCSHostPort.AddCRC] " & Err.Description)
      End Try
   End Sub ' AddCRC



   Private Function CheckCRC(ByRef bytFrame() As Byte, ByVal intUpperbound As Integer) As Boolean
      ' Checks the CRC on a received host mode data frame...

      Dim intCRC As Integer = &HFFFF
      For intIndex As Integer = 2 To intUpperbound - 2
         intCRC = UPDCRC(bytFrame(intIndex), intCRC)
      Next
      intCRC = Not intCRC
      If bytFrame(intUpperbound - 1) <> CByte(intCRC And &HFF) Then Return False
      If bytFrame(intUpperbound) <> CByte((intCRC And &HFF00) \ 256) Then Return False
      Return True
   End Function ' CheckCRC

   Sub SendFrame(ByVal Msg() As Byte)

      ' Add the alternate frame bit...
      'If Msg(3) >= &H80 Then Toggle = True
      'If Toggle = True Then Msg(3) = Msg(3) Or CByte(&H80)
      'Toggle = Not Toggle
      ' Fill in the CRC bytes...
      AddCRC(Msg)

      ' Add any required stuffing bytes...
      AddStuffingBytes(Msg)

      PutMessage(GetString(Msg))

   End Sub


End Class
