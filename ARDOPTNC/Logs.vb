#Region "Imports"
Imports System.IO
#End Region ' Imports

Public Class Logs  ' A class for writing text to debug and exception logs
#Region "Objects"
    Private Shared objLogLock As New Object
#End Region ' Objects

#Region "Public Shared Subs"

    ' Subroutine to write the indicated text to the exception log...
    Public Shared Sub Exception(ByVal strText As String)
        Dim dttTimestamp As Date = DateTime.UtcNow
        ' Debug.WriteLine("ARDOP EXCEPTION Log: " & strText)
        SyncLock objLogLock
            Try  ' insures Exception log can't throw an Unhandled exception
                My.Computer.FileSystem.WriteAllText(strExecutionDirectory & "Logs\ARDOP_WinTNC_Exceptions_" & _
                    Format(dttTimestamp, "yyyyMMdd") & ".log", Format(dttTimestamp, "yyyy/MM/dd HH:mm:ss") & " [" & Application.ProductVersion & "] " & strText & vbCrLf, True)
            Catch
            End Try
        End SyncLock
    End Sub  'Exception

    ' Subroutine to write the indicated text to the debug log...
    Public Shared Sub WriteDebug(ByVal strText As String)
        SyncLock objLogLock
            Try  ' insures Debug log can't throw an unhandled exception
                Dim dttTimestamp As Date = DateTime.UtcNow
                Dim strTS As String = Format(dttTimestamp, "yyyy/MM/dd HH:mm:ss") & "." & Format(dttTimestamp.Millisecond, "000")
                My.Computer.FileSystem.WriteAllText(strExecutionDirectory & "Logs\ARDOP_WinTNC_Debug_" & _
                   Format(dttTimestamp, "yyyyMMdd") & ".log", strTS & " [" & Application.ProductVersion & "] " & strText & vbCrLf, True)
            Catch
            End Try
        End SyncLock
    End Sub  ' WriteDebug

#End Region ' Public Shared Subs

End Class
