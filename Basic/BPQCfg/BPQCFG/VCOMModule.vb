Imports System
Imports System.Text

Module Module2

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

   Declare Function CloseHandle Lib "kernel32" (ByVal hObject As Long) As Long

   Declare Function WriteFile Lib "kernel32" _
      (ByVal hFile As Long, ByVal lpBuffer As String, _
      ByVal nNumberOfBytesToWrite As Long, _
      ByVal lpNumberOfBytesWritten As Long, _
      ByVal lpOverlapped As Long) As Long


   Declare Function CreateFile Lib "kernel32" Alias "CreateFileA" _
      (ByVal lpFileName As String, _
      ByVal dwDesiredAccess As UInteger, _
      ByVal dwShareMode As UInteger, _
      ByVal lpSecurityAttributes As UInteger, _
      ByVal dwCreationDisposition As UInteger, _
      ByVal dwFlagsAndAttributes As UInteger, _
      ByVal hTemplateFile As UInteger) As UInteger

   Declare Function DeviceIoControl Lib "kernel32" (ByVal hDevice As UInteger, ByVal dwIoControlCode As UInteger, _
      ByVal lpInBuffer As String, ByVal nInBufferSize As Integer, ByVal lpOutBuffer As String, _
      ByVal nOutBufferSize As Integer, ByRef lpBytesReturned As Integer, ByVal lpOverlapped As UInteger) As UInteger

   Public VCOMHandle As UInteger

   Public Sub OpenVCOMControlChannel()

      Dim Access As Long = &HC0000000

      VCOMHandle = CreateFile("\\.\BPQControl", Access And &HFFFFFFFF, _
       0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)

      If VCOMHandle > 4294967290 Then

         Dim err As New ComponentModel.Win32Exception
         MsgBox(err.Message)

      End If

   End Sub
End Module
