Imports System.Net
Imports System.Threading
Imports System.Text
Imports System
Imports System.IO



Public Structure NodeData
   Public Callsign As String
   Public CallIndex As Integer
   Public downIcon As String
   Public upIcon As String
   Public PopupMode As Integer
   Public Comment As String
   Public Count As Integer
   Public Deleted As Boolean
   Public KillTimer As Integer
   Public Version As String
   Public HeardNodes() As HeardData

End Structure

Public Structure HeardData
   Public Callsign As String
   Public CallIndex As Integer
   Public HeardItems() As HeardItem

End Structure

Public Structure HeardByData
   Public Callsign As String
   Public CallIndex As Integer
   Public Time As DateTime
End Structure
Public Structure CallsignStruct

   Public Callsign As String
   Public Lat As String
   Public Lon As String
   Public Locator As String
   Public LocType As Integer
   Public HeardBy() As HeardByData

End Structure


Public Structure HeardItem
   Public Time As DateTime
   Public Freq As String
   Public Flags As String

End Structure

Public Structure ChatNodeData
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

   Public ChatNodes() As ChatNodeData = New ChatNodeData() {}
   Public ChatNodeIndex As Integer = 0

   Public ChatLinks() As ChatLink = New ChatLink() {}
   Public ChatLinkCount As Integer = 0

   Public CallsignData() As CallSignStruct = New CallSignStruct() {}
   Public CallsignCount As Integer = 0

End Module

