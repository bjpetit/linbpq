Imports System.Net
Imports System.Threading
Imports System.Text
Imports System
Imports System.IO

Public Structure NodeData
   Public Callsign As String
   Public NAlias As String
   Public Lat As String
   Public Lon As String
   Public downIcon As String
   Public upIcon As String
   Public PopupMode As Integer
   Public Comment As String
   Public Count As Integer
   Public Deleted As Boolean
   Public KillTimer As Integer

End Structure

Public Structure ChatLink
   Public Call1 As String
   Public Call2 As String
   Public Call1State As Integer
   Public Call2State As Integer
   Public Timeout1 As Integer
   Public Timeout2 As Integer
   Public KillTimer As Integer

End Structure

Module Globals

   Public Nodes() As NodeData = New NodeData() {}
   Public NodeIndex As Integer = 0

   Public ChatLinks() As ChatLink = New ChatLink() {}
   Public ChatLinkCount As Integer = 0

End Module

Public Class Form1


   Public Buff(1000) As Byte
   Private thrServer As Thread
   Dim s As Socket
   Dim Changed As Boolean
   Dim Exiting As Boolean
   Dim BPQStream As Integer
   Dim NoMoreBoxes As Boolean = False

   Dim ProgramUpdated As Boolean = False
   Dim DataUpdated As Boolean = False

   Public WithEvents AxBPQCtrl1 As AxBPQCTRLLib.AxBPQCtrl

   Dim Node As New Dictionary(Of String, Integer)



   Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      If My.Settings.AutoUpdate Then

         Checkforupdates()
         Timer3.Interval = 86400 * 1000
         Timer3.Enabled = True

      End If

      If My.Settings.UseUDP = False And My.Settings.UseNode = False Then

         ConfigBox.Visible = True

      End If

      If My.Settings.UseNode Then

         Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Form1))

         If IsNothing(AxBPQCtrl1) Then

            AxBPQCtrl1 = New AxBPQCTRLLib.AxBPQCtrl
            CType(AxBPQCtrl1, System.ComponentModel.ISupportInitialize).BeginInit()

            AxBPQCtrl1.Enabled = True
            AxBPQCtrl1.Name = "AxBPQCtrl1"
            AxBPQCtrl1.OcxState = CType(resources.GetObject("AxBPQCtrl1.OcxState"), System.Windows.Forms.AxHost.State)
            AxBPQCtrl1.Visible = False
            Me.Controls.Add(AxBPQCtrl1)
            CType(AxBPQCtrl1, System.ComponentModel.ISupportInitialize).EndInit()

         End If

         AddHandler AxBPQCtrl1.MonDataAvail, AddressOf BPQ32MonDataAvailable

         BPQStream = AxBPQCtrl1.FindFreeStream
         AxBPQCtrl1.SetFlags(BPQStream, 0, 128)

      End If

      ReadNodesFile()

      If My.Settings.UseUDP Then

         thrServer = New Thread(AddressOf ReceiveFrom)
         thrServer.Name = "Server"
         thrServer.Start()

      End If

   End Sub

   Private Sub Form1_FormClosing(ByVal sender As Object, _
       ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

      Exiting = True

      Try
         AxBPQCtrl1.SetFlags(BPQStream, 0, 128)
      Catch ex As Exception
      End Try
      Try
         s.Close()
         thrServer.Abort()
      Catch ex As Exception

      End Try

   End Sub

   Public Sub BPQ32MonDataAvailable(ByVal sender As Object, ByVal e As AxBPQCTRLLib._DBPQCtrlEvents_MonDataAvailEvent)

      Dim intStream As Integer
      Dim strBPQData As String
      Dim i As Integer
      Dim Callsign As String
      Dim Nalias As String

      intStream = e.stream

      strBPQData = AxBPQCtrl1.GetMonFrame(intStream)

      While strBPQData.Length > 0

         strBPQData = Mid(strBPQData, 8)     ' Remove Header

         Dim Buff() As Byte = GetBytes(strBPQData)

         Dim len As Integer = strBPQData.Length

         If Buff(14) = 3 And Buff(15) = 207 And Buff(16) = 255 Then

            Nalias = GetString(Buff, 17, 22)
            Callsign = GetCall(Buff, 7)
            '           SerHandle" NODES from " & Callsign & " " & Nalias & " Len " & len.ToString)
            UpdateNode(Callsign, True)
            Node(Nalias) = 60

            For i = 23 To len - 20 Step 21
               Callsign = GetCall(Buff, i)
               UpdateNode(Callsign, True)
               Nalias = GetString(Buff, i + 7, i + 12)
               Nalias = RTrim(Nalias)
               Node(Nalias) = 60
               '          SerHandleCallsign & " " & Nalias)
            Next


         End If

         strBPQData = AxBPQCtrl1.GetMonFrame(intStream)

      End While


   End Sub


   Sub ReceiveFrom()


      Dim lep As New IPEndPoint(0, My.Settings.Port)
      Dim send As New IPEndPoint(0, My.Settings.Port)
      Dim RemoteEP As EndPoint = CType(send, EndPoint)
      Dim Len As Integer
      Dim i As Integer
      Dim Callsign As String
      Dim CallTo As String
      Dim Nalias As String

      s = New Socket(lep.Address.AddressFamily, SocketType.Dgram, ProtocolType.Udp)

      Try
         s.Bind(lep)

         While True

            Len = s.ReceiveFrom(Buff, RemoteEP)

            Try

               If Buff(14) = 3 Then

                  If Buff(15) = 207 And Buff(16) = 255 Then

                     Nalias = GetString(Buff, 17, 22)
                     Nalias = RTrim(Nalias)
                     Callsign = GetCall(Buff, 7)
                     UpdateNode(Callsign, True)
                     Debug.Print(Now & " NODES from " & Callsign & " " & Nalias & " Len " & Len.ToString & _
                       " " & RemoteEP.ToString)

                     Node(Nalias) = 60

                     For i = 23 To Len - 20 Step 21
                        Callsign = GetCall(Buff, i)
                        UpdateNode(Callsign, True)
                        Nalias = GetString(Buff, i + 7, i + 12)
                        Nalias = RTrim(Nalias)
                        Node(Nalias) = 60
                        '            debug.print(Callsign & " " & Nalias)
                     Next

                  Else ' Not Nodes

                     CallTo = GetCall(Buff, 0)

                     If CallTo = "DUMMY" Then

                        Dim Report As String
                        Dim Elements() As String
                        Dim n As Integer, ptr As Integer
                        Dim CallFrom As String
                        Dim NewState As Integer

                        CallFrom = GetCall(Buff, 7)

                        UpdateNode(CallFrom, True)

                        Report = GetString(Buff, 16, Len - 5)

                        Try

                           ' Try

                           ' My.Computer.FileSystem.WriteAllText("ChatMonLog.txt", _
                           '     Now & " " & CallFrom & " " & Report & " " & RemoteEP.ToString & vbCrLf, True)

                           'Catch ex As Exception

                           ' End Try

                           If Report.Length > 2 Then

                              Elements = Split(Report, " ")

                              If Elements(0) = "INFO" Then

                                 Try

                                    Dim latstring As String
                                    Dim lonstring As String
                                    Dim LatLon() As String
                                    Dim index As Integer = FindCall(CallFrom)

                                    Report = Mid(Report, 6)
                                    Elements = Split(Report, "|")

                                    If Elements.Length = 3 Then

                                       If Elements(0).Length > 0 Then

                                          ' Update Lat/Lon

                                          If InStr(Elements(0), ",") > 0 Then

                                             ' Comma Separated - easy!

                                             LatLon = Split(LTrim(Elements(0)), ",")
                                             latstring = LatLon(0)
                                             lonstring = LatLon(1)

                                          Else

                                             LatLon = Split(LTrim(Elements(0)), " ")

                                             ' May have spaces round N/S/E/W - if so get 4 elements

                                             If LatLon.Length = 4 Then
                                                latstring = LatLon(0) & " " & LatLon(1)
                                                lonstring = LatLon(2) & " " & LatLon(3)
                                             Else
                                                latstring = LatLon(0)
                                                lonstring = LatLon(1)
                                             End If

                                          End If

                                          Nodes(index).Lat = DecodeLat(latstring).ToString
                                          Nodes(index).Lon = DecodeLon(lonstring).ToString

                                       End If

                                       ' Update Popup text and flag if present

                                       If Elements(1).Length > 0 Then Nodes(index).Comment = Elements(1)
                                       If Elements(2).Length > 0 Then Nodes(index).PopupMode = CInt(Elements(2))

                                       Changed = True

                                       SaveNodesFile()

                                    End If

                                 Catch ex As Exception

                                    Debug.Print("Decode INFO Failed - " & ex.Message())

                                 End Try

                              Else

                                 For n = 0 To Elements.Length - 1 Step 2

                                    If Elements(n).Length > 3 Then

                                       UpdateNode(Elements(n), False)

                                       ptr = FindChatPair(CallFrom, Elements(n))

                                       NewState = CInt(Elements(n + 1))

                                       ChatLinks(ptr).KillTimer = 0

                                       If ChatLinks(ptr).Call1 = CallFrom Then
                                          If NewState <> ChatLinks(ptr).Call1State Then Changed = True
                                          ChatLinks(ptr).Call1State = NewState
                                          ChatLinks(ptr).Timeout1 = 21
                                       Else
                                          If NewState <> ChatLinks(ptr).Call2State Then Changed = True
                                          ChatLinks(ptr).Call2State = NewState
                                          ChatLinks(ptr).Timeout2 = 21
                                       End If

                                    End If

                                 Next

                              End If ' INFO/CHAT LINKS
                           End If ' Len > 2

                        Catch ex As Exception
                           ' 
                           'Duff DUMMY frame - print it

                           Try

                              My.Computer.FileSystem.WriteAllText("ChatDebugLog.txt", _
                                  Now & " " & CallFrom & " " & Report & " " & RemoteEP.ToString & vbCrLf, True)

                           Catch exx As Exception

                           End Try

                        End Try
                     End If ' Call = DUMMY
                  End If ' NODES 
               End If  ' Not UI

            Catch ex As Exception

            End Try


         End While

      Catch ex As Exception

         If Exiting = False Then MsgBox("UDP Listen Failed - " & ex.Message())

      End Try

   End Sub 'ReceiveFrom

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
         If bytSingle <> 0 Then
            sbdInput.Append(Chr(bytSingle))
         Else
            'sbdInput.Append("~")
            Exit For
         End If
      Next
      Return sbdInput.ToString
   End Function ' GetString
   Public Function GetCall(ByVal bytBuffer() As Byte, ByVal start As Integer) As String

      ' Converts ax25 byte array to a text string...

      Dim sbdInput As New StringBuilder
      Dim SSID As Integer
      Dim intIndex As Integer

      For intIndex = start To start + 5
         Dim bytSingle As Byte = bytBuffer(intIndex) >> 1
         If bytSingle = 32 Then Exit For
         sbdInput.Append(Chr(bytSingle))
      Next

      SSID = bytBuffer(start + 6)
      SSID = (SSID And 30) >> 1

      If SSID > 0 Then
         sbdInput.Append("-")
         sbdInput.Append(SSID.ToString)

      End If

      Return sbdInput.ToString

   End Function ' GetString


   Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ReloadButton.Click

      'For Each kvp As KeyValuePair(Of String, Integer) In Node
      'debug.print("Key = {0}, Value = {1}", kvp.Key, kvp.Value.ToString)
      'Next kvp


      ReadNodesFile()

   End Sub

   Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick

      Dim i As Integer, j As Integer = 0, k As Integer
      Dim State As Integer

      For i = 0 To NodeIndex - 1

         If Nodes(i).Count > 0 Then
            Nodes(i).Count = Nodes(i).Count - 1
            If Nodes(i).Count = 0 Then
               Changed = True
            Else
               j = j + 1
            End If
         End If

      Next

      For i = 0 To ChatLinkCount - 1

         If ChatLinks(i).Timeout1 > 0 Then
            ChatLinks(i).Timeout1 = ChatLinks(i).Timeout1 - 1

            If ChatLinks(i).Timeout1 = 0 Then

               ChatLinks(i).Call1State = 0
               Changed = True

            End If
         End If

         If ChatLinks(i).Timeout2 > 0 Then
            ChatLinks(i).Timeout2 = ChatLinks(i).Timeout2 - 1

            If ChatLinks(i).Timeout2 = 0 Then

               ChatLinks(i).Call2State = 0
               Changed = True

            End If
         End If
      Next

      Active.Text = j.ToString

      If Changed Then

         Lastupdated.Text = Now.ToString

         Using sw As StreamWriter = New StreamWriter(My.Settings.OutputFileName)

            sw.Write(Now & vbCrLf & "|")

            For i = 0 To NodeIndex - 1

               If Nodes(i).KillTimer < 24 Then ' Not heard of for a while - don't display

                  If Nodes(i).Count = 0 Then
                     sw.Write(Nodes(i).Callsign & "," & Nodes(i).Lon & "," & Nodes(i).Lat & "," & Nodes(i).downIcon & "," & Nodes(i).PopupMode & "," & Nodes(i).Comment & "," & vbCrLf & "|")
                  Else
                     sw.Write(Nodes(i).Callsign & "," & Nodes(i).Lon & "," & Nodes(i).Lat & "," & Nodes(i).upIcon & "," & Nodes(i).PopupMode & "," & Nodes(i).Comment & "," & vbCrLf & "|")
                  End If

               End If

            Next

            For i = 0 To ChatLinkCount - 1

               j = FindCall(ChatLinks(i).Call1)

               If j <> -1 Then

                  k = FindCall(ChatLinks(i).Call2)

                  If k <> -1 Then

                     ' We hae both Positions - write a Line: line
                     ' |Line,-122.1225,47.6891666666667, -93.294332,36.620264,#000000,2

                     If (ChatLinks(i).KillTimer < 24) Then ' Not heard of for a while - remove

                        If ChatLinks(i).Call1State = 0 And ChatLinks(i).Call2State = -1 Then
                           State = 3      ' Mismatch
                        ElseIf ChatLinks(i).Call1State = 2 And ChatLinks(i).Call2State = 2 Then
                           State = 2
                        ElseIf ChatLinks(i).Call1State = 2 Or ChatLinks(i).Call2State = 2 Then
                           State = 1
                        Else
                           State = 0
                        End If

                        sw.Write("Line," & Nodes(j).Lon & "," & Nodes(j).Lat & "," & _
                                 Nodes(k).Lon & "," & Nodes(k).Lat & "," & State.ToString & "," & ChatLinks(i).Call1 & " <> " & ChatLinks(i).Call2 & "," & vbCrLf & "|")
                     End If

                  End If

               End If

            Next

            sw.Close()

         End Using

         If (My.Settings.UserName = "") Then

            Changed = False

         Else
            Dim client As New WebClient()

            client.Credentials = New NetworkCredential(My.Settings.UserName, My.Settings.Password)

            Try
               client.UploadFile(My.Settings.URL, My.Settings.OutputFileName)
               Changed = False
               '          My.Computer.FileSystem.WriteAllText("ChatMonLog.txt", Now & " FTP Transfer Complete" & vbCrLf, True)

            Catch ex As Exception

               If NoMoreBoxes = False Then

                  NoMoreBoxes = True
                  MsgBox("FTP Failed - " & ex.ToString())
                  NoMoreBoxes = False

               End If

            End Try

         End If

      End If

   End Sub

   Private Sub UpdateNode(ByVal Callsign As String, ByVal UpdateCount As Boolean)

      Dim i As Integer

      For i = 0 To NodeIndex - 1

         ' If UpdateCount is false, then this is coming from a chat link report
         ' Dont set node active, but update killtimer so it will be displayed

         If Nodes(i).Callsign = Callsign Then

            Nodes(i).KillTimer = 0

            If UpdateCount Then

               If Nodes(i).Count = 0 Then
                  Changed = True
               End If
               Nodes(i).Count = 60

            End If

         End If
      Next

   End Sub


   Private Sub ReadNodesFile()

      Dim returnValue As String

      Try

         returnValue = File.ReadAllText(My.Settings.FileName)

         NodeIndex = 0

         Dim strLines() As String = returnValue.Split(Chr(10))
         Dim Elements() As String
         For Each strLine As String In strLines

            If strLine.Length < 10 Then Exit For

            Elements = Split(strLine, ",")

            If Trim(Elements(0)) <> "XX" Then

               ReDim Preserve Nodes(NodeIndex + 1)

               Nodes(NodeIndex).Callsign = Trim(Elements(0))
               Nodes(NodeIndex).NAlias = Trim(Elements(1))
               Nodes(NodeIndex).Lat = Trim(Elements(2))
               Nodes(NodeIndex).Lon = Trim(Elements(3))
               Nodes(NodeIndex).downIcon = Trim(Elements(4))
               Nodes(NodeIndex).upIcon = Trim(Elements(5))
               Nodes(NodeIndex).PopupMode = CInt(Elements(6))
               Nodes(NodeIndex).Comment = LTrim(Microsoft.VisualBasic.Left(Elements(7), Elements(7).Length - 1))

               If Nodes(NodeIndex).upIcon = "" Then Nodes(NodeIndex).upIcon = "greenmarker.png"
               If Nodes(NodeIndex).downIcon = "" Then Nodes(NodeIndex).downIcon = "redmarker.png"

               Nodes(NodeIndex).KillTimer = 24

               NodeIndex = NodeIndex + 1

            End If

         Next

      Catch ex As Exception

         MsgBox("Failed to read Node Definition file " & My.Settings.FileName)

      End Try

      Defined.Text = NodeIndex.ToString

      RestoreLastState()

   End Sub

   Sub RestoreLastState()

      Dim returnValue As String
      Dim i As Integer

      Try

         returnValue = File.ReadAllText(My.Settings.OutputFileName)

         Dim strLines() As String = returnValue.Split(New [Char]() {"|"c})

         Dim Elements() As String

         For Each strLine As String In strLines

            If strLine.Length < 10 Then Exit For

            Elements = Split(strLine, ",")

            ' Find Call

            For i = 0 To NodeIndex - 1

               If Nodes(i).Callsign = Elements(0) Then

                  Nodes(i).KillTimer = 23

                  If Elements(3) = Nodes(i).upIcon Then

                     Nodes(i).Count = 60
                     Changed = True
                     Exit For

                  End If

               End If

            Next

         Next

      Catch ex As Exception

      End Try


   End Sub

   Private Sub ConfigMonitorToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ConfigMonitorToolStripMenuItem.Click

      ConfigBox.Visible = True

   End Sub

   Private Sub EditNodesListToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EditNodesListToolStripMenuItem.Click

      AddNode.Visible = True
      AddNode.Activate()

   End Sub

   Function FindChatPair(ByVal Call1 As String, ByVal Call2 As String) As Integer

      Dim i As Integer

      For i = 0 To ChatLinkCount - 1

         If ((ChatLinks(i).Call1 = Call1 And ChatLinks(i).Call2 = Call2) Or _
            (ChatLinks(i).Call1 = Call2 And ChatLinks(i).Call2 = Call1)) Then

            Return i

         End If

      Next

      ReDim Preserve ChatLinks(ChatLinkCount + 1)

      ChatLinks(ChatLinkCount).Call1 = Call1
      ChatLinks(ChatLinkCount).Call2 = Call2

      ChatLinks(ChatLinkCount).Call1State = -1
      ChatLinks(ChatLinkCount).Call2State = -1

      ChatLinkCount = ChatLinkCount + 1

      Return ChatLinkCount - 1


   End Function

   Function FindCall(ByVal Call1 As String) As Integer

      Dim i As Integer

      For i = 0 To NodeIndex - 1

         If Nodes(i).Callsign = Call1 Then Return i

      Next

      ReDim Preserve Nodes(NodeIndex + 1)

      Nodes(NodeIndex).Callsign = Call1
      Nodes(NodeIndex).NAlias = ""
      Nodes(NodeIndex).Lat = "0"
      Nodes(NodeIndex).Lon = "0"
      Nodes(NodeIndex).upIcon = "greenmarker.png"
      Nodes(NodeIndex).downIcon = "redmarker.png"

      Nodes(NodeIndex).Comment = Call1
      Nodes(NodeIndex).PopupMode = 0

      NodeIndex = NodeIndex + 1

      Defined.Text = NodeIndex.ToString

      Changed = True

      Return NodeIndex - 1

   End Function

   Private Sub Timer2_Tick(ByVal sender As Object, ByVal e As System.EventArgs) Handles Timer2.Tick

      Dim i As Integer

      For i = 0 To ChatLinkCount - 1

         ChatLinks(i).KillTimer = ChatLinks(i).KillTimer + 1
         If ChatLinks(i).KillTimer = 24 Then Changed = True

      Next

      For i = 0 To NodeIndex - 1

         Nodes(i).KillTimer = Nodes(i).KillTimer + 1
         If Nodes(i).KillTimer = 24 Then Changed = True

      Next

   End Sub

   Sub Checkforupdates()

      Dim client As New WebClient()
      Dim Msg As String = Nothing
      Dim DataTime As Date
      Dim ProgTime As Date
      Dim NewDataTime As Date
      Dim NewProgTime As Date

      '
      '  See if new config or program files to load.
      '
      '  Status Message is timestamp from files.

      Try

         DataTime = File.GetLastWriteTime(My.Settings.FileName)

         If DataTime = #1/1/1601# Then Return

         ProgTime = File.GetLastWriteTime(Application.ExecutablePath)

      Catch ex As Exception

      End Try
 
      Try

         Msg = client.DownloadString("http://www.cantab.net/users/john.wiseman/Icons/FileTimeStamps.txt")

      Catch ex As Exception

         Return

      End Try

      If IsNothing(Msg) Then Return

      If Msg.Length < 40 Or Msg.Length > 50 Then Return

      Dim Lines() As String = Msg.Split(Chr(10))

      If Lines.Length < 2 Then Return

      Try

         NewDataTime = CDate(Lines(0))
         NewProgTime = CDate(Lines(1))

      Catch ex As Exception
         Return
      End Try

       If NewDataTime > DataTime Then

         Dim FN As String
         Dim NewData As String

         Lines = My.Settings.FileName.Split("\"c)
         FN = Lines(Lines.Length - 1)

         Try

            NewData = client.DownloadString("http://www.cantab.net/users/john.wiseman/Icons/" & FN)

            Try
               Dim NewName As String = FN & "." & DataTime.ToString
               Mid(NewName, InStr(NewName, "/"), 1) = "-"
               Mid(NewName, InStr(NewName, "/"), 1) = "-"
               Mid(NewName, InStr(NewName, ":"), 1) = "-"
               Mid(NewName, InStr(NewName, ":"), 1) = "-"


               My.Computer.FileSystem.RenameFile(My.Settings.FileName, NewName)
               My.Computer.FileSystem.WriteAllText(My.Settings.FileName, NewData, False)

               MsgBox("Data Updated")

               DataUpdated = True

            Catch ex As Exception

            End Try

         Catch ex As Exception
            MsgBox(ex.ToString)
         End Try

      End If
      If NewProgTime > ProgTime Then

         Dim FN As String
         Dim NewProg() As Byte

         Lines = Application.ExecutablePath.Split("\"c)
         FN = Lines(Lines.Length - 1)

         Try

            NewProg = client.DownloadData("http://www.cantab.net/users/john.wiseman/Icons/" & FN)

            If NewProg.Length > 50000 Then

               Try
                  Dim NewName As String = FN & "." & DataTime.ToString
                  Mid(NewName, InStr(NewName, "/"), 1) = "-"
                  Mid(NewName, InStr(NewName, "/"), 1) = "-"
                  Mid(NewName, InStr(NewName, ":"), 1) = "-"
                  Mid(NewName, InStr(NewName, ":"), 1) = "-"

                  My.Computer.FileSystem.RenameFile(Application.ExecutablePath, NewName)
                  My.Computer.FileSystem.WriteAllBytes(Application.ExecutablePath, NewProg, False)

                  MsgBox("Program Updated")

                  Process.Start(Application.ExecutablePath)

                  End

               Catch ex As Exception

               End Try

            End If


         Catch ex As Exception
            MsgBox(ex.ToString)
         End Try

      End If



   End Sub

   Private Sub Timer3_Tick(ByVal sender As Object, ByVal e As System.EventArgs) Handles Timer3.Tick

      Checkforupdates()

      If DataUpdated Then
         ReadNodesFile()
         DataUpdated = False
      End If

   End Sub
End Class
