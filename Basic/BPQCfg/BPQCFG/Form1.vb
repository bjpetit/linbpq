
Imports System.IO
Imports Microsoft.VisualBasic.FileIO
Imports System
Imports System.ServiceProcess
Imports System.Diagnostics
Imports System.Threading






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

      CopyConfigtoArray()

      If DontAskBeforeClose Then Return

      If CompareConfig() = False Or AGWAppl <> OriginalAGWAppl Then

         If MsgBox("Changes have not been saved - do you want to save before exiting?", _
                 MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
            If Not SaveConfigAsBinary() Then e.Cancel = True
         End If
      End If

   End Sub

   Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim errmsg As String

      errmsg = Space(256)

      BPQDirectory = My.Computer.Registry.GetValue _
         ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32", "BPQ Directory", ".")

      OriginalAGWAppl = My.Computer.Registry.GetValue _
         ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\AGWtoBPQ", "ApplMask", "0")


      OpenVCOMControlChannel()

      CloseHandle(VCOMHandle)

      Dim scServices() As ServiceController
      scServices = ServiceController.GetServices()



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
      menuItem3.Text = "&Update Source and Create Binary"
      menuItem4.Text = "&Create Binary"
      menuItem5.Text = "Create &Source"
      menuItem6.Text = "Switch to Simple Configuration Mode"
      menuItem7.Text = "&Exit"

      menuItem3.Enabled = False

      ' Add the menu items to the main menu.
      topMenuItem.MenuItems.Add(menuItem1)
      topMenuItem.MenuItems.Add(menuItem2)
      topMenuItem.MenuItems.Add(menuItem3)
      topMenuItem.MenuItems.Add(menuItem4)
      topMenuItem.MenuItems.Add(menuItem5)
      topMenuItem.MenuItems.Add(menuItem6)

      mainMenu1.MenuItems.Add(topMenuItem)

      ' Add functionality to the menu items using the Click event. 
      AddHandler menuItem1.Click, AddressOf Me.Open_Click
      AddHandler menuItem2.Click, AddressOf Me.Validate_Click
      AddHandler menuItem3.Click, AddressOf Me.Save_SourceandBinary
      AddHandler menuItem4.Click, AddressOf Me.Save_Binary
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
      CreateTNCPage()

      If My.Settings.StartMode = "S" Then
         SimpleForm.Visible = True
         Timer1.Enabled = True
         Return
      End If

      GetConfigFile()

      If NewConfig Then

         ReDim Preserve Config(2560)   ' Create Enpty File

         ExtendConfig()

         PortConfigPointer = 2560

         NumberOfPortsinConfig = 1

         AddPortTab()

         GetPortInfoFromBinary(1)

         TxtPortCfg(PORTID).Value(1) = "Loopback"
         TxtPortCfg(PORTTYPE).Value(1) = "1"           ' Start with minimal (Internal)
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

   End Sub



   Private Sub TabControl2_SelectedIndexChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles TabControl2.SelectedIndexChanged

      If TabControl2.SelectedIndex = NumberOfPorts Then
         PortConfigPointer = 2560 + NumberOfPortsinConfig * 512
         ExtendConfig()
         NumberOfPortsinConfig = NumberOfPortsinConfig + 1
         AddPortTab()
         LoadPortParams(NumberOfPorts)
      Else
         SavePortInfo()
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

   Sub Save_Binary(ByVal sender As Object, ByVal e As System.EventArgs)

      SaveConfigAsBinary()

   End Sub

   Sub Save_SourceandBinary(ByVal sender As Object, ByVal e As System.EventArgs)

      SaveConfigAsText()

      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      File.WriteAllBytes(CfgFile & ".bin", NewConfigBytes)

      UpdateAppls()

      Dim i As Integer

      For i = 0 To Config.Length - 1
         Config(i) = NewConfigBytes(i)
      Next

   End Sub

   Private Sub Save_Source(ByVal sender As Object, ByVal e As System.EventArgs)

      SaveConfigAsText()

   End Sub
   Private Sub Switch_To_Simple(ByVal sender As Object, ByVal e As System.EventArgs)

      SavePortInfo()

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

   Function SaveConfigAsBinary() As Boolean

      If Not PreSaveValidate() Then Return False

      SaveFileDialog1.Title = "Save config file"
      SaveFileDialog1.InitialDirectory = BPQDirectory
      SaveFileDialog1.Filter = "cfg binary files  (*.bin)|*.bin"
      SaveFileDialog1.FilterIndex = 1
      SaveFileDialog1.FileName = My.Settings.CfgFileSaveName
      SaveFileDialog1.RestoreDirectory = True

      If SaveFileDialog1.ShowDialog() <> Windows.Forms.DialogResult.OK Then Exit Function

      CfgFile = SaveFileDialog1.FileName

      If (CfgFile Is Nothing) Then Exit Function

      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileSaveName = CfgFile
      My.Settings.Save()

      CopyConfigtoArray()

      File.WriteAllBytes(CfgFile & ".bin", NewConfigBytes)

      UpdateAppls()

      ReDim Config(NewConfigBytes.Length - 1)

      Dim i As Integer

      For i = 0 To Config.Length - 1
         Config(i) = NewConfigBytes(i)
      Next

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
      SaveFileDialog1.Filter = "cfg source (*.txt)|*.txt"
      SaveFileDialog1.FilterIndex = 1
      SaveFileDialog1.FileName = My.Settings.CfgFileSaveName
      SaveFileDialog1.RestoreDirectory = True

      If SaveFileDialog1.ShowDialog() <> Windows.Forms.DialogResult.OK Then Exit Function

      CfgFile = SaveFileDialog1.FileName

      If (CfgFile Is Nothing) Then Exit Function

      CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileSaveName = CfgFile
      My.Settings.Save()

      CopyConfigtoArray()

      SaveasTextwithComments()

      ReDim Config(NewConfigBytes.Length - 1)

      Dim i As Integer

      For i = 0 To Config.Length - 1
         Config(i) = NewConfigBytes(i)
      Next


   End Function

   Public Sub GetConfigFile()

      OpenFileDialog1.Title = "Open config file"
      OpenFileDialog1.InitialDirectory = BPQDirectory
      OpenFileDialog1.Filter = "cfg binary files  (*.bin)|*.bin|cfg source (*.txt)|*.txt"
      OpenFileDialog1.FilterIndex = My.Settings.BinOrTextOpen
      OpenFileDialog1.FileName = My.Settings.CfgFileOpenName
      OpenFileDialog1.RestoreDirectory = True

      If OpenFileDialog1.ShowDialog() = Windows.Forms.DialogResult.OK Then
         CfgFile = OpenFileDialog1.FileName
         If Not (CfgFile Is Nothing) Then

            If LCase(Microsoft.VisualBasic.Right(CfgFile, 3)) = "txt" Then
               ReDim Config(2560)
               ReadTextConfig()
            Else
               ReadConfig()
            End If
         End If
      End If

      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileOpenName = CfgFile
      My.Settings.BinOrTextOpen = OpenFileDialog1.FilterIndex
      My.Settings.Save()

   End Sub

   Public Sub ReadConfig()

      Dim i As Integer

      BPQ32.Checked = False
      BPQCODE.Checked = False

      Config = My.Computer.FileSystem.ReadAllBytes(CfgFile)

      ConfigLength = Config.Length

      NumberOfPortsinConfig = Int((ConfigLength - 2560) / 512)

      If NumberOfPortsinConfig <= 0 Or NumberOfPortsinConfig > 16 Then
         MsgBox("File is corrupt or not a BPQ Config File", _
             MsgBoxStyle.Exclamation + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")
         Exit Sub
      End If

      ConfigLoaded = True

      NewConfig = False

      While TabControl2.TabPages.Count

         TabControl2.TabPages.RemoveAt(0)

      End While

      NumberOfPorts = 0

      For i = 1 To NumberOfPortsinConfig

         AddPortTab()

      Next

      NodeCallBox.Text = GetString(0, 10)
      NodeAliasBox.Text = GetString(10, 10)

      ApplCall(1).Text = GetString(20, 10)
      ApplAlias(1).Text = GetString(30, 10)

      IDMsgBox.Text = GetMultiLineString(512, 512)
      InfoMsgBox.Text = GetMultiLineString(1024, 512)
      CTEXTBox.Text = GetMultiLineString(2048, 512)
      BTEXTBox.Text = GetMultiLineString(121, 80)


      ObsInitBox.Text = Config(40)
      ObsMinBox.Text = Config(42)
      NodesIntervalBox.Text = Get16Bits(44)
      L3TTLBox.Text = Get16Bits(46)
      L4RetriesBox.Text = Get16Bits(48)
      L4TimeOutBox.Text = Get16Bits(50)
      BuffersBox.Text = Get16Bits(52)
      PACLENBox.Text = Get16Bits(54)
      TransDelayBox.Text = Get16Bits(56)
      T3Box.Text = Get16Bits(58)
      IdleTimeBox.Text = Get16Bits(64)

      EMSBox.Checked = Config(66)

      i = Config(67)
      If i = Asc("Y") Then EnableLinked.SelectedIndex = 1
      If i = Asc("N") Then EnableLinked.SelectedIndex = 0
      If i = Asc("A") Then EnableLinked.SelectedIndex = 2

      BBSBox.Checked = Config(68)
      NodeBox.Checked = Config(69)
      HostInterruptBox.Text = Config(70)
      DesqViewBox.Checked = Config(71)
      MaxLinksBox.Text = Config(72)
      MaxNodesBox.Text = Get16Bits(74)
      MaxRoutesBox.Text = Get16Bits(76)
      MaxCircuitsBox.Text = Get16Bits(78)
      IDIntervalBox.Text = Get16Bits(96)
      FullCTEXT.Checked = Get16Bits(98)
      MinQualBox.Text = Get16Bits(100)
      HideNodesBox.Checked = Config(102)
      L4DelayBox.Text = Get16Bits(103)
      L4WindowBox.Text = Get16Bits(105)
      BTIntervalBox.Text = Get16Bits(107)
      AutoSaveBox.Checked = Config(109)
      CIsChatBox.Checked = Config(111)
      IPGatewayBox.Checked = Config(112)
      MaxRTTBox.Text = Config(113)
      MaxHopsBox.Text = Config(114)

      ApplQual(1).Text = Get16Bits(118)

      For i = 0 To 7
         ApplName(i + 1).Text = GetString(256 + 16 * i, 16)
      Next

      For i = 0 To 15

         COMPort(i).Text = Config(384 + 8 * i)
         COMType(i).SelectedIndex = (Config(385 + 8 * i)) / 2
         COMAPPLMask(i).Text = Config(386 + 8 * i)
         COMKISSMask(i).Text = Get16Bits(387 + 8 * i)
         COMAPPLFlags(i).Text = Config(389 + 8 * i)

      Next

      PortConfigPointer = 2560

      For i = 1 To NumberOfPortsinConfig

         GetPortInfoFromBinary(i)

      Next

      Dim RouteBase As Integer = 1536

      For i = 0 To 31

         RouteCall(i).Text = GetString(RouteBase, 10)
         RouteQual(i).Text = Config(RouteBase + 10)
         RoutePort(i).Text = Config(RouteBase + 11)

         If Config(RouteBase + 12) > 127 Then
            RouteMaxFrame(i).Text = Config(RouteBase + 12) - 128
            Routeinp3(i).Text = 1
         Else
            RouteMaxFrame(i).Text = Config(RouteBase + 12)
            Routeinp3(i).Text = 0
         End If

         RouteFrack(i).Text = Get16Bits(RouteBase + 13)
         RoutePaclen(i).Text = Config(RouteBase + 15)

         RouteBase = RouteBase + 16

      Next

      If ConfigLength > PortConfigPointer Then

         ' We have applcalls

         BPQ32.Checked = True

         If Config(PortConfigPointer) > 32 Then

            ApplCall(1).Text = GetPortString(0, 10)
            ApplAlias(1).Text = GetPortString(80, 10)
            ApplQual(1).Text = GetPort16Bits(160, 0)
            DEDMask(1) = My.Computer.Registry.GetValue _
                ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(1).Text, 0)

            If DEDMask(1) = 1 Then ApplType(1).SelectedIndex = 2

         End If

         For i = 2 To 8

            ApplCall(i).Text = GetPortString((i - 1) * 10, 10)
            ApplAlias(i).Text = GetPortString((i - 1) * 10 + 80, 10)
            ApplQual(i).Text = GetPort16Bits(158 + 2 * i, 0)

            DEDMask(i) = My.Computer.Registry.GetValue _
                ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(i).Text, 0)

            If (DEDMask(i) >> (i - 1)) And 1 = 1 Then ApplType(i).SelectedIndex = 2

         Next

      End If

      TabControl2.SelectedIndex = 0

      If BPQ32.Checked = BPQCODE.Checked Then
         MsgBox("Can't Determine if system is BPQ32 or DOS BPQCODE" & vbCrLf & _
                "Please make the appropriate selection 'Config Type BPQ32' or 'DOS BPQCODE'", _
                 MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

         TabControl1.SelectedIndex = 1
         BPQ32.Checked = False
         BPQCODE.Checked = False
      End If

   End Sub

   Public Sub GetPortInfoFromBinary(ByVal i As Integer)

      Dim p As Integer

      For p = 1 To NumofPortConfigParams

         If p = 12 Then
            TxtPortCfg(p).Value(i) = TxtPortCfg(p).BinCfgProc.Invoke(TxtPortCfg(p).Offset, TxtPortCfg(p).Len)
         Else
            TxtPortCfg(p).Value(i) = TxtPortCfg(p).BinCfgProc.Invoke(TxtPortCfg(p).Offset, TxtPortCfg(p).Len)
         End If

      Next

      PortConfigPointer = PortConfigPointer + 512

      If TxtPortCfg(PORTTYPE).Value(i) = "2" Then 'EXTERNAL
         If TxtPortCfg(DLLNAME).Value(i) <> String.Empty Then BPQ32.Checked = 1
      End If

      If TxtPortCfg(PORTTYPE).Value(i) = "0" Then        ' Async
         If TxtPortCfg(IOADDR).Value(i) > 255 Then
            BPQCODE.Checked = True
         Else
            BPQ32.Checked = True
         End If
      End If

   End Sub

   Dim Lines As Integer
   Dim AllLines() As String
   Dim ParamAtLine() As Integer



   Public Sub ReadTextConfig()

      Dim Line As String, i As Integer

      BPQ32.Checked = False
      BPQCODE.Checked = False

      For i = 1 To NumberOfPorts

         TabControl2.TabPages.RemoveAt(0)

      Next

      NumberOfPorts = 0
      NumberOfPortsinConfig = 0

      AllLines = File.ReadAllLines(CfgFile, System.Text.Encoding.ASCII)
      Lines = AllLines.Length


      If Lines = 0 Then

         MsgBox("Config file is empty", MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")
         Exit Sub

      End If

      ConfigLoaded = True

      i = Asc(Mid(AllLines(Lines - 1), 1, 1))
      If i = 0 Then Lines = Lines - 1 ' Nulls May be present at end of DOS config

      ReDim ParamAtLine(Lines)

      LineNo = -1

      Line = GetLine()

      Do While Line <> Nothing

         If Line <> Nothing Then ProcessCfgLine(Line)
         Line = GetLine()

      Loop

      If NumberOfPorts > 0 Then NewConfig = False

      TabControl2.SelectedIndex = 0

      If BPQ32.Checked = BPQCODE.Checked Then
         MsgBox("Can't Determine if system is BPQ32 or DOS BPQCODE" & vbCrLf & _
                "Please make the appropriate selection 'Config Type BPQ32' or 'DOS BPQCODE'", _
                 MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")
      End If

      TabControl1.SelectedIndex = 1

      For i = 1 To 8

         DEDMask(i) = My.Computer.Registry.GetValue _
             ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(i).Text, 0)

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

      line = AllLines(LineNo)

      If line.Length = 0 Then GoTo GetNext

      If Mid(line, 1, 1) = "#" Then GoTo GetNext
      If Mid(line, 1, 1) = ";" Then GoTo GetNext

      If Mid(line, 1, 2) = "/*" Then
loop1:   LineNo = LineNo + 1
         If LineNo >= Lines Then Return Nothing
         line = AllLines(LineNo)
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

      If Microsoft.VisualBasic.Right(Line, 1) = ":" Then

         ProcessMultilineText(Line)
         Exit Sub

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

      For i = 1 To 72

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

      For i = 1 To 67

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

   Public Sub ProcessTextApps(ByVal Value As String)

      Dim i As Integer, j As Integer

      Value = Value & ",,,,,,,,"

      For i = 1 To 8
         j = InStr(Value, ",")
         ApplName(i).Text = Microsoft.VisualBasic.Left(Value, j - 1)
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

      Dim i As Integer, Line As String

      TxtCfg(41).LineNo = LineNo
      ParamAtLine(LineNo) = 41

      PortConfigPointer = 2560 + NumberOfPortsinConfig * 512

      ExtendConfig()

      NumberOfPortsinConfig = NumberOfPortsinConfig + 1

      AddPortTab()

      Config(PortConfigPointer) = NumberOfPortsinConfig    ' Default Portnumber

      Do
         Line = GetLine()

         If Line Is Nothing Or Line = "ENDPORT" Then

            GetPortInfoFromBinary(NumberOfPortsinConfig)
            Exit Sub

         End If

         i = InStr(Line, "=")

         If i > 0 Then        ' Key=Value Param

            Key = Microsoft.VisualBasic.Left(Line, i - 1)
            Value = Microsoft.VisualBasic.Mid(Line, i + 1)

            For i = 1 To NumofPortConfigParams

               If Key = TxtPortCfg(i).Key Then

                  If Not TxtPortCfg(i).TextCfgProc.Invoke(Value, i) Then

                     MsgBox("Invalid Port Config Line: " & Line & " at line " & LineNo, MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")

                  End If

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


   Public Sub CopyConfigtoArray()

      Dim i As Integer, NewLength As Integer

      NewLength = 2560 + NumberOfPorts * 512

      If BPQ32.Checked Then NewLength = NewLength + 176

      ReDim NewConfigBytes(NewLength - 1)   ' Length seems to have a extra byte!

      NewConfigBytes(255) = 22  ' Version

      PutString(0, 10, NodeCallBox.Text)
      PutString(10, 10, NodeAliasBox.Text)
      PutString(20, 10, ApplCall(1).Text)
      PutString(30, 10, ApplAlias(1).Text)
      PutMultilineString(512, 512, IDMsgBox.Text)
      PutMultilineString(1024, 512, InfoMsgBox.Text)
      PutMultilineString(2048, 512, CTEXTBox.Text)
      PutMultilineString(121, 80, BTEXTBox.Text)
      Putbyte(40, ObsInitBox.Text)
      Putbyte(42, ObsMinBox.Text)
      Put16bits(44, NodesIntervalBox.Text)
      Put16bits(46, L3TTLBox.Text)
      Put16bits(48, L4RetriesBox.Text)
      Put16bits(50, L4TimeOutBox.Text)
      Put16bits(52, BuffersBox.Text)
      Put16bits(54, PACLENBox.Text)
      Put16bits(56, TransDelayBox.Text)
      Put16bits(58, T3Box.Text)
      Put16bits(64, IdleTimeBox.Text)
      PutBool(66, EMSBox.Checked)
      If EnableLinked.SelectedIndex = 0 Then Putbyte(67, Asc("N"))
      If EnableLinked.SelectedIndex = 1 Then Putbyte(67, Asc("Y"))
      If EnableLinked.SelectedIndex = 2 Then Putbyte(67, Asc("A"))
      PutBool(68, BBSBox.Checked)
      PutBool(69, NodeBox.Checked)
      Putbyte(70, HostInterruptBox.Text)
      PutBool(71, DesqViewBox.Checked)
      Putbyte(72, MaxLinksBox.Text)
      Put16bits(74, MaxNodesBox.Text)
      Put16bits(76, MaxRoutesBox.Text)
      Put16bits(78, MaxCircuitsBox.Text)
      Put16bits(96, IDIntervalBox.Text)
      PutBool(98, FullCTEXT.Checked)
      Put16bits(100, MinQualBox.Text)
      PutBool(102, HideNodesBox.Checked)
      Put16bits(103, L4DelayBox.Text)
      Put16bits(105, L4WindowBox.Text)
      Put16bits(107, BTIntervalBox.Text)
      PutBool(109, AutoSaveBox.Checked)
      PutBool(111, CIsChatBox.Checked)
      PutBool(112, IPGatewayBox.Checked)
      Put16bits(113, MaxRTTBox.Text)
      Put16bits(114, MaxHopsBox.Text)

      Put16bits(118, ApplQual(1).Text)

      For i = 0 To 15
         If COMPort(i).Text <> "" Then
            Putbyte(384 + 8 * i, COMPort(i).Text)
            Putbyte(385 + 8 * i, COMType(i).SelectedIndex * 2)
            Putbyte(386 + 8 * i, COMAPPLMask(i).Text)
            Put16bits(387 + 8 * i, COMKISSMask(i).Text)
            Putbyte(389 + 8 * i, COMAPPLFlags(i).Text)
         End If
      Next

      PortConfigPointer = 2560

      For i = 1 To NumberOfPorts

         PutPortString(2, 30, TxtPortCfg(PORTID).Value(i))

         PutPort16bits(32, 2 * unMapTypes(TxtPortCfg(PORTTYPE).Value(i)))
         PutPort16bits(34, 2 * TxtPortCfg(PROTOCOL).Value(i))
         PutPort16bits(36, TxtPortCfg(IOADDR).Value(i))
         PutPort16bits(38, TxtPortCfg(INTLEVEL).Value(i))
         PutPort16bits(40, TxtPortCfg(SPEED).Value(i))
         If TxtPortCfg(CHANNEL).Value(i) <> "" Then PutPort16bits(42, Asc(TxtPortCfg(CHANNEL).Value(i)))
         PutPortBoolean(BBSFLAG, i)
         PutPort16bits(46, TxtPortCfg(QUALITY).Value(i))
         PutPort16bits(48, TxtPortCfg(MAXFRAME).Value(i))
         PutPort16bits(50, TxtPortCfg(TXDELAY).Value(i))
         PutPort16bits(52, TxtPortCfg(SLOTTIME).Value(i))
         PutPort16bits(54, TxtPortCfg(PERSIST).Value(i))
         PutPortBoolean(FULLDUP, i)
         PutPortBoolean(SOFTDCD, i)
         PutPort16bits(60, TxtPortCfg(FRACK).Value(i))
         PutPort16bits(62, TxtPortCfg(RESPTIME).Value(i))
         PutPort16bits(64, TxtPortCfg(RETRIES).Value(i))
         PutPort16bits(66, TxtPortCfg(PACLEN).Value(i))
         PutPortString(80, 10, TxtPortCfg(CWID).Value(i), 0)
         PutPortString(90, 10, TxtPortCfg(PORTCALL).Value(i), 0)
         PutPortString(100, 10, TxtPortCfg(PORTALIAS).Value(i), 0)
         PutPortString(256, 256, TxtPortCfg(VALIDCALLS).Value(i), 0)
         PutPort16bits(68, TxtPortCfg(QUALADJUST).Value(i))
         Putport8bits(70, TxtPortCfg(DIGIFLAG).Value(i))
         Putport8bits(71, TxtPortCfg(DIGIPORT).Value(i))
         PutPort16bits(72, TxtPortCfg(DIGIMASK).Value(i))
         PutPort16bits(74, TxtPortCfg(USERS).Value(i))
         PutPortString(128, 72, TxtPortCfg(UNPROTO).Value(i), 0)
         PutPort16bits(0, TxtPortCfg(PORTNUM).Value(i))
         PutPort16bits(76, TxtPortCfg(TXTAIL).Value(i))
         PutPortBoolean(ALIAS_IS_BBS, i)
         PutPortBoolean(L3ONLY, i)
         PutPort16bits(112, TxtPortCfg(KISSOPTIONS).Value(i))
         PutPort16bits(114, TxtPortCfg(INTERLOCK).Value(i))
         PutPort16bits(116, TxtPortCfg(NODESPACLEN).Value(i))
         Putport8bits(118, TxtPortCfg(TXPORT).Value(i))
         PutPortMH(MHEARD, i)
         PutPortBoolean(CWIDTYPE, i)
         Putport8bits(122, TxtPortCfg(MINQUAL).Value(i))
         PutPort16bits(123, TxtPortCfg(MAXDIGIS).Value(i))
         PutPortString(200, 10, TxtPortCfg(PORTALIAS2).Value(i), 0)

         If TxtPortCfg(BCALL).Value(i) <> "" Then PutPortString(226, 10, TxtPortCfg(BCALL).Value(i), 32)
         If TxtPortCfg(DLLNAME).Value(i) <> "" Then PutPortString(210, 16, TxtPortCfg(DLLNAME).Value(i), 32)

         PortConfigPointer = PortConfigPointer + 512

      Next

      For i = 0 To 7
         PutString(256 + 16 * i, 16, ApplName(i + 1).Text)
      Next


      Dim RouteBase As Integer = 1536

      Me.ValidateChildren()

      For i = 0 To 31

         If RouteCall(i).Text <> "" Then
            PutString(RouteBase, 10, RouteCall(i).Text, True)
            Putbyte(RouteBase + 10, RouteQual(i).Text)
            Putbyte(RouteBase + 11, RoutePort(i).Text)
            If Routeinp3(i).Text = "1" Then
               Putbyte(RouteBase + 12, RouteMaxFrame(i).Text + 128)
            Else
               Putbyte(RouteBase + 12, RouteMaxFrame(i).Text)
            End If

            Put16bits(RouteBase + 13, RouteFrack(i).Text)
            Putbyte(RouteBase + 15, RoutePaclen(i).Text)
            RouteBase = RouteBase + 16
         End If
      Next

      If BPQ32.Checked Then

         ' We have applcalls

         For i = 1 To 8

            PutPortString((i - 1) * 10, 10, ApplCall(i).Text)
            PutPortString((i - 1) * 10 + 80, 10, ApplAlias(i).Text)
            PutPort16bits(158 + 2 * i, ApplQual(i).Text)

         Next

      End If

   End Sub

   Private Sub SaveasText()

      Dim value As String, i As Integer

      Dim textfile As System.IO.StreamWriter
      textfile = My.Computer.FileSystem.OpenTextFileWriter(CfgFile, False, System.Text.Encoding.ASCII)

      textfile.WriteLine("; Generated by WinBPQCFG")
      textfile.WriteLine("NODECALL=" & NodeCallBox.Text)
      textfile.WriteLine("NODEALIAS=" & NodeAliasBox.Text)
      textfile.WriteLine("BBSCALL=" & ApplCall(1).Text)
      textfile.WriteLine("BBSALIAS=" & ApplAlias(1).Text)
      textfile.WriteLine("BBSQUAL=" & ApplQual(1).Text)
      textfile.WriteLine("OBSINIT=" & ObsInitBox.Text)
      textfile.WriteLine("OBSMIN=" & ObsMinBox.Text)
      textfile.WriteLine("NODESINTERVAL=" & NodesIntervalBox.Text)
      textfile.WriteLine("L3TIMETOLIVE=" & L3TTLBox.Text)
      textfile.WriteLine("L4RETRIES=" & L4RetriesBox.Text)
      textfile.WriteLine("L4TIMEOUT=" & L4TimeOutBox.Text)
      textfile.WriteLine("BUFFERS=" & BuffersBox.Text)
      textfile.WriteLine("PACLEN=" & PACLENBox.Text)
      textfile.WriteLine("TRANSDELAY=" & TransDelayBox.Text)
      textfile.WriteLine("T3=" & T3Box.Text)
      textfile.WriteLine("IDLETIME=" & IdleTimeBox.Text)

      textfile.WriteLine("MaxHops=" & MaxHopsBox.Text)
      textfile.WriteLine("MAXRTT" & MaxRTTBox.Text)

      If EMSBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("EMS=" & value)

      textfile.WriteLine("ENABLE_LINKED=" & Chr(NewConfigBytes(67)))

      If BBSBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("BBS=" & value)
      If NodeBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("NODE=" & value)
      If DesqViewBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("DESQVIEW=" & value)
      textfile.WriteLine("HOSTINTERRUPT=" & HostInterruptBox.Text)

      textfile.WriteLine("MAXLINKS=" & MaxLinksBox.Text)
      textfile.WriteLine("MAXNODES=" & MaxNodesBox.Text)
      textfile.WriteLine("MAXROUTES=" & MaxRoutesBox.Text)
      textfile.WriteLine("MAXCIRCUITS=" & MaxCircuitsBox.Text)

      textfile.WriteLine("IDINTERVAL=" & IDIntervalBox.Text)

      If FullCTEXT.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("FULL_CTEXT=" & value)

      textfile.WriteLine("MINQUAL=" & MinQualBox.Text)
      If HideNodesBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("HIDENODES=" & value)

      textfile.WriteLine("L4DELAY=" & L4DelayBox.Text)
      textfile.WriteLine("L4WINDOW=" & L4WindowBox.Text)
      textfile.WriteLine("BTINTERVAL=" & BTIntervalBox.Text)

      If AutoSaveBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("AUTOSAVE=" & value)

      If IPGatewayBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("IPGATEWAY=" & value)

      If CIsChatBox.Checked Then value = "1" Else value = "0"
      textfile.WriteLine("C_IS_CHAT=" & value)


      textfile.WriteLine("IDMSG:")
      textfile.WriteLine(IDMsgBox.Text)
      textfile.WriteLine("***")

      textfile.WriteLine("INFOMSG:")
      textfile.WriteLine(InfoMsgBox.Text)
      textfile.WriteLine("***")

      textfile.WriteLine("BTEXT:")
      textfile.WriteLine(BTEXTBox.Text)
      textfile.WriteLine("***")

      textfile.WriteLine("CTEXT:")
      textfile.WriteLine(CTEXTBox.Text)
      textfile.WriteLine("***")

      textfile.WriteLine("ROUTES:")
      For i = 0 To 31

         If Mid(RouteCall(i).Text, 1, 1) > " " Then
            value = RTrim(RouteCall(i).Text) & "," & _
                RouteQual(i).Text & "," & _
                RoutePort(i).Text & "," & _
                RouteMaxFrame(i).Text & "," & _
                RouteFrack(i).Text & "," & _
                RoutePaclen(i).Text & "," & _
                Routeinp3(i).Text

            textfile.WriteLine(value)
         End If

      Next

      textfile.WriteLine("***")

      For i = 1 To NumberOfPorts

         '        If Channel(i) <> Nothing Then textfile.WriteLine("  CHANNEL=" & Channel(i))
         '        If DLLName(i) <> String.Empty Then textfile.WriteLine("  DLLNAME=" & DLLName(i))

         '       If BBSFlag(i) = 0 Then
         'textfile.WriteLine("  BBSFLAG=BBSOK")
         '     Else
         '    textfile.WriteLine("  BBSFLAG=NOBBS")
         '    End If
         '     If CWID(i) <> String.Empty Then textfile.WriteLine("  CWID=" & CWID(i))
         '  If PORTCALL(i) <> String.Empty Then textfile.WriteLine("  PORTCALL=" & PORTCALL(i))
         '  If PORTALIAS(i) <> String.Empty Then textfile.WriteLine("  PORTALIAS=" & PORTALIAS(i))
         '  If VALIDCALLS(i) <> String.Empty Then textfile.WriteLine("  VALIDCALLS=" & VALIDCALLS(i))

         '      If UNPROTO(i) <> String.Empty Then textfile.WriteLine("  UNPROTO=" & UNPROTO(i))
         '     If ALIAS_IS_BBS(i) Then textfile.WriteLine("  ALIAS_IS_BBS=1")
         '     If L3ONLY(i) Then textfile.WriteLine("  L3ONLY=1")

         value = ""

         '         If (KISSOPTIONS(i) And 1) = 1 Then value = value & "CHECKSUM,"
         '        If (KISSOPTIONS(i) And 2) = 2 Then value = value & "POLLED,"
         '        If (KISSOPTIONS(i) And 4) = 4 Then value = value & "ACKMODE,"
         '        If (KISSOPTIONS(i) And 8) = 8 Then value = value & "SLAVE,"
         If value.Length > 0 Then

            value = Mid(value, 1, value.Length - 1)
            textfile.WriteLine("  KISSOPTIONS=" & value)

         End If

         '           If MHEARD(i) Then textfile.WriteLine("  MHEARD=N")
         '   If CWIDTYPE(i) Then textfile.WriteLine("  CWIDTYPE=ONOFF")
         '   If PORTALIAS2(i) <> String.Empty Then textfile.WriteLine("  PORTALIAS2=" & PORTALIAS2(i))
         '   If BCALL(i) <> String.Empty Then textfile.WriteLine("  BCALL=" & BCALL(i))

         textfile.WriteLine("ENDPORT")

      Next

      value = "APPLICATIONS=" & RTrim(ApplName(1).Text) & "," & _
              RTrim(ApplName(2).Text) & "," & _
              RTrim(ApplName(3).Text) & "," & _
              RTrim(ApplName(4).Text) & "," & _
              RTrim(ApplName(5).Text) & "," & _
              RTrim(ApplName(6).Text) & "," & _
              RTrim(ApplName(7).Text) & "," & _
              RTrim(ApplName(8).Text)

      textfile.WriteLine(value)

      If BPQ32.Checked Then

         For i = 1 To 8

            If ApplCall(i).Text <> String.Empty Then textfile.WriteLine("  APPL" & i & "CALL=" & ApplCall(i).Text)
            If ApplAlias(i).Text <> String.Empty Then textfile.WriteLine("  APPL" & i & "ALIAS=" & ApplAlias(i).Text)
            If ApplQual(i).Text <> 0 Then textfile.WriteLine("  APPL" & i & "QUAL=" & ApplQual(i).Text)

         Next

      End If

      textfile.Close()

   End Sub

   Private Sub SaveasTextwithComments()

      Dim i As Integer, j As Integer, p As Integer, Line As String, Comment As String

      Dim textfile As System.IO.StreamWriter

      SavePortNo = 0
      SaveTNCPortNo = 0

      For i = 0 To 15
         COMPort(i).Tag = False                   ' Indicate not written
      Next


      textfile = My.Computer.FileSystem.OpenTextFileWriter(CfgFile & ".txt", False, System.Text.Encoding.ASCII)

      If Lines = 0 Then

         ' We didnt have a source text, so just save without trying to match input file

         For i = 1 To 43                 ' Up to APPLCALLS

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

         For i = 69 To 70                 'Only output APPLCALLS if defined

            If TxtCfg(i).LineNo = 0 Then

               If TxtCfg(i).CfgField IsNot Nothing Then

                  If TxtCfg(i).CfgField.text <> "" And TxtCfg(i).CfgField.text <> "0" Then
                     textfile.WriteLine(TxtCfg(i).Key & "=" & TxtCfg(i).CfgField.text)

                  End If
               End If
            End If
         Next

         If IPGatewayBox.Checked Then Value = "1" Else Value = "0"
         textfile.WriteLine("IPGATEWAY=" & Value)

         If CIsChatBox.Checked Then Value = "1" Else Value = "0"
         textfile.WriteLine("C_IS_CHAT=" & Value)


         For i = 0 To 15

            WriteTNCPortLine(i, textfile)

         Next


         While SavePortNo < NumberOfPorts

            SavePortNo = SavePortNo + 1
            textfile.WriteLine("PORT")
            AddNewPortLines(textfile)
            textfile.WriteLine("ENDPORT")

         End While

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

         Line = "APPLICATIONS=" & RTrim(ApplName(1).Text) & "," & _
                      RTrim(ApplName(2).Text) & "," & _
                      RTrim(ApplName(3).Text) & "," & _
                      RTrim(ApplName(4).Text) & "," & _
                      RTrim(ApplName(5).Text) & "," & _
                      RTrim(ApplName(6).Text) & "," & _
                      RTrim(ApplName(7).Text) & "," & _
                      RTrim(ApplName(8).Text)

         Line = (StripCommas(Line))

         textfile.WriteLine(Line)

         If BPQ32.Checked Then

            For i = 45 To 68                 'Only output APPLCALLS if defined

               If TxtCfg(i).LineNo = 0 Then

                  If TxtCfg(i).CfgField IsNot Nothing Then

                     If TxtCfg(i).CfgField.text <> "" And TxtCfg(i).CfgField.text <> "0" Then
                        textfile.WriteLine(TxtCfg(i).Key & "=" & TxtCfg(i).CfgField.text)

                     End If
                  End If
               End If
            Next
         End If

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

            i = SaveTNCPortWithComments(i, textfile)

            Continue For

         End If

         If p = 41 Then               ' PORT

            i = SavePortWithComments(i, textfile)

            textfile.WriteLine(AllLines(i))         ' ENDPORT Line

            Continue For

         End If

         If p = 43 Then               ' ROUTES:

            ' Save any newly defined ports

            While SavePortNo < NumberOfPorts

               SavePortNo = SavePortNo + 1
               textfile.WriteLine("PORT")
               AddNewPortLines(textfile)
               textfile.WriteLine("ENDPORT")

            End While


            i = SaveRoutesWithComments(i, textfile)

            textfile.WriteLine(AllLines(i))

            Continue For

         End If

         If p = 44 Then               ' APPLICATIONS

            Line = "APPLICATIONS=" & RTrim(ApplName(1).Text) & "," & _
                                    RTrim(ApplName(2).Text) & "," & _
                                    RTrim(ApplName(3).Text) & "," & _
                                    RTrim(ApplName(4).Text) & "," & _
                                    RTrim(ApplName(5).Text) & "," & _
                                    RTrim(ApplName(6).Text) & "," & _
                                    RTrim(ApplName(7).Text) & "," & _
                                    RTrim(ApplName(8).Text)

            Line = (StripCommas(Line)) & Comment

            textfile.WriteLine(Line)

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

         If p < APPL1CALL Or BPQ32.Checked Then

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


      For i = 1 To 44                 ' Up to APPLCALLS

         If BPQ32.Checked Then       ' Dont output missing BBSCALL etc if APPL1CALL present

            If i = BBSCALL And TxtCfg(APPL1CALL).CfgField.text <> "" Then
               Continue For
            ElseIf i = BBSALIAS And TxtCfg(APPL1ALIAS).CfgField.text <> "" Then
               Continue For
            ElseIf i = BBSQUAL And TxtCfg(APPL1QUAL).CfgField.text <> "0" Then
               Continue For

            End If

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


      For i = 0 To 15

         WriteTNCPortLine(i, textfile)

      Next

      If BPQ32.Checked Then

         For i = 45 To 68                 'Only output APPLCALLS if defined

            If TxtCfg(i).LineNo = 0 Then

               If TxtCfg(i).CfgField IsNot Nothing Then

                  If TxtCfg(i).CfgField.text <> "" And TxtCfg(i).CfgField.text <> "0" Then
                     textfile.WriteLine(TxtCfg(i).Key & "=" & TxtCfg(i).CfgField.text)

                  End If
               End If
            End If
         Next
      End If

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

   Function SavePortWithComments(ByVal n As Integer, ByVal textfile As System.IO.StreamWriter) As Integer


      '   Not so simple as main config, as number of lines may have altered

      '   Scan input till ENDPORT
      '       if comment, copy it
      '       if not, find line with same lineno and output it
      '       output any new lines (without a line number)
      '       output ENDPORT

      Dim i As Integer, j As Integer, p As Integer, line As String, comment As String

      textfile.WriteLine(AllLines(n))         ' The PORT line

      SavePortNo = SavePortNo + 1

      For i = n + 1 To Lines

         line = AllLines(i)

         p = ParamAtLine(i)

         If p > 0 Then ' A PORT line

            j = InStr(line, ";")

            If j > 0 Then

               comment = Mid(line, j)
               comment = Chr(9) & Chr(9) & LTrim(RTrim(comment))
            Else
               comment = ""
            End If

            If p = PORTTYPE Then

               textfile.WriteLine(" TYPE=" & Types(TxtPortCfg(PORTTYPE).Value(SavePortNo)) & comment)
               Continue For

            End If

            If p = PROTOCOL Then

               textfile.WriteLine(" PROTOCOL=" & Protos(TxtPortCfg(PROTOCOL).Value(SavePortNo)) & comment)
               Continue For

            End If


            If p = IOADDR Then

               textfile.WriteLine(" IOADDR=" & Hex(TxtPortCfg(IOADDR).Value(SavePortNo)) & comment)
               Continue For

            End If

            If p = KISSOPTIONS Then

               Dim Kiss As Integer = TxtPortCfg(KISSOPTIONS).Value(SavePortNo)

               Value = ""

               If (Kiss And 1) = 1 Then Value = Value & "CHECKSUM,"
               If (Kiss And 2) = 2 Then Value = Value & "POLLED,"
               If (Kiss And 4) = 4 Then Value = Value & "ACKMODE,"
               If (Kiss And 8) = 8 Then Value = Value & "SLAVE,"

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
            Return i

         End If

         textfile.WriteLine(line)

      Next

   End Function

   Function SaveTNCPortWithComments(ByVal n As Integer, ByVal textfile As System.IO.StreamWriter) As Integer


      '   Not worth a .lot of effort, as only supported for DOS compatibility
      '   Sip to ENDPORT, and if first time, output TNC definitions. We don't keep comments

      Dim i As Integer, line As String

      i = SaveTNCPortNo

      WriteTNCPortLine(i, textfile)

      SaveTNCPortNo = SaveTNCPortNo + 1

      For i = n + 1 To Lines

         line = AllLines(i)

         If Microsoft.VisualBasic.Left(line, 7) = "ENDPORT" Then

            Return i

         End If

      Next

   End Function
   Sub WriteTNCPortLine(ByVal i As Integer, ByVal textfile As System.IO.StreamWriter)

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

               If TxtPortCfg(PORTNUM).Value(SavePortNo) And TxtPortCfg(PORTNUM).Value(SavePortNo) <> SavePortNo Then

                  textfile.WriteLine(" PORTNUM=" & TxtPortCfg(PORTNUM).Value(SavePortNo))

               End If

               Continue For

            End If


            If p = PORTTYPE Then

               textfile.WriteLine(" TYPE=" & Types(TxtPortCfg(PORTTYPE).Value(SavePortNo)))
               Continue For

            End If


            If p = PROTOCOL Then

               ' ASYNC and HDLC WINMOR and PACTOR have PROTOCOL

               Dim NeedProto As Boolean = False

               If TxtPortCfg(PORTTYPE).Value(SavePortNo) = 0 Then
                  NeedProto = True
               ElseIf TxtPortCfg(PORTTYPE).Value(SavePortNo) > 2 Then
                  NeedProto = True
               ElseIf TxtPortCfg(PORTTYPE).Value(SavePortNo) = "2" Then
                  ' Exetrnal
                  If TxtPortCfg(DLLNAME).Value(SavePortNo).Contains("Pactor") Or _
                                   TxtPortCfg(DLLNAME).Value(SavePortNo).Contains("WINMOR") Then
                     NeedProto = True
                  End If
               End If

               If NeedProto Then
                  textfile.WriteLine(" PROTOCOL=" & Protos(TxtPortCfg(PROTOCOL).Value(SavePortNo)))
               End If


               Continue For

            End If

            If p = IOADDR Then

               If TxtPortCfg(IOADDR).Value(SavePortNo) <> "0" Then

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

   Private Sub BPQ32_Validated(ByVal sender As Object, ByVal e As System.EventArgs) Handles BPQ32.Validated
      ErrorProvider1.SetError(BPQ32, "")
      ErrorProvider1.SetError(BPQCODE, "")
   End Sub


   Private Sub BPQ32_Validating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs) Handles BPQ32.Validating

      If BPQ32.Checked = BPQCODE.Checked Then
         ErrorProvider1.SetError(sender, "Select either BPQ32 or BPQCODE")
         e.Cancel = True
      End If

   End Sub

   Private Sub BPQCODE_Validated(ByVal sender As Object, ByVal e As System.EventArgs) Handles BPQCODE.Validated
      ErrorProvider1.SetError(BPQ32, "")
      ErrorProvider1.SetError(BPQCODE, "")
   End Sub

   Private Sub BPQCODE_Validating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs) Handles BPQCODE.Validating

      If BPQ32.Checked = BPQCODE.Checked Then
         ErrorProvider1.SetError(sender, "Select either BPQ32 or BPQCODE")
         e.Cancel = True
      End If

   End Sub

   Private Sub IDMsgBox_MouseHover(ByVal sender As Object, ByVal e As System.EventArgs) Handles IDMsgBox.MouseHover

   End Sub

   Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick

      Timer1.Enabled = False
      Me.Visible = False

   End Sub

   Public Sub CreateSimpleConfig()

      Dim i As Integer, NewLength As Integer

      NewLength = 2560 + 176

      ReDim NewConfigBytes(NewLength - 1)   ' Length seems to have a extra byte!

      NewConfigBytes(255) = 22  ' Version

      Putbyte(40, 6) ' ObsInit
      Putbyte(42, 5) 'ObsMin
      Put16bits(44, 30) '  NodesInterval
      Put16bits(46, 25) 'L3TTL
      Put16bits(48, 3) 'L4Retries
      Put16bits(50, 60) 'L4TimeOut
      Put16bits(52, 999) 'Buffers
      Put16bits(54, 236) 'PACLEN
      Put16bits(56, 1) 'TransDelay
      Put16bits(58, 180) 'T3
      Put16bits(64, 900) 'IdleTime
      PutBool(66, 0) 'EMS
      Putbyte(67, Asc("A")) 'EnableLinked
      PutBool(68, 1) 'BBS
      PutBool(69, 1) 'Node
      Putbyte(70, 127) 'HostInterrupt
      PutBool(71, 1) 'DesqView
      Putbyte(72, 64) 'MaxLinks
      Put16bits(74, 250) 'MaxNodes
      Put16bits(76, 64) 'MaxRoutes
      Put16bits(78, 128) 'MaxCircuits
      Put16bits(96, 10) 'IDInterval
      PutBool(98, 1) 'FullCTEXT
      Put16bits(100, 150) 'MinQual
      PutBool(102, 0) 'HideNodes
      Put16bits(103, 10) 'L4Delay
      Put16bits(105, 4) 'L4Window
      Put16bits(107, 60) 'BTInterval
      PutBool(109, 1) 'AutoSaveBox
      PutBool(111, 1) 'CisChatBox
      PutBool(112, 0) 'IPwayBox
      Put16bits(113, 90) 'MaxRTT
      Put16bits(114, 4) 'MaxHops

      BPQ32.Checked = True

      Config = NewConfigBytes

      ConfigLength = Config.Length

      NumberOfPortsinConfig = 0
      NumberOfPorts = 0

      ObsInitBox.Text = Config(40)
      ObsMinBox.Text = Config(42)
      NodesIntervalBox.Text = Get16Bits(44)

      L3TTLBox.Text = Get16Bits(46)
      L4RetriesBox.Text = Get16Bits(48)
      L4TimeOutBox.Text = Get16Bits(50)
      BuffersBox.Text = Get16Bits(52)
      PACLENBox.Text = Get16Bits(54)
      TransDelayBox.Text = Get16Bits(56)
      T3Box.Text = Get16Bits(58)
      IdleTimeBox.Text = Get16Bits(64)

      EMSBox.Checked = Config(66)

      i = Config(67)
      If i = Asc("Y") Then EnableLinked.SelectedIndex = 1
      If i = Asc("N") Then EnableLinked.SelectedIndex = 0
      If i = Asc("A") Then EnableLinked.SelectedIndex = 2

      BBSBox.Checked = Config(68)
      NodeBox.Checked = Config(69)
      HostInterruptBox.Text = Config(70)
      DesqViewBox.Checked = Config(71)
      MaxLinksBox.Text = Config(72)
      MaxNodesBox.Text = Get16Bits(74)
      MaxRoutesBox.Text = Get16Bits(76)
      MaxCircuitsBox.Text = Get16Bits(78)
      IDIntervalBox.Text = Get16Bits(96)
      FullCTEXT.Checked = Get16Bits(98)
      MinQualBox.Text = Get16Bits(100)
      HideNodesBox.Checked = Config(102)
      L4DelayBox.Text = Get16Bits(103)
      L4WindowBox.Text = Get16Bits(105)
      BTIntervalBox.Text = Get16Bits(107)
      AutoSaveBox.Checked = Config(109)
      CIsChatBox.Checked = Config(111)
      IPGatewayBox.Checked = Config(112)
      MaxRTTBox.Text = Config(113)
      MaxHopsBox.Text = Config(114)

      TabControl1.SelectedIndex = -1

   End Sub

End Class