Public Class Form1

   ' Location Source Equates

   Public Buff(1000) As Byte
   Private thrServer As Thread
   Dim s As Socket
   Dim NodeChanged As Boolean
   Dim ChatChanged As Boolean
   Dim Exiting As Boolean
   Dim BPQStream As Integer
   Dim NoMoreBoxes As Boolean = False

   Dim ProgramUpdated As Boolean = False
   Dim DataUpdated As Boolean = False
   Dim UseNETROMNODES As Boolean = False

   Dim Node As New Dictionary(Of String, Integer)

   Private Server As Thread

   Public Sub FromLOC(ByVal Locator As String, ByRef Lat As Double, ByRef Lon As Double)

      Dim i As Integer

      Locator = UCase(Locator)

      i = Asc(Mid(Locator, 1, 1))
      Lon = (i - 65) * 20

      i = Asc(Mid(Locator, 3, 1))
      Lon = Lon + (i - 48) * 2

      i = Asc(Mid(Locator, 5, 1))
      Lon = Lon + (i - 65) / 12

      i = Asc(Mid(Locator, 2, 1))
      Lat = (i - 65) * 10

      i = Asc(Mid(Locator, 4, 1))
      Lat = Lat + (i - 48)

      i = Asc(Mid(Locator, 6, 1))
      Lat = Lat + (i - 65) / 24

      If Lon < 0 Or Lon > 360 Then Lon = 180
      If Lat < 0 Or Lat > 180 Then Lat = 90

      Lon = Lon - 180
      Lat = Lat - 90

      Return

   End Sub

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

      End If

      ' Get path to status file (for Icon Create)

      Dim i As Integer = InStr(My.Settings.OutputFileName, ".")

      ReadNodesFile()

      If My.Settings.UseUDP Then

         thrServer = New Thread(AddressOf ReceiveFrom)
         thrServer.Name = "Server"
         thrServer.Start()

      End If


      CheckForIllegalCrossThreadCalls = False
      Server = New Thread(AddressOf Webserver)
      Server.Name = "Server"
      Server.Start()


   End Sub

   Private Sub Form1_FormClosing(ByVal sender As Object, _
       ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

      Exiting = True

      Try
         tcpserver.Stop()
      Catch ex As Exception
      End Try

      Try
         s.Close()
         thrServer.Abort()
      Catch ex As Exception

      End Try
      SaveNodesFile(True)

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

               If Buff(14) = 3 Then  ' UI

                  If UseNETROMNODES AndAlso Buff(15) = 207 AndAlso Buff(16) = 255 Then

                     Nalias = GetString(Buff, 17, 22)
                     Nalias = RTrim(Nalias)
                     Callsign = GetCall(Buff, 7)
                     UpdateChatNode(Callsign, True)
                     ' Debug.Print(Now & " NODES from " & Callsign & " " & Nalias & " Len " & Len.ToString & _
                     '  " " & RemoteEP.ToString)

                     Node(Nalias) = 60

                     For i = 23 To Len - 20 Step 21
                        Callsign = GetCall(Buff, i)
                        UpdateChatNode(Callsign, True)
                        Nalias = GetString(Buff, i + 7, i + 12)
                        Nalias = RTrim(Nalias)
                        Node(Nalias) = 60
                        '            debug.print(Callsign & " " & Nalias)
                     Next

                     Continue While

                  End If

                  ' Not Nodes

                  CallTo = GetCall(Buff, 0)

                  If CallTo = "DUMMY-1" Then

                     Dim Report As String
                     Dim Elements() As String
                     Dim CallFrom As String
                     Dim HeardCall As String
                     Dim Index As Integer
                     Dim Freq As String
                     Dim LOC As String
                     Dim Flags As String

                     ' Node Map Update

                     Report = GetString(Buff, 16, Len - 3)

                     Try

                        CallFrom = GetCall(Buff, 7)

                        If Microsoft.VisualBasic.Left(Report, 3) = "UP " Then

                           ' remote Callsign locator update

                           Elements = Split(Mid(Report, 4), ",")
                           If Elements.Length < 2 Then Continue While

                           Dim CallIndex As Integer = FindCallsign(Elements(0))
                           Dim Lat As Double, Lon As Double

                           With CallsignData(CallIndex)

                              .Locator = Elements(1)

                              FromLOC(.Locator, Lat, Lon)

                              Lat = Lat + Rnd() / 24
                              Lon = Lon + Rnd() / 12

                              .Lat = Lat.ToString
                              .Lon = Lon.ToString
                              .LocType = 5

                           End With

                           SaveNodesFile(False)

                           Continue While

                        End If
                        If Microsoft.VisualBasic.Left(Report, 3) = "MH " Then
                           Try
                              My.Computer.FileSystem.WriteAllText("MonMHLog.txt", _
                                 Now.ToUniversalTime & " " & CallFrom & " " & Report & vbCrLf, True)

                           Catch ex As Exception
                           End Try

                           ' Heard or Connection Report

                           Elements = Split(Mid(Report, 4), ",")

                           If Elements.Length < 4 Then Continue While

                           Index = FindNodeCall(CallFrom)

                           HeardCall = Elements(0)
                           Freq = Elements(1)
                           LOC = Elements(2)
                           Flags = Elements(3)

                           If LOC.Length <> 6 Then LOC = ""

                           UpdateHeardData(Index, HeardCall, Freq, LOC, Flags, Now.ToUniversalTime, True)

                           Continue While

                        End If

                        ' Node Report

                        If Report.Length > 2 Then

                           Elements = Split(Report, " ", 2)

                           If Elements.Length = 2 Then

                              CallFrom = GetCall(Buff, 7)

                              Index = FindNodeCall(CallFrom)

                              UpdateNode(Index, Elements(0), Elements(1))


                           End If
                        End If

                     Catch ex As Exception

                     End Try

                     Continue While

                  End If


                  If CallTo = "DUMMY" Then

                     Dim Report As String
                     Dim Elements() As String
                     Dim n As Integer, ptr As Integer
                     Dim CallFrom As String
                     Dim NewState As Integer
                     Dim index As Integer

                     CallFrom = GetCall(Buff, 7)

                     FindChatCall(CallFrom, True)           ' So will be created
                     UpdateChatNode(CallFrom, True)

                     Report = GetString(Buff, 16, Len - 5)

                     Try

                        ' Try

                        ' My.Computer.FileSystem.WriteAllText("ChatMonLog.txt", _
                        '     Now & " " & CallFrom & " " & Report & " " & RemoteEP.ToString & vbCrLf, True)

                        'Catch ex As Exception

                        ' End Try

                        If Report.Length > 2 Then

                           If Mid(Report, 1, 4) = "INFO" Then

                              Try

                                 Dim latstring As String
                                 Dim lonstring As String
                                 Dim LatLon() As String

                                 index = FindChatCall(CallFrom, True)

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

                                       ChatNodes(index).Lat = DecodeLat(latstring).ToString
                                       ChatNodes(index).Lon = DecodeLon(lonstring).ToString

                                    End If

                                    ' Update Popup text and flag if present

                                    If Elements(1).Length > 0 Then ChatNodes(index).Comment = Elements(1)
                                    If Elements(2).Length > 0 Then ChatNodes(index).PopupMode = CInt(Elements(2))

                                    ChatChanged = True

                                    SaveNodesFile(True)

                                 End If

                              Catch ex As Exception

                                 Debug.Print("Decode INFO Failed - " & ex.Message())

                              End Try

                           Else

                              ' Remove consecutive Spaces

                              i = InStr(Report, "  ")

                              While i > 0

                                 Report = Mid(Report, 1, i) & Mid(Report, i + 2)
                                 i = InStr(Report, "  ")

                              End While

                              Elements = Split(Report, " ")

                              For n = 0 To Elements.Length - 1 Step 2

                                 If Elements(n).Length > 3 Then

                                    NewState = CInt(Elements(n + 1))

                                    ' Ignore down reports if target doesn't exist

                                    If (NewState <> 2) Then

                                       For index = 0 To ChatNodeIndex - 1

                                          If ChatNodes(index).Callsign = Elements(n) Then

                                             If ChatNodes(index).KillTimer > 22 Then
                                                GoTo Ignore ' Ignore
                                             Else
                                                GoTo Ok   ' Node is OK
                                             End If

                                          End If

                                       Next

                                       ' Not found

                                       GoTo Ignore

                                    End If

Ok:
                                    UpdateChatNode(Elements(n), False)

                                    ptr = FindChatPair(CallFrom, Elements(n))

                                    NewState = CInt(Elements(n + 1))

                                    ChatLinks(ptr).KillTimer = 0

                                    If ChatLinks(ptr).Call1 = CallFrom Then
                                       If NewState <> ChatLinks(ptr).Call1State Then ChatChanged = True
                                       ChatLinks(ptr).Call1State = NewState
                                       ChatLinks(ptr).Timeout1 = 21
                                    Else
                                       If NewState <> ChatLinks(ptr).Call2State Then ChatChanged = True
                                       ChatLinks(ptr).Call2State = NewState
                                       ChatLinks(ptr).Timeout2 = 21
                                    End If

                                 End If

Ignore:

                              Next

                           End If ' INFO/CHAT LINKS
                        End If ' Len > 2

                     Catch ex As Exception
                        ' 
                        'Duff DUMMY frame - print it

                        Try

                           My.Computer.FileSystem.WriteAllText("ChatDebugLog.txt", _
                               Now.ToUniversalTime & " " & CallFrom & " " & Report & " " & RemoteEP.ToString & vbCrLf, True)

                        Catch exx As Exception

                        End Try

                     End Try

                  End If ' Call = DUMMY

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

      ChatDefined.Text = ChatNodeIndex.ToString

      For i = 0 To ChatNodeIndex - 1

         If ChatNodes(i).Count > 0 Then
            ChatNodes(i).Count = ChatNodes(i).Count - 1
            If ChatNodes(i).Count = 0 Then
               ChatChanged = True
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
               ChatChanged = True

            End If
         End If

         If ChatLinks(i).Timeout2 > 0 Then
            ChatLinks(i).Timeout2 = ChatLinks(i).Timeout2 - 1

            If ChatLinks(i).Timeout2 = 0 Then

               ChatLinks(i).Call2State = 0
               ChatChanged = True

            End If
         End If
      Next

      ChatActive.Text = j.ToString

      If ChatChanged Then

         Lastupdated.Text = Now.ToUniversalTime.ToString

         Using sw As StreamWriter = New StreamWriter(My.Settings.OutputFileName)

            sw.Write(Now.ToUniversalTime & vbCrLf & "|")

            For i = 0 To ChatNodeIndex - 1

               If ChatNodes(i).KillTimer < 24 Then ' Not heard of for a while - don't display

                  If ChatNodes(i).Count = 0 Then
                     sw.Write(ChatNodes(i).Callsign & "," & ChatNodes(i).Lon & "," & ChatNodes(i).Lat & "," & ChatNodes(i).downIcon & "," & ChatNodes(i).PopupMode & "," & ChatNodes(i).Comment & "," & vbCrLf & "|")
                  Else
                     sw.Write(ChatNodes(i).Callsign & "," & ChatNodes(i).Lon & "," & ChatNodes(i).Lat & "," & ChatNodes(i).upIcon & "," & ChatNodes(i).PopupMode & "," & ChatNodes(i).Comment & "," & vbCrLf & "|")
                  End If

               End If

            Next

            For i = 0 To ChatLinkCount - 1

               j = FindChatCall(ChatLinks(i).Call1, False)

               If j <> -1 Then

                  k = FindChatCall(ChatLinks(i).Call2, False)

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

                        sw.Write("Line," & ChatNodes(j).Lon & "," & ChatNodes(j).Lat & "," & _
                                 ChatNodes(k).Lon & "," & ChatNodes(k).Lat & "," & State.ToString & "," & ChatLinks(i).Call1 & " <> " & ChatLinks(i).Call2 & "," & vbCrLf & "|")
                     End If

                  End If

               End If

            Next

            sw.Close()

         End Using

         If (My.Settings.UserName = "") Then

            ChatChanged = False

         Else
            Dim client As New WebClient()

            client.Credentials = New NetworkCredential(My.Settings.UserName, My.Settings.Password)

            Try
               client.UploadFile(My.Settings.URL, My.Settings.OutputFileName)
               ChatChanged = False
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

      UpdateNodesMap()

   End Sub

   Private Sub UpdateChatNode(ByVal Callsign As String, ByVal UpdateCount As Boolean)

      Dim i As Integer

      For i = 0 To ChatNodeIndex - 1

         ' If UpdateCount is false, then this is coming from a chat link report
         ' Dont set node active, but update killtimer so it will be displayed

         If ChatNodes(i).Callsign = Callsign Then

            If UpdateCount Then

               ChatNodes(i).KillTimer = 0
               If ChatNodes(i).Count = 0 Then
                  ChatChanged = True
               End If
               ChatNodes(i).Count = 60

            End If

            Exit For

         End If
      Next

   End Sub

   Private Sub UpdateNode(ByVal Index As Integer, ByVal Locator As String, ByVal Version As String)

      Dim Lat As Double, Lon As Double

      Nodes(Index).KillTimer = 0

      With CallsignData(Nodes(Index).CallIndex)

         If .Locator <> Locator Then

            .Locator = Locator

            If Locator.Length = 6 Then

               ' Locator

               FromLOC(Locator, Lat, Lon)

               ' Randomize - Square is one 5 minutes longitude * 2,5 minutes latitude

               Lat = Lat + Rnd() / 24
               Lon = Lon + Rnd() / 12

               .LocType = LOC

            Else

               ' Assume Lat/Lon - comma separared

               Dim LatLon() As String = Split(Locator, ":")

               If LatLon.Length < 2 Then Return

               Lat = CDbl(LatLon(0))
               Lon = CDbl(LatLon(1))

               .LocType = USER


            End If

            .Lat = Lat.ToString
            .Lon = Lon.ToString

            NodeChanged = True

         End If

      End With

      With Nodes(Index)

         If .Version <> Version Then

            NodeChanged = True
            .Version = Version

         End If

         If Locator.Length = 6 Then
            .Comment = .Callsign & " " & Locator & " Ver " & Version
         Else
            .Comment = .Callsign & " Ver " & Version
         End If

         If .Count = 0 Then NodeChanged = True

         .Count = 60
      End With

   End Sub

   Private Sub ReadNodesFile()

      Dim returnValue As String
      Dim strLine As String = ""
      Try

         returnValue = File.ReadAllText(My.Settings.FileName)

         ChatNodeIndex = 0

         Dim strLines() As String = returnValue.Split(Chr(10))
         Dim Elements() As String
         Dim i As Integer

         For Each strLine In strLines
            If strLine.Length < 10 Then Exit For
            If strLine.Length > 5000 Then
               Continue For
            End If


            Elements = Split(strLine, ",")

            If Trim(Elements(0)) = "NODE" Then

               Dim Callsign As String = Trim(Elements(1))

               If Callsign.Length = 0 Then
                  Console.WriteLine(Callsign)
                  Continue For
               End If

               For i = 0 To Callsign.Length - 1
                  If Char.IsUpper(Callsign, i) Or Char.IsDigit(Callsign, i) Or Callsign(i) = "-" Then
                  Else
                     Console.WriteLine(Callsign)
                     GoTo Skipit
                  End If
               Next

               Dim Index As Integer = FindNodeCall(Callsign)

               With Nodes(Index)

                  .Version = Trim(Elements(2))
                  .PopupMode = CInt(Elements(3))

                  .upIcon = "greenmarker.png"
                  .downIcon = "redmarker.png"

                  .CallIndex = FindCallsign(Elements(1))

                  If CallsignData(.CallIndex).Locator.Length = 6 Then
                     .Comment = Nodes(Index).Callsign & " " & CallsignData(.CallIndex).Locator & " Ver " & .Version
                  Else
                     .Comment = Nodes(Index).Callsign & " Ver " & .Version
                  End If

                  .KillTimer = 23
                  .Count = 20

               End With

               Continue For

            End If

            If Trim(Elements(0)) = "CALL" Then

               Dim t As Integer = CallsignData.Length
               Dim Callsign As String = Trim(Elements(1))
               Dim xx As Integer
               If Callsign.Length = 0 Then
                  Console.WriteLine(Callsign)
                  Continue For

               End If

               For xx = 0 To Callsign.Length - 1
                  If Char.IsUpper(Callsign, xx) Or Char.IsDigit(Callsign, xx) Or Callsign(xx) = "-" Then
                  Else
                     Console.WriteLine(Callsign)
                     GoTo Skipit
                  End If
               Next

               Dim Index As Integer = FindCallsign(Elements(1))

               With CallsignData(Index)

                  .Locator = Trim(Elements(2))
                  .Lat = Trim(Elements(3))
                  .Lon = Trim(Elements(4))
                  .LocType = CInt(Trim(Elements(5)))

                  '                If .LocType < APRSSSID And .Locator = "" Then

                  ' If we have a locator, but position is less good, update it

                  '               Dim LookupResult As Integer

                  '              LookupResult = LookupCall(.Callsign, Lat, Lon)
                  '             .Lat = Lat.ToString
                  '            .Lon = Lon.ToString
                  '           If Lon <> 0 Then .LocType = LookupResult

                  '          End If

                  '         If .Lat = "0" Then .Lat = Rnd().ToString

               End With

               Continue For

            End If

            If Trim(Elements(0)) = "MH" Then

               Dim Callsign As String = Trim(Elements(1))
               Dim xx As Integer
               If Callsign.Length = 0 Then
                  Console.WriteLine(Callsign)
                  GoTo Skipit
               End If

               For xx = 0 To Callsign.Length - 1
                  If Char.IsUpper(Callsign, xx) Or Char.IsDigit(Callsign, xx) Or Callsign(xx) = "-" Then
                  Else
                     Console.WriteLine(Callsign)
                     GoTo Skipit
                  End If
               Next


               Dim Index As Integer = FindNodeCall(Elements(1))
               Dim Time As DateTime
               Dim Flags As String
               Dim Freq As String

               With Nodes(Index)

                  Dim Count As Integer = CInt(Elements(3)) - 1
                  Dim j As Integer

                  For j = 0 To Count

                     Dim Callsignx As String = Trim(Elements(2))
                     Dim xxx As Integer
                     If Callsignx.Length = 0 Then
                        Console.WriteLine(Callsignx)
                        Continue For
                     End If

                     For xxx = 0 To Callsignx.Length - 1
                        If Char.IsUpper(Callsignx, xxx) Or Char.IsDigit(Callsignx, xxx) Or Callsignx(xxx) = "-" Then
                        Else
                           Console.WriteLine(Callsignx)
                           GoTo skipit2
                        End If
                     Next

                     Try
                        Time = DateTime.FromOADate(CDbl(Elements(5 + (j * 3))))
                        Flags = Elements(6 + j * 3)
                        Freq = Elements(4 + j * 3)
                        UpdateHeardData(Index, Elements(2), Freq, "", Flags, Time, False)
                     Catch ex As Exception
                     End Try
skipit2:
                  Next

               End With

               Continue For

            End If



            If Trim(Elements(0)) <> "XX" Then

               ReDim Preserve ChatNodes(ChatNodeIndex + 1)

               ChatNodes(ChatNodeIndex).Callsign = Trim(Elements(0))
               ChatNodes(ChatNodeIndex).NAlias = Trim(Elements(1))
               ChatNodes(ChatNodeIndex).Lat = Trim(Elements(2))
               ChatNodes(ChatNodeIndex).Lon = Trim(Elements(3))
               ChatNodes(ChatNodeIndex).downIcon = Trim(Elements(4))
               ChatNodes(ChatNodeIndex).upIcon = Trim(Elements(5))
               ChatNodes(ChatNodeIndex).PopupMode = CInt(Elements(6))
               ChatNodes(ChatNodeIndex).Comment = LTrim(Microsoft.VisualBasic.Left(Elements(7), Elements(7).Length - 1))

               If ChatNodes(ChatNodeIndex).upIcon = "" Then ChatNodes(ChatNodeIndex).upIcon = "greenmarker.png"
               If ChatNodes(ChatNodeIndex).downIcon = "" Then ChatNodes(ChatNodeIndex).downIcon = "redmarker.png"

               ChatNodes(ChatNodeIndex).KillTimer = 24

               ChatNodeIndex = ChatNodeIndex + 1

            End If
Skipit:
         Next

      Catch ex As Exception

         MsgBox(ex.Message)
         MsgBox(strline)
         MsgBox("Failed to read Node Definition file " & My.Settings.FileName)

      End Try

      ChatDefined.Text = ChatNodeIndex.ToString
      NodeDefined.Text = NodeIndex.ToString

      NodeChanged = True

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

            For i = 0 To ChatNodeIndex - 1

               If ChatNodes(i).Callsign = Elements(0) Then

                  ChatNodes(i).KillTimer = 23

                  If Elements(3) = ChatNodes(i).upIcon Then

                     ChatNodes(i).Count = 60
                     ChatChanged = True
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

   Function FindBaseCall(ByVal Call1 As String) As Integer

      Dim i As Integer
      Dim Pos As Integer

      Pos = InStr(Call1, "-")
      If Pos > 0 Then Call1 = Mid(Call1, 1, Pos - 1)

      Dim Count As Integer = CallsignData.Length

      For i = 0 To Count - 1

         Dim Call2 As String = CallsignData(i).Callsign

         Pos = InStr(Call2, "-")
         If Pos > 0 Then Call2 = Mid(Call2, 1, Pos - 1)

         If Call2 = Call1 Then
            Return i
         End If


      Next

      Return -1

   End Function

   Function FindNodeCall(ByVal Call1 As String) As Integer

      Dim i As Integer

      For i = 0 To NodeIndex - 1

         If Nodes(i).Callsign = Call1 Then Return i

      Next

      Dim HeardNodes() As HeardData = New HeardData() {}

      ReDim Preserve Nodes(NodeIndex + 1)

      With Nodes(NodeIndex)

         .Callsign = Call1
         .CallIndex = FindCallsign(Call1)
         .upIcon = "greenmarker.png"
         .downIcon = "redmarker.png"
         .Comment = Call1
         .PopupMode = 0
         .Version = ""

         .HeardNodes = HeardNodes

      End With

      NodeIndex = NodeIndex + 1

      NodeChanged = True

      Return NodeIndex - 1

   End Function

   Function FindChatCall(ByVal Call1 As String, ByVal Create As Boolean) As Integer

      Dim i As Integer
      Dim Node As Integer

      For i = 0 To ChatNodeIndex - 1

         If ChatNodes(i).Callsign = Call1 Then Return i

      Next

      If Create = False Then Return -1


      ReDim Preserve ChatNodes(ChatNodeIndex + 1)

      ChatNodes(ChatNodeIndex).Callsign = Call1
      ChatNodes(ChatNodeIndex).NAlias = ""

      '  See if we have a node with same base callsign

      Node = FindBaseCall(Call1)

      If Node > -1 Then
         ChatNodes(ChatNodeIndex).Lat = CallsignData(Node).Lat
         ChatNodes(ChatNodeIndex).Lon = CallsignData(Node).Lon
      Else
         ChatNodes(ChatNodeIndex).Lat = (Rnd() / 24).ToString
         ChatNodes(ChatNodeIndex).Lon = (Rnd() / 12).ToString
      End If

      Dim Callsign As String
      Dim Pos As Integer

      Dim ImageX As Integer
      Dim ImageY As Integer = 25

      Callsign = Call1


      Pos = InStr(Callsign, "-")
      If Pos > 0 Then Callsign = Mid(Callsign, 1, Pos - 1)

      ImageX = Callsign.Length * 14

      Dim image1 As Bitmap = New Bitmap(ImageX, ImageY)
      Dim image2 As Bitmap = New Bitmap(ImageX, ImageY)


      Dim g1 As Graphics = Graphics.FromImage(image1)
      Dim g2 As Graphics = Graphics.FromImage(image2)

      Dim f As Font = New Font("system", 14, FontStyle.Regular)

      g1.FillRectangle(Brushes.LightGreen, 0, 0, ImageX, ImageY - 3)
      g1.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 2), ImageY - 3, 5, 1)
      g1.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 1), ImageY - 2, 3, 1)

      g1.DrawString(Callsign, f, Brushes.Black, 0, 0)

      g2.FillRectangle(Brushes.Red, 0, 0, ImageX, ImageY - 3)
      g2.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 2), ImageY - 3, 5, 1)
      g2.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 1), ImageY - 2, 3, 1)


      g2.DrawString(Callsign, f, Brushes.White, 0, 0)

      image1.Save(Callsign & ".ok.png", System.Drawing.Imaging.ImageFormat.Png)
      image2.Save(Callsign & ".down.png", System.Drawing.Imaging.ImageFormat.Png)

      g1.Dispose()
      g2.Dispose()

      f.Dispose()

      ChatNodes(ChatNodeIndex).upIcon = Callsign & ".ok.png"
      ChatNodes(ChatNodeIndex).downIcon = Callsign & ".down.png"

      ChatNodes(ChatNodeIndex).Comment = Call1
      ChatNodes(ChatNodeIndex).PopupMode = 0

      ChatNodeIndex = ChatNodeIndex + 1

      ChatChanged = True

      Return ChatNodeIndex - 1

   End Function


   Private Sub Timer2_Tick(ByVal sender As Object, ByVal e As System.EventArgs) Handles Timer2.Tick

      Dim i As Integer

      For i = 0 To ChatLinkCount - 1

         ChatLinks(i).KillTimer = ChatLinks(i).KillTimer + 1
         If ChatLinks(i).KillTimer = 24 Then ChatChanged = True

      Next

      For i = 0 To ChatNodeIndex - 1

         ChatNodes(i).KillTimer = ChatNodes(i).KillTimer + 1
         If ChatNodes(i).KillTimer = 24 Then ChatChanged = True

      Next

      For i = 0 To NodeIndex - 1

         Nodes(i).KillTimer = Nodes(i).KillTimer + 1
         If Nodes(i).KillTimer = 24 Then NodeChanged = True

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

   Private Sub UpdateNodesMap()

      Dim i As Integer, j As Integer = 0
      Dim Lat As Single, Lon As Single
      Dim HF As String

      NodeDefined.Text = NodeIndex.ToString

      For i = 0 To NodeIndex - 1

         If Nodes(i).Count > 0 Then
            Nodes(i).Count = Nodes(i).Count - 1
            If Nodes(i).Count = 0 Then
               NodeChanged = True
            Else
               j = j + 1
            End If
         End If

      Next

      NodeActive.Text = j.ToString

      If NodeChanged = False Then
         SaveNodesFile(False)
         Return
      End If

      NodeLastUpdated.Text = Now.ToUniversalTime.ToString

      Using sw As StreamWriter = New StreamWriter(My.Settings.NodesFileName)

         sw.Write(Now.ToUniversalTime & " - " & j.ToString & " Active Nodes" & vbCrLf & "|")

         For i = 0 To NodeIndex - 1

            With Nodes(i)

               If Nodes(i).KillTimer < 24 Then ' Not heard of for a while - don't display

                  Try
                     Lat = CSng(CallsignData(.CallIndex).Lat)
                     Lon = CSng(CallsignData(.CallIndex).Lon)
                  Catch ex As Exception
                     Lat = 0
                     Lon = 0
                  End Try

                  If Lat = 0.0 And Lon = 0.0 Then
                     Lat = Lat + Rnd() / 24
                     Lon = Lon + Rnd() / 12
                  End If

                  If Nodes(i).HeardNodes.Length = 0 Then
                     HF = "0"
                  Else
                     HF = "1"
                  End If

                  If Nodes(i).Count = 0 Then
                     sw.Write(.Callsign & "," & Lon.ToString & "," & Lat.ToString & "," & .downIcon & "," & .PopupMode & "," & .Comment & "," & HF & "," & vbCrLf & "|")
                  Else
                     sw.Write(.Callsign & "," & Lon.ToString & "," & Lat.ToString & "," & .upIcon & "," & .PopupMode & "," & .Comment & "," & HF & "," & vbCrLf & "|")
                  End If

                  Dim HeardCount As Integer = .HeardNodes.Length
                  Dim HeardIndex As Integer
                  Dim LineCount As Integer = 0
                  Dim Age As TimeSpan

                  For HeardIndex = 0 To HeardCount - 1

                     With .HeardNodes(HeardIndex)

                        Dim HTML As String = ""
                        Dim Protocol As Integer = 0
                        For j = 0 To .HeardItems.Length - 1
                           With .HeardItems(j)
                              Age = Now.ToUniversalTime - .Time
                              If Age.TotalDays < 4 Then
                                 HTML = HTML & .Time & " " & .Freq & " " & .Flags & " <br>"
                                 If Mid(.Flags, 1, 1) = "A" Then
                                    Protocol = Protocol Or 1
                                 ElseIf Mid(.Flags, 1, 1) = "H" Then    ' V4
                                    Protocol = Protocol Or 4
                                 ElseIf Mid(.Flags, 1, 1) = "G" Then    ' Tracker/RP
                                    Protocol = Protocol Or 8
                                 Else
                                    Protocol = Protocol Or 2
                                 End If
                                 End If
                           End With
                        Next

                        If Protocol > 4 And Protocol <> 8 Then
                           Protocol = 3         ' V4/RP plus anything - B
                        End If

                        ' Add 'who has heard me' to popup

                        If HTML.Length > 0 Then
                           HTML = HTML & "<br>Heard By<br>"

                           Dim Index As Integer = FindCallsign(.Callsign)

                           With CallsignData(Index)

                              If .HeardBy IsNot Nothing Then

                                 For Index = 0 To .HeardBy.Length - 1
                                    Age = Now.ToUniversalTime - .HeardBy(Index).Time
                                    If Age.TotalDays < 4 Then
                                       HTML = HTML & .HeardBy(Index).Callsign & "<br>"
                                    End If
                                 Next

                              End If

                           End With

                           With CallsignData(.CallIndex)
                              sw.Write("MH," & Nodes(i).Callsign & "," & .Callsign & "," & .Lon & "," & .Lat & "," & Protocol.ToString & ",")
                           End With

                           sw.Write(HTML & vbCrLf & "|")

                        End If
                     End With
                  Next
               End If
            End With
         Next

         sw.Close()

      End Using

      If (My.Settings.UserName = "") Then

         NodeChanged = False
         Return

      End If

      Dim client As New WebClient()

      client.Credentials = New NetworkCredential(My.Settings.UserName, My.Settings.Password)

      Try
         client.UploadFile(My.Settings.NodeURL, My.Settings.NodesFileName)
         NodeChanged = False
      Catch ex As Exception

         If NoMoreBoxes = False Then

            NoMoreBoxes = True
            MsgBox("FTP Failed - " & ex.ToString())
            NoMoreBoxes = False

         End If

      End Try

   End Sub

   Sub UpdateHeardData(ByVal Index As Integer, ByVal HeardCall As String, ByVal Freq As String, ByVal Locator As String, ByVal Flags As String, ByVal Time As DateTime, ByVal TryAPRS As Boolean)

      Dim Lat As Double, Lon As Double
      Dim HeardIndex As Integer, ItemIndex As Integer
      Dim Count As Integer
      Dim FreqModeCount As Integer
      Dim CallIndex As Integer
      Dim HeardBy As Integer

      NodeChanged = True

      If HeardCall.Length > 9 Then Return

      CallIndex = FindCallsign(HeardCall)

      HeardBy = FindCallsign(Nodes(Index).Callsign)

      With CallsignData(CallIndex)

         If .LocType <= LOC And Locator <> "" And Locator <> .Locator Then

            ' If we have a locator, but position is less good, update it

            FromLOC(Locator, Lat, Lon)

            .Locator = Locator
            .Lat = Lat.ToString
            .Lon = Lon.ToString
            .LocType = LOC

         End If

         If TryAPRS Then

            If .LocType < APRSSSID And Locator = "" Then

               ' If we don't have a locator, try to get posn from APRS

               Dim LookupResult As Integer

               LookupResult = LookupCall(HeardCall, Lat, Lon)
               .Lat = Lat.ToString
               .Lon = Lon.ToString
               If Lon <> 0 Then .LocType = LookupResult

            End If
         End If

      End With

      HeardIndex = FindHeardCallEntry(Index, HeardCall, CallIndex)

      With Nodes(Index)

         Count = .HeardNodes.Length

         ' If present, update it (Match Call, Freq abd Mode)

         With .HeardNodes(HeardIndex)

            If .Callsign = HeardCall Then

               If .HeardItems IsNot Nothing Then

                  FreqModeCount = .HeardItems.Length

                  For ItemIndex = 0 To FreqModeCount - 1

                     If .HeardItems(ItemIndex).Freq = Freq And .HeardItems(ItemIndex).Flags = Flags Then

                        ' Move this to end, and move others back

                        Dim i As Integer

                        For i = ItemIndex To FreqModeCount - 2

                           .HeardItems(i) = .HeardItems(i + 1)

                        Next

                        With .HeardItems(FreqModeCount - 1)

                           .Time = Time
                           .Freq = Freq
                           .Flags = Flags

                        End With

                        Return

                     End If

                  Next

               End If
               ' Item Not Present

               ReDim Preserve .HeardItems(FreqModeCount)

               With .HeardItems(FreqModeCount)

                  .Freq = Freq
                  .Flags = Flags
                  .Time = Time

                  UpdateHeardBy(CallIndex, HeardBy, Freq, Flags, Time)


               End With

            End If

         End With

      End With

      UpdateHeardBy(CallIndex, HeardBy, Freq, Flags, Time)


   End Sub

   Sub UpdateHeardBy(ByVal HeardBy As Integer, ByVal Heard As Integer, ByVal Freq As String, ByVal Flags As String, ByVal Time As DateTime)

      Dim HeardIndex As Integer

      HeardIndex = FindHeardByCallEntry(Heard, HeardBy)
      Try
         CallsignData(HeardBy).HeardBy(HeardIndex).Time = Time
      Catch ex As Exception

      End Try

   End Sub

   Function FindHeardByCallEntry(ByVal HeardBy As Integer, ByVal Heard As Integer) As Integer

      Dim Count As Integer, HeardIndex As Integer

      With CallsignData(Heard)

         If .HeardBy Is Nothing Then

            ReDim Preserve .HeardBy(1)

            With .HeardBy(0)

               .Callsign = CallsignData(HeardBy).Callsign
               .CallIndex = HeardBy

            End With

            Return 0

         End If

         Count = .HeardBy.Length

         For HeardIndex = 0 To Count - 2

            With .HeardBy(HeardIndex)

               If .CallIndex = HeardBy Then Return HeardIndex

            End With

         Next

         ' Not Present

         HeardIndex = Count

         ReDim Preserve .HeardBy(HeardIndex)

         With .HeardBy(HeardIndex - 1)

            .Callsign = CallsignData(HeardBy).Callsign
            .CallIndex = HeardBy

         End With

         Return HeardIndex - 1

      End With

   End Function


   Function FindHeardCallEntry(ByVal Index As Integer, ByVal HeardCall As String, ByVal Callindex As Integer) As Integer

      Dim Count As Integer, HeardIndex As Integer

      With Nodes(Index)

         Count = .HeardNodes.Length

         For HeardIndex = 0 To Count - 1

            With .HeardNodes(HeardIndex)

               If .Callsign = HeardCall Then Return HeardIndex

            End With

         Next

         ' Not Present

         HeardIndex = Count

         ReDim Preserve .HeardNodes(HeardIndex)

         With .HeardNodes(HeardIndex)

            .Callsign = HeardCall
            .CallIndex = Callindex

         End With

         Return HeardIndex

      End With

   End Function



   Function LookupCall(ByVal Callsign As String, ByRef Lat As Double, ByRef Lon As Double) As Integer

      Dim client As New WebClient()
      Dim Reply As String = ""
      Dim ptr As Integer
      Dim Posn As String

      LookupCall = 0

      Lat = 0
      Lon = 0

      Try
         Reply = client.DownloadString("http://www.findu.com/cgi-bin/find.cgi?" & Callsign)


         ptr = InStr(Reply, "<br><a href=""http://maps.google.com/maps")
         If ptr > 0 Then

            Posn = Mid(Reply, ptr + 45, 100)
            ptr = InStr(Posn, ",")

            Lat = CDbl(Mid(Posn, 1, ptr - 1))
            Posn = Mid(Posn, ptr + 1)
            ptr = InStr(Posn, "&")
            Lon = CDbl(Mid(Posn, 1, ptr - 1))

            Return APRS

         End If

      Catch ex As Exception
      End Try

      ' Try a wildcard search

      ptr = InStr(Callsign, "-")

      If ptr > 0 Then
         Callsign = Mid(Callsign, 1, ptr - 1)
      End If

      Callsign = Callsign & "*"

      Try
         Reply = client.DownloadString("http://www.findu.com/cgi-bin/find.cgi?" & Callsign)


         ptr = InStr(Reply, "<br><a href=""http://maps.google.com/maps")
         If ptr > 0 Then

            Posn = Mid(Reply, ptr + 45, 100)
            ptr = InStr(Posn, ",")

            Lat = CDbl(Mid(Posn, 1, ptr - 1))
            Posn = Mid(Posn, ptr + 1)
            ptr = InStr(Posn, "&")
            Lon = CDbl(Mid(Posn, 1, ptr - 1))

            Return APRSSSID

         End If

         ptr = InStr(Reply, "<h2>Stations Matching")
         If ptr > 0 Then
            Posn = Mid(Reply, ptr + 45)
            ptr = InStr(Posn, "ALIGN")
            Posn = Mid(Posn, ptr + 4)
            ptr = InStr(Posn, "ALIGN")
            Posn = Mid(Posn, ptr, 100)
            ptr = InStr(Posn, "<td> ")
            Posn = Mid(Posn, ptr + 4, 100)
            ptr = InStr(Posn, "<")
            Lat = CDbl(Mid(Posn, 1, ptr - 1))
            ptr = InStr(Posn, "<td> ")
            Posn = Mid(Posn, ptr + 4, 100)
            ptr = InStr(Posn, "<")
            Lon = CDbl(Mid(Posn, 1, ptr - 1))

            Return APRSSSID

         End If

      Catch ex As Exception
      End Try



   End Function


   Function FindCallsign(ByVal Callsign As String) As Integer

      Dim Count As Integer = CallsignData.Length

      Dim i As Integer

      For i = 0 To Count - 1

         If CallsignData(i).Callsign = Callsign Then Return i

      Next

      ReDim Preserve CallsignData(Count)


      With CallsignData(Count)

         .Callsign = Callsign
         .Lat = ""
         .Lon = ""
         .Locator = ""
         .LocType = 0
      End With


      Return Count

   End Function


   Private Sub EditBPQ32NodesListToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EditBPQ32NodesListToolStripMenuItem.Click
      UpdateNodeDialog.Visible = True
      UpdateNodeDialog.Activate()

   End Sub

   Dim Connections As Integer = 0
   Dim tcpserver As TcpListener

   Private Sub Webserver()

      ' Cache the main page and icon

      Dim returnValue As String = File.ReadAllText("NodeMap.html")
      Dim pageLen As Integer = returnValue.Length

      Dim Page As [Byte]() = System.Text.Encoding.ASCII.GetBytes(returnValue)

      Dim Icon As Byte() = File.ReadAllBytes("favicon.ico")

      Try

         Dim port As Int32 = 81
         Dim localAddr As IPAddress = IPAddress.Parse("0.0.0.0")

         tcpserver = New TcpListener(localAddr, port)

         ' Start listening for client requests.
         tcpserver.Start()

         ' Buffer for reading data

         Dim bytes(8192) As [Byte]
         Dim data As [String] = Nothing

         ' Enter the listening loop.

         While True

            Console.Write("Waiting for a connection... ")
            ServerStatus.Text = "Waiting for a connection"

            ' Perform a blocking call to accept requests.
            ' You could also user server.AcceptSocket() here.

            Dim client As TcpClient = tcpserver.AcceptTcpClient()
            Console.WriteLine("Connected!")

            ServerStatus.Text = "Connected"

            Connections = Connections + 1
            ServerCount.Text = Connections.ToString

            data = Nothing

            ' Get a stream object for reading and writing
            Dim stream As NetworkStream = client.GetStream()

            Dim i As Int32

            Try


               ' Loop to receive all the data sent by the client.
               i = stream.Read(bytes, 0, bytes.Length)
               While (i <> 0)
                  ' Translate data bytes to a ASCII string.
                  data = System.Text.Encoding.ASCII.GetString(bytes, 0, i)

                  Dim params As String() = Split(data, " ", 4)

                  Console.WriteLine(params(1))
                  ServerStatus.Text = params(1)


                  If params(1) = "/" Then

                     ' Send back main page.

                     Dim Header As String = "HTTP/1.1 200 OK\r\nContent-Length: "

                     Header = Header & pageLen.ToString & vbCrLf & "Content-Type: text/html" & vbCrLf & vbCrLf

                     Dim msg As [Byte]() = System.Text.Encoding.ASCII.GetBytes(Header)

                     stream.Write(msg, 0, msg.Length)
                     stream.Write(Page, 0, Page.Length)
                     Exit While

                  End If

                  If params(1) = "/favicon.ico" Then

                     Dim Header As String = "HTTP/1.1 200 OK\r\nContent-Length: "
                     Header = Header & Icon.Length.ToString & vbCrLf & "Content-Type: text/html" & vbCrLf & vbCrLf

                     Dim msg As [Byte]() = System.Text.Encoding.ASCII.GetBytes(Header)

                     stream.Write(msg, 0, msg.Length)
                     stream.Write(Icon, 0, Icon.Length)
                     Exit While

                  Else

                     Dim NodeData As Byte()

                     Dim FN As String = Mid(params(1), 2)

                     If InStr(FN, "/") <> 0 OrElse InStr(FN, "\") <> 0 OrElse InStr(FN, "..") <> 0 Then

                        Dim Errormsg As String = "HTTP/1.1 404 Not Found" & vbCrLf & "Content-Length: 16" & vbCrLf & vbCrLf & "Page not found" & vbCrLf

                        Dim msg1 As [Byte]() = System.Text.Encoding.ASCII.GetBytes(Errormsg)
                        stream.Write(msg1, 0, msg1.Length)

                     End If

                     Try
                        NodeData = File.ReadAllBytes(FN)

                     Catch ex As Exception

                        Dim Errormsg As String = "HTTP/1.1 404 Not Found" & vbCrLf & "Content-Length: 16" & vbCrLf & vbCrLf & "Page not found" & vbCrLf

                        Dim msg1 As [Byte]() = System.Text.Encoding.ASCII.GetBytes(Errormsg)
                        stream.Write(msg1, 0, msg1.Length)

                        Exit While


                     End Try

                     Dim Header As String = "HTTP/1.1 200 OK\r\nContent-Length: "
                     Header = Header & NodeData.Length.ToString & vbCrLf & "Content-Type: text/html" & vbCrLf & vbCrLf

                     Dim msg As [Byte]() = System.Text.Encoding.ASCII.GetBytes(Header)

                     stream.Write(msg, 0, msg.Length)
                     stream.Write(NodeData, 0, NodeData.Length)
                     Exit While

                  End If


               End While

            Catch ex As Exception
               Console.WriteLine("SocketException: {0}", ex)
            End Try


            ' Shutdown and end connection
            client.Close()
         End While
      Catch e1 As SocketException
         Console.WriteLine("SocketException: {0}", e1)
      End Try

      ServerStatus.Text = "Terminated"

   End Sub

   Private Sub Button1_Click_1(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click

   End Sub
End Class