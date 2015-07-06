Namespace My

    ' The following events are available for MyApplication:
    ' 
    ' Startup: Raised when the application starts, before the startup form is created.
    ' Shutdown: Raised after all application forms are closed.  This event is not raised if the application terminates abnormally.
    ' UnhandledException: Raised if the application encounters an unhandled exception.
    ' StartupNextInstance: Raised when launching a single-instance application and the application is already active. 
    ' NetworkAvailabilityChanged: Raised when the network connection is connected or disconnected.
    Partial Friend Class MyApplication
        Private Sub MyUnhandledException(ByVal s As Object, _
               ByVal e As ApplicationServices.UnhandledExceptionEventArgs) Handles Me.UnhandledException
            Dim strUnhandledException As String = TimestampEx() & " [" & strProductVersion & "] " & _
               s.ToString & ": " & vbCrLf & e.Exception.Message & vbCrLf & e.Exception.StackTrace & _
               vbCrLf & e.Exception.TargetSite.ToString & vbCrLf & vbCrLf
            My.Computer.FileSystem.WriteAllText(strExecutionDirectory & "Logs\ARDOP_WinTNC_Unhandled Exceptions.log", strUnhandledException, True)
        End Sub
    End Class


End Namespace

