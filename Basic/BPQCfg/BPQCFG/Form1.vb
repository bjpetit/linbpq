
Imports System.IO
Imports Microsoft.VisualBasic.FileIO
Imports System
Imports System.ServiceProcess
Imports System.Diagnostics
Imports System.Threading
Imports System.Environment


Public Class Form1

   Dim WithEvents SaveFileDialog1 As New SaveFileDialog()
   Dim WithEvents OpenFileDialog1 As New OpenFileDialog()

   Dim LineNo As Integer
   Dim Key As String, Value As String

   Dim mainMenu1 As New MainMenu()
   Dim topMenuItem As New MenuItem()
   Dim menuItem1 As New MenuItem()
   Dim menuItem2 As New MenuItem()
   Dim menuItem3 As New MenuItem()
   Dim menuItem4 As New MenuItem()
   Dim menuItem5 As New MenuItem()
   Dim menuItem6 As New MenuItem()
   Dim menuItem7 As New MenuItem()



   Private Sub Form1_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

      SavePortInfo()

      If DontAskBeforeClose Then Return

      If MsgBox("Do you want to save before exiting?", _
                 MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then

         If Not SaveConfigAsText() Then e.Cancel = True

      End If

   End Sub


   Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim errmsg As String

      osInfo = OSVersion

      If osInfo.Version.Major >= 6 Then
         RegTree = "HKEY_CURRENT_USER"
      Else
         RegTree = "HKEY_LOCAL_MACHINE"

      End If

      errmsg = Space(256)

      BPQDirectory = My.Computer.Registry.GetValue _
         (RegTree & "\Software\G8BPQ\BPQ32", "BPQ Directory", ".")

      BPQDirectory = Environment.ExpandEnvironmentVariables(BPQDirectory)

      OriginalAGWAppl = My.Computer.Registry.GetValue _
         (RegTree & "\Software\G8BPQ\BPQ32\AGWtoBPQ", "ApplMask", "0")


      '  OpenVCOMControlChannel()

      ' CloseHandle(VCOMHandle)

      '     Dim scServices() As ServiceController
      '    scServices = ServiceController.GetServices()



      '       Debug.Print(GetAdapterList(ErrMsg))
      '
      '   Do
      '        '           Name = Space(256)
      '           Desc = Space(256)

      '          Result = GetNextAdapter(Name, Desc)
      '          Name = RTrim(Name)
      '          Desc = RTrim(Desc)
      '          Debug.Print(Name)
      '          Debug.Print("")
      '          Debug.Print(Desc)
      '         Debug.Print("")

      '      Loop Until (Result <> 0)

      ' Set the caption of the menu items.
      topMenuItem.Text = "&File"
      menuItem1.Text = "&Open"
      menuItem2.Text = "&Validate"
      '   menuItem3.Text = "&Update Source and Create Binary"
      '  menuItem4.Text = "&Create Binary"
      menuItem5.Text = "Save Config"
      menuItem6.Text = "Switch to Simple Configuration Mode"
      menuItem7.Text = "&Exit"

      menuItem3.Enabled = False

      ' Add the menu items to the main menu.
      topMenuItem.MenuItems.Add(menuItem1)
      topMenuItem.MenuItems.Add(menuItem2)
      '    topMenuItem.MenuItems.Add(menuItem3)
      '   topMenuItem.MenuItems.Add(menuItem4)
      topMenuItem.MenuItems.Add(menuItem5)
      topMenuItem.MenuItems.Add(menuItem6)

      mainMenu1.MenuItems.Add(topMenuItem)

      ' Add functionality to the menu items using the Click event. 
      AddHandler menuItem1.Click, AddressOf Me.Open_Click
      AddHandler menuItem2.Click, AddressOf Me.Validate_Click
      '  AddHandler menuItem3.Click, AddressOf Me.Save_SourceandBinary
      ' AddHandler menuItem4.Click, AddressOf Me.Save_Binary
      AddHandler menuItem5.Click, AddressOf Me.Save_Source
      AddHandler menuItem6.Click, AddressOf Me.Switch_To_Simple
      AddHandler menuItem7.Click, AddressOf Exit_Click
      ' Assign mainMenu1 to the form.
      Me.Menu = mainMenu1

      SetupAppl()
      InitTextCfgKeywords()

      NewConfig = True

      InitLabels()
      InitHandlers()
      CreateRoutePage()

      If My.Settings.StartMode = "S" Then
         SimpleForm.Visible = True
         Timer1.Enabled = True
         Return
      End If

      GetConfigFile()

      If NewConfig Then

         '           ReDim Preserve Config(2560)   ' Create Enpty File

         NumberOfPortsinConfig = 1

         AddPortTab()

         TxtPortCfg(PORTID).Value(1) = "Loopback"
         TxtPortCfg(PORTTYPE).Value(1) = "INTERNAL"           ' Start with minimal (Internal)
         TYPE.SelectedIndex = 1
         PROTOCOLBox.SelectedIndex = 0

         TxtPortCfg(IOADDR).Value(1) = "0"
         IOADDRBox.Text = "0"
         CurrentPort = 1
         RefreshPortPage()

         TabControl1.SelectedTab = PortsTab

         ConfigLoaded = True

      End If

   End Sub

   Private Sub TabControl2_Deselecting(ByVal sender As Object, ByVal e As System.Windows.Forms.TabControlCancelEventArgs) Handles TabControl2.Deselecting
      If Not ValidateChildren() Then e.Cancel = True
   End Sub

   Private Sub TabControl2_Leave(ByVal sender As Object, ByVal e As System.EventArgs) Handles TabControl2.Leave

      SavePortInfo()
      HasConfigText = False

   End Sub



   Private Sub TabControl2_SelectedIndexChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles TabControl2.SelectedIndexChanged

      If TabControl2.SelectedIndex = NumberOfPorts Then
         NumberOfPortsinConfig = NumberOfPortsinConfig + 1
         AddPortTab()
         SetDefaultL2Params(NumberOfPortsinConfig)
         LoadPortParams(NumberOfPorts)
         RefreshPortPage()
      Else
         SavePortInfo()
         HasConfigText = False
         If Adding Then Exit Sub
         LoadPortParams(TabControl2.SelectedIndex + 1)
         RefreshPortPage()
      End If



   End Sub

   Private Sub TabControl1_Resize(ByVal sender As Object, ByVal e As System.EventArgs) Handles TabControl1.Resize

      TabControl2.Width = TabControl1.Width - 20
      TabControl2.Height = TabControl1.Height - 40

   End Sub



   Private Sub TabControl1_SelectedIndexChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles TabControl1.SelectedIndexChanged

      If TabControl1.SelectedTab Is PortsTab Then

         Adding = True
         LoadPortParams(TabControl2.SelectedIndex + 1)
         RefreshPortPage()
         Adding = False

      End If

   End Sub

   Sub Open_Click(ByVal sender As Object, ByVal e As System.EventArgs)
      ' Create a new OpenFileDialog and display it.

      GetConfigFile()

   End Sub
   Sub Validate_Click(ByVal sender As Object, ByVal e As System.EventArgs)
      ' Create a new OpenFileDialog and display it.

      Debug.Print(Me.ValidateChildren())


   End Sub

   '   Sub Save_Binary(ByVal sender As Object, ByVal e As System.EventArgs)

   '       SaveConfigAsBinary()

   '  End Sub

   '  Sub Save_SourceandBinary(ByVal sender As Object, ByVal e As System.EventArgs)

   '      SaveConfigAsText()

   '      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

   '     File.WriteAllBytes(CfgFile & ".bin", NewConfigBytes)

   '     UpdateAppls()

   '  Dim i As Integer

   '   For i = 0 To Config.Length - 1
   '       Config(i) = NewConfigBytes(i)
   '   Next

   '  End Sub

   Private Sub Save_Source(ByVal sender As Object, ByVal e As System.EventArgs)

      SaveConfigAsText()

   End Sub
   Public Sub Switch_To_Simple(ByVal sender As Object, ByVal e As System.EventArgs)

      SavePortInfo()
      HasConfigText = False

      SimpleForm.CopyConfigtoSimple()
      Me.Visible = False
      SimpleForm.Visible = True

      My.Settings.StartMode = "S"


   End Sub


   Function PreSaveValidate()

      SavePortInfo()

      If Not (Me.ValidateChildren()) Then

         MsgBox("Validation failed", MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

         Return False

      End If

      Return True

   End Function

   Sub UpdateAppls()

      If AGWAppl <> OriginalAGWAppl Then
         If MsgBox("Applications using AGW inteface changed - do you want to update AGWtoBPQ Configuration?", _
         MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
            SetAGWAppl()
         End If
      End If

      If DEDApplChanged() Then
         If MsgBox("Applications using DED inteface changed - do you want to update BPQDED Configuration?", _
         MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
            SetDEDAppl()
         End If
      End If


   End Sub
   Private Function SaveConfigAsText() As Boolean

      SavePortInfo()

      If Not (Me.ValidateChildren()) Then

         MsgBox("Validation failed", MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

         Return False

      End If

      SaveFileDialog1.Title = "Save config file"
      SaveFileDialog1.InitialDirectory = BPQDirectory
      SaveFileDialog1.Filter = "cfg source (*.cfg)|*.cfg"
      SaveFileDialog1.FilterIndex = 1
      SaveFileDialog1.FileName = My.Settings.CfgFileSaveName
      SaveFileDialog1.RestoreDirectory = True

      If SaveFileDialog1.ShowDialog() <> Windows.Forms.DialogResult.OK Then Exit Function

      CfgFile = SaveFileDialog1.FileName

      If (CfgFile Is Nothing) Then Exit Function

      CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileSaveName = CfgFile
      My.Settings.Save()

      Dim FileName As String

      FileName = CfgFile & ".cfg"
      Try
         File.Copy(FileName & ".save", FileName & ".old", True)
      Catch ex As Exception
      End Try
      Try
         File.Copy(FileName, FileName & ".save", True)
      Catch ex As Exception
      End Try

      SaveasTextwithComments()

      '      ReDim Config(NewConfigBytes.Length - 1)

      '    Dim i As Integer

      '    For i = 0 To Config.Length - 1
      'Config(i) = NewConfigBytes(i)
      '   Next


   End Function

   Public Sub GetConfigFile()

      OpenFileDialog1.Title = "Open config file"
      OpenFileDialog1.InitialDirectory = BPQDirectory
      OpenFileDialog1.Filter = "cfg source (*.cfg)|*.cfg"
      OpenFileDialog1.FilterIndex = My.Settings.BinOrTextOpen
      OpenFileDialog1.FileName = My.Settings.CfgFileOpenName
      OpenFileDialog1.RestoreDirectory = True

      If OpenFileDialog1.ShowDialog() = Windows.Forms.DialogResult.OK Then
         CfgFile = OpenFileDialog1.FileName
         If Not (CfgFile Is Nothing) Then
            ReadTextConfig()
         End If
      End If

      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileOpenName = CfgFile
      My.Settings.BinOrTextOpen = OpenFileDialog1.FilterIndex
      My.Settings.Save()

   End Sub

   Public Lines As Integer
   Dim AllLines() As String

   Public Sub ReadTextConfig()

      Dim Line As String, i As Integer

      While TabControl2.TabPages.Count > 0
         TabControl2.TabPages.RemoveAt(0)
      End While

      For i = 1 To NumberOfPortsinConfig
         ClearPort(i)
      Next


      NumberOfPorts = 0
      NumberOfPortsinConfig = 0

      ClearConfig()


      For i = 1 To 32

         ApplName(i).Text = ""
         ApplCmdAlias(i).Text = ""
         ApplType(i).Text = ""
         ApplQual(i).Text = ""
         ApplCall(i).Text = ""
         ApplAlias(i).Text = ""

      Next

      AllLines = File.ReadAllLines(CfgFile, System.Text.Encoding.ASCII)
      Lines = AllLines.Length


      If Lines = 0 Then

         MsgBox("Config file is empty", MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")
         Exit Sub

      End If

      ConfigLoaded = True

      ReDim ParamAtLine(Lines)

      LineNo = -1

      Line = GetLine()

      Do While Line <> Nothing

         If Line <> Nothing Then ProcessCfgLine(Line)
         Line = GetLine()

      Loop

      If NumberOfPorts > 0 Then NewConfig = False

      TabControl2.SelectedIndex = 0

      TabControl1.SelectedIndex = 0

      For i = 1 To 8

         DEDMask(i) = My.Computer.Registry.GetValue _
             (RegTree & "\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(i).Text, 0)

         If (DEDMask(i) >> (i - 1)) And 1 = 1 Then ApplType(i).SelectedIndex = 2

      Next

      Me.ValidateChildren()

      menuItem3.Enabled = True


   End Sub

   Dim Comment As String

   Public Function GetLine() As String

      Dim line As String
      Dim i As Integer

GetNext:

      LineNo = LineNo + 1
      If LineNo >= Lines Then Return Nothing

      line = Trim(AllLines(LineNo))

      If line.Length = 0 Then GoTo GetNext

      If Mid(line, 1, 1) = "#" Then GoTo GetNext
      If Mid(line, 1, 1) = ";" Then GoTo GetNext

      If Mid(line, 1, 2) = "/*" Then
loop1:   LineNo = LineNo + 1
         If LineNo >= Lines Then Return Nothing
         line = Trim(AllLines(LineNo))
         If Mid(line, 1, 2) <> "*/" Then GoTo loop1
         GoTo GetNext

      End If

      Do

         i = InStr(line, Chr(9))

         If i > 0 Then Mid(line, i, 1) = " "

      Loop Until i = 0

      i = InStr(line, ";")

      If i > 0 Then
         Comment = Mid(line, i + 1)
         line = Microsoft.VisualBasic.Left(line, i - 1)
      Else
         Comment = ""
      End If
      line = LTrim(RTrim(line))

      If line = "" Then GoTo GetNext

      Return line

   End Function

   Public Sub ProcessCfgLine(ByVal Line As String)

      Dim i As Integer = 0

      i = InStr(Line, "=")

      If i > 0 Then        ' Key=Value Param

         Key = Microsoft.VisualBasic.Left(Line, i - 1)
         Value = Microsoft.VisualBasic.Mid(Line, i + 1)

         ProcessKeyValuePair()

         Exit Sub

      End If

      If Line = "ROUTES:" Then
         ProcessTextRoutes()
         Exit Sub
      End If

      If Microsoft.VisualBasic.Left(Line, 11) = "APPLICATION" Then

         TxtCfg(APPLPARAM).LineNo = LineNo
         ParamAtLine(LineNo) = APPLPARAM
         ProcessApplLine(Line)

         Exit Sub

      End If

      If Microsoft.VisualBasic.Right(Line, 1) = ":" Then

         ProcessMultilineText(Line)
         Exit Sub

      End If

      If Line = "IPGATEWAY" Then

         ' Get Params up to ***

         ParamAtLine(LineNo) = IPGATEWAY
         TxtCfg(IPGATEWAY).LineNo = LineNo

         Dim n As Integer = 0
         PortConfig(0) = ""
lineloop:
         LineNo = LineNo + 1
         If LineNo >= Lines Then Line = Nothing

         Line = AllLines(LineNo)
         ParamAtLine(LineNo) = -1

         If Line Is Nothing Or Line = "****" Then Exit Sub

         PortConfig(0) = PortConfig(0) & Line & vbCrLf

         GoTo lineloop

      End If


      If Line = "TNCPORT" Then
         ProcessTextTNCPort()
         Exit Sub

      End If

      If Line = "PORT" Then
         ProcessTextPort()
         Exit Sub

      End If

      MsgBox("Invalid Line: " & Line & " at line " & LineNo, MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

   End Sub

   Public Sub ProcessKeyValuePair()
      Dim i As Integer

      For i = 1 To 75

         If Key = TxtCfg(i).Key Then

            TxtCfg(i).LineNo = LineNo
            ParamAtLine(LineNo) = i

            If i = 39 Then       ' Enable Linked
               If Value = "Y" Then EnableLinked.SelectedIndex = 1
               If Value = "N" Then EnableLinked.SelectedIndex = 0
               If Value = "A" Then EnableLinked.SelectedIndex = 2
               Exit Sub
            End If

            If i = 44 Then        ' Applications
               ProcessTextApps(Value)
               Exit Sub
            End If

            If i = 71 Then        ' Old IPGateway

               MsgBox("IPGATEWAY= Param no longer supported. Use new format " & vbCrLf & "IPGATEWAY" & vbCrLf & "Params" & vbCrLf & "****")
               ParamAtLine(LineNo) = -1
               Exit Sub

            End If

            If Not TxtCfg(i).CfgField Is Nothing Then
               If TxtCfg(i).Checkbox Then
                  TxtCfg(i).CfgField.checked = Value
               Else
                  TxtCfg(i).CfgField.text = Value
               End If
            End If

            Exit Sub

         End If

      Next
      Debug.Print(Key)

   End Sub

   Public Sub ProcessMultilineText(ByVal Line As String)

      Dim i As Integer, val As String = ""

      For i = 1 To NumberofParams

         If Line = TxtCfg(i).Key Then

            TxtCfg(i).LineNo = LineNo
            ParamAtLine(LineNo) = i

            Do
               LineNo = LineNo + 1                     ' Don't use GetLine, as may have blank lines
               If LineNo >= Lines Then Return

               Line = AllLines(LineNo)

               If Microsoft.VisualBasic.Left(Line, 3) = "***" Then
                  TxtCfg(i).CfgField.text = Mid(val, 3)
                  Exit Sub
               Else
                  val = val & vbCrLf & Line
               End If

            Loop Until LineNo >= Lines

         End If

      Next

   End Sub

   Public Sub ProcessApplLine(ByVal Value As String)

      ' APPLICATION 1,RMS,C 3 CMS,GM8BPQ-10,BPQRMS,0

      Dim i As Integer

      Dim Values As String()

      Value = Value & ",,,,,"

      Values = Mid(Value, 13).Split(",")

      i = Values(0)
      ApplName(i).Text = Values(1)
      ApplCmdAlias(i).Text = Values(2)
      ApplCall(i).Text = Values(3)
      ApplAlias(i).Text = Values(4)
      ApplQual(i).Text = Values(5)


      '     For i = 1 To 8
      'j = InStr(Value, ",")
      '    ApplName(i).Text = Microsoft.VisualBasic.Left(Value, j - 1)
      '   Value = Mid(Value, j + 1)
      '   Next

   End Sub

   Public Sub ProcessTextApps(ByVal Value As String)

      Dim i As Integer, j As Integer

      Value = Value & ",,,,,,,,"

      For i = 1 To 8
         j = InStr(Value, ",")
         '         ApplName(i).Text = Microsoft.VisualBasic.Left(Value, j - 1)
         Dim Temp() As String = Split(Microsoft.VisualBasic.Left(Value, j - 1), "/")
         ApplName(i).Text = Temp(0)
         If Temp.Length > 1 Then ApplCmdAlias(i).Text = Temp(1)
         Value = Mid(Value, j + 1)
      Next

   End Sub
   Public Sub ProcessTextRoutes()

      Dim Line As String, i As Integer, p As Integer = 0

      TxtCfg(43).LineNo = LineNo
      ParamAtLine(LineNo) = 43

      Do
         Line = GetLine()

         If (Line = Nothing) Or (Microsoft.VisualBasic.Left(Line, 3) = "***") Then Exit Sub

         Line = Line & ",,,,,,"

         i = InStr(Line, ",")
         RouteCall(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         RouteQual(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         RoutePort(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         RouteMaxFrame(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         RouteFrack(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         RoutePaclen(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)
         Line = Mid(Line, i + 1)

         i = InStr(Line, ",")
         Routeinp3(p).Text = Microsoft.VisualBasic.Left(Line, i - 1)

         RouteComment(p) = Comment
         RouteLineno(p) = LineNo

         p = p + 1
         ParamAtLine(LineNo) = p

         If p > 31 Then p = 31

      Loop

   End Sub

   Public Sub ProcessTextPort()

      Dim i As Integer, Port As Integer, Line As String, WinmorPort As Boolean = False


      TxtCfg(41).LineNo = LineNo
      ParamAtLine(LineNo) = 41

      NumberOfPortsinConfig = NumberOfPortsinConfig + 1

      Port = NumberOfPortsinConfig
      RigControl(Port) = ""
      WL2KReport(Port) = ""

      AddPortTab()

      SimpleForm.CreateTab(NumberOfPortsinConfig)

      Do
         Line = GetLine()

         If Line Is Nothing Then Exit Sub

         If UCase(Line).StartsWith("CONFIG") Then

            '  Copy Lines to Endport

            ParamAtLine(LineNo) = -1

            Dim n As Integer = 0
            PortConfig(NumberOfPortsinConfig) = ""
lineloop:
            LineNo = LineNo + 1
            If LineNo >= Lines Then Line = Nothing

            Line = AllLines(LineNo)

            If Line Is Nothing Or Trim(Line) = "ENDPORT" Then
               IDBox(NumberOfPortsinConfig).Text = TxtPortCfg(PORTID).Value(NumberOfPortsinConfig)
               Exit Sub
            End If

            If WinmorPort Then
               Line = Trim(Line)
               If UCase(Line).StartsWith("WL2KREPORT") Then
                  WL2KReport(Port) = WL2KReport(Port) & Mid(Line, 12)
                  ParamAtLine(LineNo) = -1
                  GoTo lineloop
               End If

               If UCase(Line).StartsWith("RIGCONTROL") Then
                  RigControl(Port) = RigControl(Port) & Mid(Line, 12)
                  ParamAtLine(LineNo) = -1
                  GoTo lineloop
               End If
            End If

            If WinmorPort Then
               ParamAtLine(LineNo) = ProcessWinmorLine(Line, NumberOfPortsinConfig) ' Extract Params
            Else
               PortConfig(NumberOfPortsinConfig) = PortConfig(NumberOfPortsinConfig) & Line & vbCrLf
               ParamAtLine(LineNo) = -1
            End If

            GoTo lineloop

         End If
         If Line Is Nothing Or Trim(Line) = "ENDPORT" Then
            IDBox(NumberOfPortsinConfig).Text = TxtPortCfg(PORTID).Value(NumberOfPortsinConfig)
            Exit Sub

         End If

         i = InStr(Line, "=")

         If i > 0 Then        ' Key=Value Param

            Key = Microsoft.VisualBasic.Left(Line, i - 1)
            Value = Microsoft.VisualBasic.Mid(Line, i + 1)

            For i = 1 To NumofPortConfigParams

               If Key = TxtPortCfg(i).Key Then

                  If i = IOADDR Then

                     Value = Convert.ToInt32("0" & Value, 16)

                  End If

                  If i = KISSOPTIONS Then Value = GetTextKissOptions(Value)

                  If i = DLLNAME Then
                     If InStr(UCase(Value), "WINMOR") Then
                        CreateWINMORTab(NumberOfPortsinConfig)
                        WinmorPort = True
                     End If
                  End If

                  TxtPortCfg(i).Value((NumberOfPortsinConfig)) = Value
                  TxtPortCfg(i).LineNo(NumberOfPortsinConfig) = LineNo
                  ParamAtLine(LineNo) = i

                  Exit For

               End If

            Next

         End If

      Loop

   End Sub

   Public Sub ProcessTextTNCPort()

      Dim i As Integer, Line As String

      TxtCfg(40).LineNo = LineNo
      ParamAtLine(LineNo) = 40

      Do
         Line = GetLine()

         If Line Is Nothing Or Line = "ENDPORT" Then

            NumberofTNCPorts = NumberofTNCPorts + 1
            Exit Sub

         End If

         i = InStr(Line, "=")

         If i > 0 Then        ' Key=Value Param

            Key = Microsoft.VisualBasic.Left(Line, i - 1)
            Value = Microsoft.VisualBasic.Mid(Line, i + 1)

            If Key = "COM" Then

               COMPort(NumberofTNCPorts).Text = Value
               Continue Do


            ElseIf Key = "TYPE" Then

               For i = 0 To 3

                  If Value = TNCTypes(i) Then

                     COMType(NumberofTNCPorts).SelectedIndex = i
                     Continue Do

                  End If

               Next

            ElseIf Key = "APPLMASK" Then
               COMAPPLMask(NumberofTNCPorts).Text = Value
               Continue Do
            ElseIf Key = "KISSMASK" Then
               COMKISSMask(NumberofTNCPorts).Text = Value
               Continue Do
            ElseIf Key = "APPLFLAGS" Then
               COMAPPLFlags(NumberofTNCPorts).Text = Value
               Continue Do

            End If

            MsgBox("Invalid TNC Config Line: " & Line & " at line " & LineNo, MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

         End If

      Loop

   End Sub

   Public Sub SaveasTextwithComments()

      Dim i As Integer, j As Integer, p As Integer, Line As String, Comment As String
      Dim textfile As System.IO.StreamWriter
      Dim ApplDone As Boolean = False

      SavePortNo = 0
      '     SaveTNCPortNo = 0


      '   For i = 0 To 15
      'COMPort(i).Tag = False                   ' Indicate not written
      '   Next

      CfgFile = Environment.ExpandEnvironmentVariables(CfgFile)

      textfile = My.Computer.FileSystem.OpenTextFileWriter(CfgFile & ".cfg", False, System.Text.Encoding.ASCII)

      If Lines = 0 Then

         ' We didnt have a source text, so just save without trying to match input file


         textfile.WriteLine("; Updated by WinBPQCFG")
         textfile.WriteLine("")

         For i = 3 To NumberofParams         ' Omit Obolete DOS Params

            If i = BBSCALL Then i = BBSALIAS + 1
            If i = BBSQUAL Then i = i + 1
            If i = APPL1CALL Then i = MAXHOPS ' Skip all APPLCALL/ALIAS/QUAL - 

            If TxtCfg(i).CfgField IsNot Nothing Then

               If i = 39 Then       ' Enable Linked

                  If EnableLinked.SelectedIndex = 1 Then
                     Line = "Y"
                  Else
                     If EnableLinked.SelectedIndex = 2 Then Line = "A" Else Line = "N"
                  End If

                  textfile.WriteLine(TxtCfg(i).Key & "=" & Line)
                  Continue For

               End If

               If TxtCfg(i).MultiLineText Then

                  textfile.WriteLine(TxtCfg(i).Key)
                  textfile.WriteLine(TxtCfg(i).CfgField.text)
                  textfile.WriteLine("***")
                  Continue For

               End If

               If TxtCfg(i).Checkbox Then
                  If TxtCfg(i).CfgField.checked Then
                     textfile.WriteLine(TxtCfg(i).Key & "=1")
                  Else
                     textfile.WriteLine(TxtCfg(i).Key & "=0")
                  End If
               Else
                  textfile.WriteLine(TxtCfg(i).Key & "=" & TxtCfg(i).CfgField.text)
               End If

            End If

         Next

         '       For i = 0 To 15
         '
         '       WriteTNCPortLine(i, textfile)
         '
         '       Next

         textfile.WriteLine("")
         textfile.WriteLine(";------")
         textfile.WriteLine("; Port Definitions")
         textfile.WriteLine(";------")
         textfile.WriteLine("")

         While SavePortNo < NumberOfPorts

            SavePortNo = SavePortNo + 1

            If DeletedPort(SavePortNo) = False Then
               textfile.WriteLine("PORT")
               AddNewPortLines(textfile)
               textfile.WriteLine("ENDPORT")
               textfile.WriteLine("")
            End If

         End While

         textfile.WriteLine("")
         textfile.WriteLine(";------")
         textfile.WriteLine("; Locked Routes")
         textfile.WriteLine(";------")
         textfile.WriteLine("")


         textfile.WriteLine("ROUTES:")

         For p = 0 To 31

            If Mid(RouteCall(p).Text, 1, 1) > " " And RouteLineno(p) = 0 Then

               Line = RTrim(RouteCall(p).Text) & "," & RouteQual(p).Text & "," & RoutePort(p).Text

               If RouteMaxFrame(p).Text > 0 Then Line = Line & "," & RouteMaxFrame(p).Text Else Line = Line & ","
               If RouteFrack(p).Text > 0 Then Line = Line & "," & RouteFrack(p).Text Else Line = Line & ","
               If RoutePaclen(p).Text > 0 Then Line = Line & "," & RoutePaclen(p).Text Else Line = Line & ","
               If Routeinp3(p).Text > 0 Then Line = Line & "," & Routeinp3(p).Text Else Line = Line & ","

               Line = StripCommas(Line)

               textfile.WriteLine(Line)
            End If

         Next

         textfile.WriteLine("***")

         textfile.WriteLine("")
         textfile.WriteLine(";------")
         textfile.WriteLine("; Application Definitions")
         textfile.WriteLine(";------")
         textfile.WriteLine("")


         For j = 1 To 32

            If ApplName(j).Text.Length > 0 Then

               Line = "APPLICATION " & j.ToString & "," & _
                RTrim(ApplName(j).Text) & "," & _
                RTrim(ApplCmdAlias(j).Text) & "," & _
                RTrim(ApplCall(j).Text) & "," & _
                RTrim(ApplAlias(j).Text) & "," & _
                RTrim(ApplQual(j).Text)

               Line = (StripCommas(Line))

               textfile.WriteLine(Line)

            End If
         Next
         textfile.Close()

         Return

      End If

      If AllLines(0) <> "; Updated by WinBPQCFG" Then textfile.WriteLine("; Updated by WinBPQCFG")

      For i = 0 To Lines - 1

         p = ParamAtLine(i)

         If p = 0 Then

            textfile.WriteLine(AllLines(i))             ' Comment line
            Continue For
         End If

         If p = -1 Then Continue For ' Embedded Config or deleted port

         If p = BBSCALL Or p = BBSALIAS Or p = BBSQUAL Then Continue For

         Line = AllLines(i)

         j = InStr(Line, ";")

         If j > 0 Then

            Comment = Mid(Line, j)
            Comment = Chr(9) & Chr(9) & LTrim(RTrim(Comment))
         Else
            Comment = ""
         End If


         If p = 39 Then       ' Enable Linked

            If EnableLinked.SelectedIndex = 1 Then Line = "Y"
            If EnableLinked.SelectedIndex = 0 Then Line = "N"
            If EnableLinked.SelectedIndex = 2 Then Line = "A"

            textfile.WriteLine(TxtCfg(p).Key & "=" & Line & Comment)
            Continue For

         End If

         If p = 40 Then               ' TNCPORT

            '         i = SaveTNCPortWithComments(i, textfile)

            Continue For

         End If

         If p = 41 Then               ' PORT

            Dim ret As Integer

            ret = SavePortWithComments(i, textfile)

            If (ret) Then textfile.WriteLine(AllLines(i)) ' ENDPORT Line

            Continue For

         End If

         If p = 43 Then               ' ROUTES:

            ' Save any newly defined ports

            While SavePortNo < NumberOfPorts

               SavePortNo = SavePortNo + 1

               If DeletedPort(SavePortNo) = False Then
                  textfile.WriteLine("PORT")
                  AddNewPortLines(textfile)
                  textfile.WriteLine("ENDPORT")
                  textfile.WriteLine("")
               End If

            End While

            i = SaveRoutesWithComments(i, textfile)

            textfile.WriteLine(AllLines(i))

            Continue For

         End If

         If p = 44 Or p = APPLPARAM Then               ' APPLICATIONS or APPLICATION

            If ApplDone Then Continue For ' All Appls are output at once

            ApplDone = True

            For j = 1 To 32

               If ApplName(j).Text.Length > 0 Then

                  Line = "APPLICATION " & j.ToString & "," & _
                   RTrim(ApplName(j).Text) & "," & _
                   RTrim(ApplCmdAlias(j).Text) & "," & _
                   RTrim(ApplCall(j).Text) & "," & _
                   RTrim(ApplAlias(j).Text) & "," & _
                   RTrim(ApplQual(j).Text)

                  Line = (StripCommas(Line))

                  textfile.WriteLine(Line)

               End If
            Next

            Continue For

         End If

         If p = IPGATEWAY Then

            If PortConfig(0) <> "" Then
               textfile.WriteLine(Line)
               textfile.Write(PortConfig(0))
               textfile.WriteLine("****")
            End If

            Continue For

         End If

         If Microsoft.VisualBasic.Right(TxtCfg(p).Key, 1) = ":" Then

            ' Process Multiline Text

            textfile.WriteLine(Line)
            textfile.WriteLine(TxtCfg(p).CfgField.text)

            ' Find the *** line

            Do

               i = i + 1

               Line = AllLines(i)

            Loop Until Microsoft.VisualBasic.Left(Line, 3) = "***" Or i > Lines - 2

            textfile.WriteLine(Line)

            Continue For

         End If

         If p < APPL1CALL Or p > 68 Then

            If TxtCfg(p).CfgField IsNot Nothing Then

               If TxtCfg(p).Checkbox Then
                  If TxtCfg(p).CfgField.checked Then
                     textfile.WriteLine(TxtCfg(p).Key & "=1" & Comment)
                  Else
                     textfile.WriteLine(TxtCfg(p).Key & "=0" & Comment)
                  End If
               Else
                  textfile.WriteLine(TxtCfg(p).Key & "=" & TxtCfg(p).CfgField.text & Comment)
               End If

            End If

         End If

      Next

      ' Output all values that were not in original file


      For i = 3 To NumberofParams

         If i = APPL1CALL Then i = APPL8QUAL + 1 ' Skip APPLCALLS etc

         If i = BBSCALL Or i = BBSALIAS Or i = BBSQUAL Or i = 10 Then ' Unproto
            Continue For
         End If

         If i = IPGATEWAY Then

            If TxtCfg(i).LineNo = 0 AndAlso PortConfig(0) <> "" Then
               textfile.WriteLine("IPGATEWAY")
               textfile.Write(PortConfig(0))
               textfile.WriteLine("****")
            End If

            Continue For

         End If

         If TxtCfg(i).LineNo = 0 Then

            If TxtCfg(i).CfgField IsNot Nothing Then

               If TxtCfg(i).Checkbox Then
                  If TxtCfg(i).CfgField.checked Then
                     textfile.WriteLine(TxtCfg(i).Key & "=1")
                  Else
                     textfile.WriteLine(TxtCfg(i).Key & "=0")
                  End If
               Else
                  textfile.WriteLine(TxtCfg(i).Key & "=" & TxtCfg(i).CfgField.text)
               End If

            End If

         End If

      Next


      '    For i = 0 To 15
      '
      '   WriteTNCPortLine(i, textfile)
      '
      '   Next

      textfile.Close()

   End Sub

   Function SaveRoutesWithComments(ByVal n As Integer, ByVal textfile As System.IO.StreamWriter) As Integer


      '   Not so simple as main config, as number of lines may have altered

      '   Scan input till ***
      '       if comment, copy it
      '       if not, find line in routes with same lineno and output it
      '       output any new lines (without a line number)
      '       output ***

      Dim i As Integer, p As Integer, line As String

      textfile.WriteLine(AllLines(n))


      For i = n + 1 To Lines

         line = AllLines(i)

         p = ParamAtLine(i)

         If p > 0 Then ' A route line

            p = p - 1           ' Fiddle, as zero is first record

            If RouteLineno(p) = i Then            ' Hasnt changed

               If Mid(RouteCall(p).Text, 1, 1) > " " Then

                  line = RouteCall(p).Text & "," & RouteQual(p).Text & "," & RoutePort(p).Text
                  If RouteMaxFrame(p).Text > 0 Then line = line & "," & RouteMaxFrame(p).Text Else line = line & ","
                  If RouteFrack(p).Text > 0 Then line = line & "," & RouteFrack(p).Text Else line = line & ","
                  If RoutePaclen(p).Text > 0 Then line = line & "," & RoutePaclen(p).Text Else line = line & ","
                  If Routeinp3(p).Text > 0 Then line = line & "," & Routeinp3(p).Text Else line = line & ","

                  line = StripCommas(line)

                  If RouteComment(p) <> "" Then line = line & Chr(9) & "; " & RouteComment(p)

                  textfile.WriteLine(line)

               End If

               Continue For

            End If

         End If


         If Microsoft.VisualBasic.Left(line, 3) = "***" Then

            For p = 0 To 31

               If Mid(RouteCall(p).Text, 1, 1) > " " And RouteLineno(p) = 0 Then

                  line = RTrim(RouteCall(p).Text) & "," & RouteQual(p).Text & "," & RoutePort(p).Text

                  If RouteMaxFrame(p).Text > 0 Then line = line & "," & RouteMaxFrame(p).Text Else line = line & ","
                  If RouteFrack(p).Text > 0 Then line = line & "," & RouteFrack(p).Text Else line = line & ","
                  If RoutePaclen(p).Text > 0 Then line = line & "," & RoutePaclen(p).Text Else line = line & ","
                  If Routeinp3(p).Text > 0 Then line = line & "," & Routeinp3(p).Text Else line = line & ","

                  line = StripCommas(line)

                  textfile.WriteLine(line)
               End If

            Next

            Return i

         End If

         textfile.WriteLine(line)

      Next

   End Function

   Function SavePortWithComments(ByRef n As Integer, ByVal textfile As System.IO.StreamWriter) As Integer


      '   Not so simple as main config, as number of lines may have altered

      '   Scan input till ENDPORT
      '       if comment, copy it
      '       if not, find line with same lineno and output it
      '       output any new lines (without a line number)
      '       output ENDPORT

      Dim i As Integer, j As Integer, p As Integer, line As String, comment As String

      SavePortNo = SavePortNo + 1

      If DeletedPort(SavePortNo) Then

         ' Skip all lines to ENDPORT

         For i = n + 1 To Lines

            line = AllLines(i)
            If Microsoft.VisualBasic.Left(line, 7) = "ENDPORT" Then
               n = i
               Return 0
            End If
         Next


      End If

      textfile.WriteLine(AllLines(n))         ' The PORT line

      For i = n + 1 To Lines

         line = AllLines(i)

         p = ParamAtLine(i)

         If p = -1 Then Continue For ' An embedded config line

         If p > 0 Then ' A PORT line

            j = InStr(line, ";")

            If j > 0 Then

               comment = Mid(line, j)
               comment = Chr(9) & Chr(9) & LTrim(RTrim(comment))
            Else
               comment = ""
            End If

            If p = PORTTYPE Then

               textfile.WriteLine(" TYPE=" & TxtPortCfg(PORTTYPE).Value(SavePortNo) & comment)
               Continue For

            End If

            If p = PROTOCOL Then

               textfile.WriteLine(" PROTOCOL=" & TxtPortCfg(PROTOCOL).Value(SavePortNo) & comment)
               Continue For

            End If


            If p = IOADDR Then

               If TxtPortCfg(IOADDR).Value(SavePortNo) <> "" Then
                  textfile.WriteLine(" IOADDR=" & Hex(TxtPortCfg(IOADDR).Value(SavePortNo)) & comment)
               End If

               Continue For

            End If

            If p = KISSOPTIONS Then

               Dim Kiss As Integer = TxtPortCfg(KISSOPTIONS).Value(SavePortNo)

               Value = ""

               If (Kiss And 1) = 1 Then Value = Value & "CHECKSUM,"
               If (Kiss And 2) = 2 Then Value = Value & "POLLED,"
               If (Kiss And 4) = 4 Then Value = Value & "ACKMODE,"
               If (Kiss And 8) = 8 Then Value = Value & "SLAVE,"
               If (Kiss And 8) = 8 Then Value = Value & "SLAVE,"
               If (Kiss And 16) = 16 Then Value = Value & "D700,"

               If Value.Length > 0 Then

                  Value = Mid(Value, 1, Value.Length - 1)
                  textfile.WriteLine("  KISSOPTIONS=" & Value & comment)

               End If

               Continue For

            End If

            If TxtPortCfg(p).Checkbox Then

               If TxtPortCfg(p).Value(SavePortNo) = "True" Or TxtPortCfg(p).Value(SavePortNo) = "1" Then
                  textfile.WriteLine(" " & TxtPortCfg(p).Key & "=" & TxtPortCfg(p).SetValue & comment)
               Else

                  If TxtPortCfg(p).UnSetValue Is Nothing Then
                     textfile.WriteLine(" " & TxtPortCfg(p).Key & "=0" & comment)
                  Else
                     textfile.WriteLine(" " & TxtPortCfg(p).Key & "=" & TxtPortCfg(p).UnSetValue & comment)
                  End If

               End If

            Else
               textfile.WriteLine(" " & TxtPortCfg(p).Key & "=" & TxtPortCfg(p).Value(SavePortNo) & comment)
            End If

            Continue For

         End If


         If Microsoft.VisualBasic.Left(line, 7) = "ENDPORT" Then

            AddNewPortLines(textfile)
            n = i
            Return 1

         End If

         textfile.WriteLine(line)

      Next

   End Function

   Function SaveTNCPortWithCommentsx(ByVal n As Integer, ByVal textfile As System.IO.StreamWriter) As Integer


      '   Not worth a .lot of effort, as only supported for DOS compatibility
      '   Sip to ENDPORT, and if first time, output TNC definitions. We don't keep comments

      Dim i As Integer, line As String

      '   i = SaveTNCPortNo
      '
      '   WriteTNCPortLine(i, textfile)
      '
      SaveTNCPortNo = SaveTNCPortNo + 1

      For i = n + 1 To Lines

         line = AllLines(i)

         If Microsoft.VisualBasic.Left(line, 7) = "ENDPORT" Then

            Return i

         End If

      Next

   End Function
   Sub WriteTNCPortLinex(ByVal i As Integer, ByVal textfile As System.IO.StreamWriter)

      If COMPort(i).Tag = False Then

         If COMPort(i).Text <> "" And COMPort(i).Text <> "0" Then

            textfile.WriteLine("TNCPORT")
            textfile.WriteLine(" COM=" & COMPort(i).Text)
            If COMType(i).SelectedIndex > 0 Then textfile.WriteLine(" TYPE=" & TNCTypes(COMType(i).SelectedIndex))
            If COMAPPLMask(i).Text <> "" And COMAPPLMask(i).Text <> "0" Then textfile.WriteLine(" APPLMASK=" & COMAPPLMask(i).Text)
            If COMKISSMask(i).Text <> "" And COMKISSMask(i).Text <> "0" Then textfile.WriteLine(" KISSMASK=" & COMKISSMask(i).Text)
            If COMAPPLFlags(i).Text <> "" And COMAPPLFlags(i).Text <> "0" Then textfile.WriteLine(" APPLFLAGS=" & COMAPPLFlags(i).Text)
            textfile.WriteLine("ENDPORT")
            ' Indicate written

         End If

         COMPort(i).Tag = True

      End If

   End Sub

   Sub AddNewPortLines(ByVal textfile As System.IO.StreamWriter)

      Dim p As Integer

      For p = 1 To NumofPortConfigParams

         If TxtPortCfg(p).LineNo(SavePortNo) = 0 Then ' Not specified

            ' Only ouput PORTNUM if not default (<> current port)

            If p = PORTNUM Then

               If TxtPortCfg(PORTNUM).Value(SavePortNo) <> "" AndAlso TxtPortCfg(PORTNUM).Value(SavePortNo) <> SavePortNo Then

                  textfile.WriteLine(" PORTNUM=" & TxtPortCfg(PORTNUM).Value(SavePortNo))

               End If

               Continue For

            End If


            If p = PORTTYPE Then

               textfile.WriteLine(" TYPE=" & TxtPortCfg(PORTTYPE).Value(SavePortNo))
               Continue For

            End If


            If p = PROTOCOL Then

               ' ASYNC and HDLC WINMOR and PACTOR have PROTOCOL

               Dim NeedProto As Boolean = False

               If TxtPortCfg(PORTTYPE).Value(SavePortNo) = "ASYNC" Then
                  NeedProto = True
                  'ElseIf TxtPortCfg(PORTTYPE).Value(SavePortNo) > 2 Then
                  '  NeedProto = True
               ElseIf TxtPortCfg(PORTTYPE).Value(SavePortNo) = "EXTERNAL" Then
                  ' Exetrnal
                  If TxtPortCfg(DLLNAME).Value(SavePortNo).Contains("Pactor") Or _
                                   TxtPortCfg(DLLNAME).Value(SavePortNo).Contains("WINMOR") Then
                     ' NeedProto = True
                  End If
               End If

               If NeedProto Then
                  textfile.WriteLine(" PROTOCOL=" & TxtPortCfg(PROTOCOL).Value(SavePortNo))
               End If


               Continue For

            End If

            If p = IOADDR Then

               If TxtPortCfg(IOADDR).Value(SavePortNo) <> "0" AndAlso TxtPortCfg(IOADDR).Value(SavePortNo) <> "" Then

                  textfile.WriteLine(" IOADDR=" & Hex(TxtPortCfg(IOADDR).Value(SavePortNo)))

               End If

               Continue For

            End If

            If p = KISSOPTIONS Then

               Dim Kiss As Integer = TxtPortCfg(KISSOPTIONS).Value(SavePortNo)

               Value = ""

               If (Kiss And 1) = 1 Then Value = Value & "CHECKSUM,"
               If (Kiss And 2) = 2 Then Value = Value & "POLLED,"
               If (Kiss And 4) = 4 Then Value = Value & "ACKMODE,"
               If (Kiss And 8) = 8 Then Value = Value & "SLAVE,"
               If (Kiss And 16) = 16 Then Value = Value & "D700,"

               If Value.Length > 0 Then

                  Value = Mid(Value, 1, Value.Length - 1)
                  textfile.WriteLine("  KISSOPTIONS=" & Value)

               End If

               Continue For

            End If

            If TxtPortCfg(p).Checkbox Then

               If TxtPortCfg(p).Value(SavePortNo) = "True" Then
                  textfile.WriteLine(" " & TxtPortCfg(p).Key & "=" & TxtPortCfg(p).SetValue)
               End If

            Else
               If TxtPortCfg(p).Value(SavePortNo) IsNot Nothing Then
                  If TxtPortCfg(p).Value(SavePortNo).Length <> 0 Then
                     If TxtPortCfg(p).Value(SavePortNo) <> "0" Then
                        textfile.WriteLine(" " & TxtPortCfg(p).Key & "=" & TxtPortCfg(p).Value(SavePortNo))
                     End If
                  End If
               End If
            End If
         End If

      Next

      If PortConfig(SavePortNo) IsNot Nothing AndAlso PortConfig(SavePortNo).Length > 0 Then
         textfile.WriteLine(" CONFIG")
         textfile.Write(PortConfig(SavePortNo))       ' Don't want an extra crlf

      End If


   End Sub
   Function StripCommas(ByVal line As String) As String

      While Microsoft.VisualBasic.Right(line, 1) = ","

         line = Microsoft.VisualBasic.Left(line, line.Length - 1)

      End While

      Return line

   End Function

   Private Sub RoutesTab_Validated(ByVal sender As Object, ByVal e As System.EventArgs) Handles RoutesTab.Validated
      Dim i As Integer

      For i = 0 To 31
         ErrorProvider1.SetError(RoutePort(i), "")
      Next


   End Sub

   Private Sub RoutesTab_Validating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs) Handles RoutesTab.Validating

      Dim i As Integer

      For i = 0 To 31
         If RouteCall(i).Text <> String.Empty And RoutePort(i).Text = "0" Then
            ErrorProvider1.SetError(RoutePort(i), "Must be between 1 and " & NumberOfPorts)
            e.Cancel = True
            RoutePort(i).Select(0, RoutePort(i).Text.Length)
         End If

      Next

   End Sub

   Private Sub Form1_Resize(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Resize

      TabControl1.Width = Me.Width - 25
      TabControl1.Height = Me.Height - 70

   End Sub

   Private Sub IDMsgBox_MouseHover(ByVal sender As Object, ByVal e As System.EventArgs) Handles IDMsgBox.MouseHover

   End Sub

   Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick

      Timer1.Enabled = False
      Me.Visible = False

   End Sub

   Public Sub CreateSimpleConfig()

      NumberOfPortsinConfig = 0
      NumberOfPorts = 0

      ObsInitBox.Text = 6
      ObsMinBox.Text = 5
      NodesIntervalBox.Text = 30

      L3TTLBox.Text = 25
      L4RetriesBox.Text = 3
      L4TimeOutBox.Text = 60
      BuffersBox.Text = 999
      PACLENBox.Text = 236
      T3Box.Text = 180
      IdleTimeBox.Text = 900

      EnableLinked.SelectedIndex = 2

      BBSBox.Checked = 1
      NodeBox.Checked = 1
      MaxLinksBox.Text = 64
      MaxNodesBox.Text = 250
      MaxRoutesBox.Text = 64
      MaxCircuitsBox.Text = 128
      IDIntervalBox.Text = 10
      FullCTEXT.Checked = 1
      MinQualBox.Text = 150
      HideNodesBox.Checked = 0
      L4DelayBox.Text = 10
      L4WindowBox.Text = 4
      BTIntervalBox.Text = 60
      AutoSaveBox.Checked = 1
      CIsChatBox.Checked = 1
      MaxRTTBox.Text = 90
      MaxHopsBox.Text = 4

      TabControl1.SelectedIndex = -1

   End Sub

   Private Sub IPGateway_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles IPGatewayButton.Click

      ipgatewayconfig.visible = True
      IPGatewayConfig.IPConfigBox.Text = PortConfig(0)

   End Sub
End Class
