Module Module1

   '  Public Declare Function GetAdapterList Lib "c:\bpq32\bpqadaptersdll.dll" _
   '       Alias "_GetAdapterList@4" (ByVal ErrMsg As String) As Integer

   ' Public Declare Function GetNextAdapter Lib "c:\bpq32\bpqadaptersdll.dll" _
   '      Alias "_GetNextAdapter@8" (ByVal Name As String, ByVal Desc As String) As Integer

   Public osInfo As OperatingSystem

   Public RegTree As String

   Public BPQDirectory As String
   Public CfgFile As String
   Public AGWAppl As Integer
   Public OriginalAGWAppl As Integer
   Public DEDMask(8) As Integer    ' As held in registry

   Public NewConfig As Boolean
   Public Adding As Boolean

   Public ParamAtLine() As Integer


   '  Public Config As Byte()
   '  Public NewConfigBytes() As Byte

   '  Public ConfigLength As Integer
   Public NumberOfPorts As Integer
   Public NumberOfPortsinConfig As Integer
   Public NumberofTNCPorts As Integer

   Public DontAskBeforeClose As Boolean
   Public ConfigLoaded As Boolean = False
   Public PortBeingValidated As Integer

   Public HasConfigText As Boolean = False

   Public SaveProc(32) As SavePortValue
   Public PORTTABS(32) As System.Windows.Forms.TabPage

   Public WINMORPort(32) As NumTextBox
   Public WINMORPTT(32) As ComboBox
   Public PTTCOMM(32) As ComboBox

   Public DriveLevel(32) As BPQCFG.NumTextBox
   Public BW(32) As ComboBox
   Public DebugLog(32) As CheckBox
   Public CWID(32) As CheckBox
   Public BusyLock(32) As CheckBox
   Public MyLabel1(32) As Label
   Public MyLabel2(32) As Label
   Public MyLabel3(32) As Label
   Public MyLabel4(32) As Label
   Public IDBox(32) As TextBox
   Public IDLabel(32) As Label
   Public PathLabel(32) As Label
   Public PathBox(32) As TextBox

   Public RigControl(32) As String
   Public WL2KReport(32) As String

   Public DeletedPort(32) As Boolean

   Public Structure TxtCfgInfoStruct

      Public Key As String
      '      Public Value As String
      Public CfgField As Object ' Textbox (or Whatever) containing value
      Public Checkbox As Boolean ' Set if Checkbox
      Public MultiLineText As Boolean
      Public LineNo As Integer  ' Line No in Source config

   End Structure

   Public Structure TxtPortCfgInfoStruct

      Public Key As String
      Public Value() As String    ' Text value of param
      Public CfgField As Object ' Textbox (or Whatever) containing value
      '      Public Offset As Integer
      Public Len As Integer
      Public Checkbox As Boolean ' Set if Checkbox
      Public SetValue As String   ' Value for boolean param
      Public UnSetValue As String
      Public LineNo() As Integer  ' Line No in Source config

   End Structure

   Public Const NumberofParams = 75

   Public TxtCfg(NumberofParams) As TxtCfgInfoStruct
   Public TxtPortCfg(NumofPortConfigParams) As TxtPortCfgInfoStruct
   Public SavePortNo As Integer
   Public SaveTNCPortNo As Integer

   Public Const BBSCALL = 8
   Public Const BBSALIAS = 9
   Public Const BBSQUAL = 30

   Public Const APPL1CALL = 45
   Public Const APPL1ALIAS = 46
   Public Const APPL1QUAL = 47
   Public Const APPL8QUAL = 68
   Public Const MAXHOPS = 69
   Public Const APPLPARAM = 74
   Public Const IPGATEWAY = 75

   Public Const NumofPortConfigParams = 45
   Public Const PORTNUM = 1
   Public Const PORTID = 2
   Public Const PORTTYPE = 3
   Public Const DLLNAME = 4
   Public Const PROTOCOL = 5
   Public Const KISSOPTIONS = 6
   Public Const IOADDR = 7
   Public Const INTLEVEL = 8
   Public Const SPEED = 9
   Public Const CHANNEL = 10
   Public Const MHEARD = 11
   Public Const QUALITY = 12
   Public Const MAXFRAME = 13
   Public Const TXDELAY = 14
   Public Const TXTAIL = 15
   Public Const SLOTTIME = 16
   Public Const PERSIST = 17
   Public Const FULLDUP = 18
   Public Const SOFTDCD = 19
   Public Const FRACK = 20
   Public Const RESPTIME = 21
   Public Const RETRIES = 22
   Public Const PACLEN = 23
   Public Const MAXDIGIS = 24
   Public Const DIGIFLAG = 25
   Public Const DIGIPORT = 26
   'Public Const CWID = 27
   Public Const CWIDTYPE = 28
   Public Const PORTCALL = 29
   Public Const PORTALIAS = 30
   Public Const PORTALIAS2 = 31
   Public Const ALIAS_IS_BBS = 32
   Public Const BBSFLAG = 33
   Public Const MINQUAL = 34
   Public Const NODESPACLEN = 35
   Public Const QUALADJUST = 36
   Public Const BCALL = 37
   Public Const UNPROTO = 38
   Public Const L3ONLY = 39
   Public Const INTERLOCK = 40
   Public Const TXPORT = 41
   Public Const USERS = 42
   Public Const VALIDCALLS = 43
   Public Const DIGIMASK = 44
   Public Const NOKEEPALIVES = 45


   Public CurrRow As Integer, CurrCol As Integer

   Public RouteCall(32) As TextBox
   Public RouteQual(32) As NumTextBox
   Public RoutePort(32) As NumTextBox
   Public RouteMaxFrame(32) As NumTextBox
   Public RouteFrack(32) As NumTextBox
   Public RoutePaclen(32) As NumTextBox
   Public Routeinp3(32) As NumTextBox
   Public RouteComment(32) As String
   Public RouteLineno(32) As Integer



   Public COMPort(16) As TextBox
   Public COMType(16) As ComboBox
   Public COMKISSMask(16) As TextBox
   Public COMAPPLMask(16) As TextBox
   Public COMAPPLFlags(16) As TextBox

   Public COMLabel(10) As Label

   Public ApplLabel(32) As Label
   Public ApplName(32) As BPQCFG.ApplNameTextBox
   Public ApplCmdAlias(32) As BPQCFG.ApplNameTextBox
   Public ApplType(32) As System.Windows.Forms.ComboBox
   Public ApplQual(32) As BPQCFG.NumTextBox
   Public ApplCall(32) As BPQCFG.CallsignTextBox
   Public ApplAlias(32) As BPQCFG.AliasTextBox

   Public PortTab(32) As TabPage
   Public PortConfig(32) As String           ' Any CONFIG section lines for this port
   Public CurrentPort As Integer

   Public PortIDBox As TextBox = New TextBox
   Public WithEvents TYPE As ComboBox = New ComboBox
   Public WithEvents PROTOCOLBox As ComboBox = New ComboBox
   Public IOADDRBox As TextBox = New TextBox
   Public INTLEVELBox As NumTextBox = New NumTextBox(255)
   Public SPEEDBox As NumTextBox = New NumTextBox(65535)
   Public CHANNELBox As TextBox = New TextBox
   Public WithEvents DLLNAMEBox As ComboBox = New ComboBox
   Public WithEvents PortConfigButton As Button = New Button
   Public BBSFLAGBox As CheckBox = New CheckBox
   Public QUALITYBox As NumTextBox = New NumTextBox(255)
   Public MAXFRAMEBox As NumTextBox = New NumTextBox(7)
   Public TXDELAYBox As NumTextBox = New NumTextBox(2000)
   Public SLOTTIMEBox As NumTextBox = New NumTextBox(1000)
   Public PERSISTBox As NumTextBox = New NumTextBox(255)
   Public FULLDUPBox As CheckBox = New CheckBox
   Public SOFTDCDBox As CheckBox = New CheckBox
   Public FRACKBox As NumTextBox = New NumTextBox(30000)
   Public RESPTIMEBox As NumTextBox = New NumTextBox(10000)
   Public RETRIESBox As NumTextBox = New NumTextBox(100)
   Public PACLENBox As NumTextBox = New NumTextBox(256)
   Public CWIDBox As CallsignTextBox = New CallsignTextBox
   Public PORTCALLBox As CallsignTextBox = New CallsignTextBox
   Public PORTALIASBox As AliasTextBox = New AliasTextBox
   Public VALIDCALLSBox As TextBox = New TextBox
   Public QUALADJUSTBox As NumTextBox = New NumTextBox(255)
   Public DIGIFLAGBox As NumTextBox = New NumTextBox(255)
   Public DIGIPORTBox As NumTextBox = New NumTextBox(16)
   Public DIGIMASKBox As NumTextBox = New NumTextBox(65535)
   Public USERSBox As NumTextBox = New NumTextBox(255)
   Public UNPROTOBox As TextBox = New TextBox
   Public PORTNUMBox As NumOrEmptyTextBox = New NumOrEmptyTextBox(32)
   Public TXTAILBox As NumTextBox = New NumTextBox(100)
   Public ALIAS_IS_BBSBox As CheckBox = New CheckBox
   Public L3ONLYBox As CheckBox = New CheckBox
   'Public KISSOPTIONSBox As TextBox = New TextBox
   Public WithEvents KISSOptionsBox As New System.Windows.Forms.CheckedListBox
   Public NoKeepalivesBox As CheckBox = New CheckBox


   Public INTERLOCKBox As NumTextBox = New NumTextBox(16)
   Public NODESPACLENBox As NumTextBox = New NumTextBox(256)
   Public TXPORTBox As NumTextBox = New NumTextBox(16)
   Public MHEARDBox As CheckBox = New CheckBox
   Public CWIDTYPEBox As CheckBox = New CheckBox
   Public MINQUALBox As NumTextBox = New NumTextBox(255)
   Public MAXDIGISBox As NumTextBox = New NumTextBox(8)
   Public PORTALIAS2Box As AliasTextBox = New AliasTextBox
   Public BCALLBox As CallsignTextBox = New CallsignTextBox

   Public PortIDLabel As Label = New Label
   Public TYPELabel As Label = New Label
   Public PROTOCOLLabel As Label = New Label
   Public IOADDRLabel As Label = New Label
   Public INTLEVELLabel As Label = New Label
   Public SPEEDLabel As Label = New Label
   Public CHANNELLabel As Label = New Label
   Public QUALITYLabel As Label = New Label
   Public MAXFRAMELabel As Label = New Label
   Public TXDELAYLabel As Label = New Label
   Public SLOTTIMELabel As Label = New Label
   Public PERSISTLabel As Label = New Label
   Public FRACKLabel As Label = New Label
   Public RESPTIMELabel As Label = New Label
   Public RETRIESLabel As Label = New Label
   Public PACLENLabel As Label = New Label
   Public CWIDLabel As Label = New Label
   Public PORTCALLLabel As Label = New Label
   Public PORTALIASLabel As Label = New Label
   Public VALIDCALLSLabel As Label = New Label
   Public QUALADJUSTLabel As Label = New Label
   Public DIGIFLAGLabel As Label = New Label
   Public DIGIPORTLabel As Label = New Label
   Public DIGIMASKLabel As Label = New Label
   Public USERSLabel As Label = New Label
   Public UNPROTOLabel As Label = New Label
   Public PORTNUMLabel As Label = New Label
   Public TXTAILLabel As Label = New Label
   Public KISSOPTIONSLabel As Label = New Label
   Public INTERLOCKLabel As Label = New Label
   Public NODESPACLENLabel As Label = New Label
   Public TXPORTLabel As Label = New Label
   Public MINQUALLabel As Label = New Label
   Public MAXDIGISLabel As Label = New Label
   Public PORTALIAS2Label As Label = New Label
   Public DLLNAMELabel As Label = New Label
   Public BCALLLabel As Label = New Label

   Public PortParamsLabel As Label = New Label
   Public PortParamsBox As TextBox = New TextBox

   Public RigControlLabel As Label = New Label
   Public RigControlBox As TextBox = New TextBox

   Public WL2KReportLabel As Label = New Label
   Public WL2KReportBox As TextBox = New TextBox

   '   Public Const ASYNC = 0
   '   Public Const PC120 = 1
   '   Public Const DRSI = 2
   '   Public Const DE56 = 2
   '  Public Const TOSH = 3
   '  Public Const QUAD = 4
   '   Public Const RLC100 = 5
   '   Public Const RLC400 = 6
   '  Public Const INTERNAL = 7
   '   Public Const EXTERNAL = 8
   '  Public Const BAYCOM = 9
   '  Public Const PA0HZP = 10

   Public Types() As String = New String() _
   {"DELETED", "ASYNC", "INTERNAL", "EXTERNAL", "PC120", "DRSI", "DE56", "TOSH", "QUAD", _
   "RLC100", "RLC400", "BAYCOM", _
   "PA0HZP"}

   Public Protos() As String = New String() {"KISS", "NETROM", "BPQKISS", "HDLC", "L2", "WINMOR", "PACTOR"}
   Public DLLNames() As String = New String() {"BPQVKISS.dll", "BPQAXIP.dll", "BPQETHER.dll", "BPQTOAGW.dll", "AEAPACTOR.dll", "HALDRIVER.dll", "KAMPACTOR.dll", "SCSPACTOR.dll", "WINMOR.dll", "TELNET.dll", "SOUNDMODEM.dll", "DEDHOSTTNC"}
   Public MapTypes() As Integer = {0, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10}

   Public unMapTypes() As Integer = {0, 7, 8, 1, 2, 3, 4, 5, 6, 9, 10}

   Public ComboBoxSize = New System.Drawing.Size(110, 22)
   Public TextBoxSize = New System.Drawing.Size(50, 22)
   Public SmallTextBoxSize = New System.Drawing.Size(30, 22)
   Public CallTextBoxSize = New System.Drawing.Size(70, 22)
   Public LargeTextBoxSize = New System.Drawing.Size(90, 22)
   Public CheckBoxSize = New System.Drawing.Size(130, 22)
   Public LabelSize = New System.Drawing.Size(65, 17)

   Const RowSpacing = 28
   Const Row1 = 16, Row2 = Row1 + RowSpacing, Row3 = Row2 + RowSpacing, Row4 = Row3 + RowSpacing
   Const Row5 = Row4 + RowSpacing, Row6 = Row5 + RowSpacing, Row7 = Row6 + RowSpacing

   Public Const Col1 = 8, Col2 = 158, Col3 = 308, Col4 = 458
   Public Const Col2a = 208, Col3a = 408

   Public TNCTypes() As String = New String() {"TNC2", "KISS", "PK232/UFQ", "PK232/AA4RE"}

   Delegate Sub SavePortValue(ByVal Port As Integer)

   Public Sub CreateRoutePage()

      Dim i As Integer
      Dim TextBoxSize = New System.Drawing.Size(45, 22)
      Dim SmallTextBoxSize = New System.Drawing.Size(25, 22)

      CurrRow = Row2
      CurrCol = 0

      For i = 0 To 31
         RouteCall(i) = New CallsignTextBox
         RouteQual(i) = New NumTextBox(255)
         RoutePort(i) = New NumTextBox(32)
         RouteMaxFrame(i) = New NumTextBox(7)
         RouteFrack(i) = New NumTextBox(10000)
         RoutePaclen(i) = New NumTextBox(256)
         Routeinp3(i) = New NumTextBox(2)

         RouteCall(i).Location = New System.Drawing.Point(CurrCol + 8, CurrRow)
         RouteQual(i).Location = New System.Drawing.Point(CurrCol + 98, CurrRow)
         RoutePort(i).Location = New System.Drawing.Point(CurrCol + 128, CurrRow)
         RouteMaxFrame(i).Location = New System.Drawing.Point(CurrCol + 158, CurrRow)
         RouteFrack(i).Location = New System.Drawing.Point(CurrCol + 188, CurrRow)
         RoutePaclen(i).Location = New System.Drawing.Point(CurrCol + 238, CurrRow)
         Routeinp3(i).Location = New System.Drawing.Point(CurrCol + 268, CurrRow)

         RouteCall(i).Size = CallTextBoxSize
         RouteQual(i).Size = SmallTextBoxSize
         RoutePort(i).Size = SmallTextBoxSize
         RouteMaxFrame(i).Size = SmallTextBoxSize
         RouteFrack(i).Size = TextBoxSize
         RoutePaclen(i).Size = SmallTextBoxSize
         Routeinp3(i).Size = SmallTextBoxSize

         AddHandler RoutePort(i).Validating, AddressOf ValidatingPort
         AddHandler RoutePort(i).Validated, AddressOf ValidatedSub

         Form1.RoutesTab.Controls.Add(RouteCall(i))
         Form1.RoutesTab.Controls.Add(RouteQual(i))
         Form1.RoutesTab.Controls.Add(RoutePort(i))
         Form1.RoutesTab.Controls.Add(RouteMaxFrame(i))
         Form1.RoutesTab.Controls.Add(RouteFrack(i))
         Form1.RoutesTab.Controls.Add(RoutePaclen(i))
         Form1.RoutesTab.Controls.Add(Routeinp3(i))

         CurrRow = CurrRow + 22

         If i = 15 Then

            CurrRow = Row2
            CurrCol = 300

         End If

      Next

   End Sub

   Public Sub CreateTNCPagez()

      Dim i As Integer
      Dim ComboBoxSize = New System.Drawing.Size(100, 22)
      Dim SmallTextBoxSize = New System.Drawing.Size(30, 22)
      Const col1 = 8, col2 = 48, col3 = 158
      Const col4 = col3 + 45
      Const col5 = col4 + 45
      Const row3 = 80

      For i = 1 To 10
         COMLabel(i) = New Label
         COMLabel(i).Size = New System.Drawing.Size(40, 30)
         '     Form1.BPQ16Tab.Controls.Add(COMLabel(i))

      Next

      COMLabel(1).Text = "COM"
      COMLabel(1).Location = New System.Drawing.Point(col1, row3)
      COMLabel(2).Text = "TYPE"
      COMLabel(2).Location = New System.Drawing.Point(col2, row3)
      COMLabel(3).Text = "APPL" & vbCrLf & "Mask"
      COMLabel(3).Location = New System.Drawing.Point(col3, row3)
      COMLabel(4).Text = "KISS" & vbCrLf & "Mask"
      COMLabel(4).Location = New System.Drawing.Point(col4, row3)
      COMLabel(5).Text = "APPL" & vbCrLf & "Flags"
      COMLabel(5).Location = New System.Drawing.Point(col5, row3)

      COMLabel(6).Text = "COM"
      COMLabel(6).Location = New System.Drawing.Point(col1 + 320, row3)
      COMLabel(7).Text = "TYPE"
      COMLabel(7).Location = New System.Drawing.Point(col2 + 320, row3)
      COMLabel(8).Text = "APPL" & vbCrLf & "Mask"
      COMLabel(8).Location = New System.Drawing.Point(col3 + 320, row3)
      COMLabel(9).Text = "KISS" & vbCrLf & "Mask"
      COMLabel(9).Location = New System.Drawing.Point(col4 + 320, row3)
      COMLabel(10).Text = "APPL" & vbCrLf & "Flags"
      COMLabel(10).Location = New System.Drawing.Point(col5 + 320, row3)

      CurrRow = 120
      CurrCol = 0

      For i = 0 To 15
         COMPort(i) = New NumTextBox(255)
         COMType(i) = New ComboBox
         COMAPPLMask(i) = New NumTextBox(255)
         COMKISSMask(i) = New NumTextBox(65535)
         COMAPPLFlags(i) = New NumTextBox(255)

         COMPort(i).Location = New System.Drawing.Point(CurrCol + col1, CurrRow)
         COMType(i).Location = New System.Drawing.Point(CurrCol + col2, CurrRow)
         COMAPPLMask(i).Location = New System.Drawing.Point(CurrCol + col3, CurrRow)
         COMKISSMask(i).Location = New System.Drawing.Point(CurrCol + col4, CurrRow)
         COMAPPLFlags(i).Location = New System.Drawing.Point(CurrCol + col5, CurrRow)

         COMPort(i).Size = SmallTextBoxSize
         COMType(i).Size = ComboBoxSize
         COMAPPLMask(i).Size = SmallTextBoxSize
         COMKISSMask(i).Size = SmallTextBoxSize
         COMAPPLFlags(i).Size = SmallTextBoxSize

         COMType(i).Items.AddRange(TNCTypes)
         COMType(i).Text = TNCTypes(0)
         '        COMType(i).locked = True

         '   Form1.BPQ16Tab.Controls.Add(COMPort(i))
         '   Form1.BPQ16Tab.Controls.Add(COMType(i))
         '  Form1.BPQ16Tab.Controls.Add(COMAPPLMask(i))
         '  Form1.BPQ16Tab.Controls.Add(COMKISSMask(i))
         '  Form1.BPQ16Tab.Controls.Add(COMAPPLFlags(i))

         CurrRow = CurrRow + 30

         If i = 7 Then

            CurrRow = 120
            CurrCol = 320

         End If

      Next

   End Sub
   Public Sub AddPortTab()

      If NumberOfPorts = 32 Then Exit Sub

      Adding = True ' Suppress triggereing updates to TYPE

      NumberOfPorts = NumberOfPorts + 1

      TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "INTERNAL"          ' Start with minimal (Internal)
      'TxtPortCfg(PORTNUM).Value(NumberOfPorts) = NumberOfPorts
      ' TxtPortCfg(PORTID).Value(NumberOfPorts) = "New Port"
      TxtPortCfg(IOADDR).Value(NumberOfPorts) = 0
      TxtPortCfg(KISSOPTIONS).Value(NumberOfPorts) = 0

      Form1.TabControl2.TabPages.Remove(Form1.NewPortPage)

      PortTab(NumberOfPorts) = New TabPage
      PortTab(NumberOfPorts).Text = "Port " & NumberOfPorts
      PortTab(NumberOfPorts).AutoScroll = True

      Form1.TabControl2.TabPages.Add(PortTab(NumberOfPorts))

      Form1.TabControl2.TabPages.Add(Form1.NewPortPage)

      Form1.TabControl2.SelectedIndex = NumberOfPorts - 1

      '   DrawPortTab(NumberOfPorts)

      Adding = False

   End Sub

   Public Sub SavePortInfo()

      Dim i As Integer

      If CurrentPort = 0 Then Return

      For i = 1 To NumofPortConfigParams

         If TxtPortCfg(i).Checkbox Then

            TxtPortCfg(i).Value(CurrentPort) = TxtPortCfg(i).CfgField.Checked
         Else
            TxtPortCfg(i).Value(CurrentPort) = TxtPortCfg(i).CfgField.text

         End If
      Next

      TxtPortCfg(PORTTYPE).Value(CurrentPort) = TYPE.SelectedItem
      TxtPortCfg(PROTOCOL).Value(CurrentPort) = PROTOCOLBox.SelectedItem
      TxtPortCfg(IOADDR).Value(CurrentPort) = IOADDRBox.Text
      TxtPortCfg(KISSOPTIONS).Value(CurrentPort) = GetKissMode()

      If (HasConfigText) Then

         PortConfig(CurrentPort) = PortParamsBox.Text

      End If

      RigControl(CurrentPort) = RigControlBox.Text
      WL2KReport(CurrentPort) = WL2KReportBox.Text

      If LCase(DLLNAMEBox.Text) = "winmor.dll" Then
         SimpleForm.SaveWINMOR(CurrentPort)
      End If



   End Sub
   Public Sub LoadPortParams(ByVal Port As Integer)

      Dim i As Integer

      If Port = 0 Then Exit Sub

      CurrentPort = Port

      For i = 1 To NumofPortConfigParams

         If TxtPortCfg(i).Checkbox Then

            TxtPortCfg(i).CfgField.Checked = TxtPortCfg(i).Value(CurrentPort)
         Else
            Dim xx As String = TxtPortCfg(i).Value(CurrentPort)
            Try
               TxtPortCfg(i).CfgField.text = xx 'TxtPortCfg(i).Value(CurrentPort)

            Catch ex As Exception

            End Try

         End If

      Next

      TYPE.SelectedItem = TxtPortCfg(PORTTYPE).Value(CurrentPort)
      PROTOCOLBox.SelectedItem = TxtPortCfg(PROTOCOL).Value(CurrentPort)
      If TxtPortCfg(IOADDR).Value(CurrentPort) = "" Then TxtPortCfg(IOADDR).Value(CurrentPort) = "0"
      IOADDRBox.Text = TxtPortCfg(IOADDR).Value(CurrentPort)

      For i = 0 To 4
         KISSOptionsBox.SetItemChecked(i, ((TxtPortCfg(KISSOPTIONS).Value(CurrentPort) >> i) And 1) = 1)
      Next

      ' Clear all pages

      For i = 1 To NumberOfPorts

         Try
            PortTab(i).Controls.Clear()
         Catch ex As Exception

         End Try

      Next

   End Sub

   Public Sub RefreshPortPage()

      Dim PType As String
      Dim DLL As String

      If CurrentPort = 0 Then Exit Sub

      PortTab(CurrentPort).Controls.Clear() '  RemoveAt(0)

      RigControlBox.Text = ""
      WL2KReportBox.Text = ""

      PortTab(CurrentPort).Controls.Add(PORTNUMLabel)
      PortTab(CurrentPort).Controls.Add(PORTNUMBox)
      PortTab(CurrentPort).Controls.Add(PortIDLabel)
      PortTab(CurrentPort).Controls.Add(PortIDBox)
      PortTab(CurrentPort).Controls.Add(TYPELabel)
      PortTab(CurrentPort).Controls.Add(TYPE)

      CurrRow = Row2
      CurrCol = Col2a

      PType = UCase(TxtPortCfg(PORTTYPE).Value(CurrentPort))

      If PType = "DELETED" Then Return

      If PType <> "INTERNAL" And PType <> "EXTERNAL" Then
         DrawLabel(PROTOCOLLabel)
         DrawComboBox(PROTOCOLBox)
      End If

      If PType = "EXTERNAL" Then ' External

         DLL = LCase(TxtPortCfg(DLLNAME).Value(CurrentPort))
         DrawLabel(DLLNAMELabel)
         DrawComboBox(DLLNAMEBox)
         CurrCol = CurrCol + 15

         If DLL = "" Then
            Return
         End If

         If DLL = "bpqtoagw.dll" Or DLL = "bpqvkiss.dll" Then

            CurrRow = Row3
            CurrCol = 8

            DrawLabel(CHANNELLabel)
            DrawBox(CHANNELBox)

            DrawLabel(IOADDRLabel)
            DrawBox(IOADDRBox)

         End If

         If DLL = "winmor.dll" Then

            Dim Port As Integer = CurrentPort

            If MyLabel3(Port) Is Nothing Then
               CreateWINMORControls(Port)
            End If

            CurrRow = Row3
            CurrCol = 8

            MyLabel1(CurrentPort).Location = New System.Drawing.Point(360, 74)
            MyLabel2(CurrentPort).Location = New System.Drawing.Point(161, 74)
            MyLabel3(CurrentPort).Location = New System.Drawing.Point(Col1, 74)
            MyLabel4(CurrentPort).Location = New System.Drawing.Point(270, 74)

            PortTab(CurrentPort).Controls.Add(MyLabel1(CurrentPort))
            PortTab(CurrentPort).Controls.Add(MyLabel2(CurrentPort))
            PortTab(CurrentPort).Controls.Add(MyLabel3(CurrentPort))
            PortTab(CurrentPort).Controls.Add(MyLabel4(CurrentPort))

            PortTab(CurrentPort).Controls.Add(WINMORPTT(CurrentPort))
            PortTab(CurrentPort).Controls.Add(DriveLevel(CurrentPort))
            PortTab(CurrentPort).Controls.Add(WINMORPort(CurrentPort))
            PortTab(CurrentPort).Controls.Add(CWID(CurrentPort))
            PortTab(CurrentPort).Controls.Add(BusyLock(CurrentPort))
            PortTab(CurrentPort).Controls.Add(DebugLog(CurrentPort))
            PortTab(CurrentPort).Controls.Add(BW(CurrentPort))
            PortTab(CurrentPort).Controls.Add(PTTCOMM(CurrentPort))

            DriveLevel(CurrentPort).Location = New System.Drawing.Point(223, 71)
            WINMORPort(CurrentPort).Location = New System.Drawing.Point(118, 71)
            WINMORPTT(CurrentPort).Location = New System.Drawing.Point(390, 71)
            PTTCOMM(CurrentPort).Location = New System.Drawing.Point(475, 71)
            BW(CurrentPort).Location = New System.Drawing.Point(300, 71)
            CWID(CurrentPort).Location = New System.Drawing.Point(198, Row4)
            BusyLock(CurrentPort).Location = New System.Drawing.Point(109, Row4)
            DebugLog(CurrentPort).Location = New System.Drawing.Point(Col1 + 2, Row4)

            CurrRow = Row4
            CurrCol = 300
            DrawLabel(PORTCALLLabel)
            DrawBox(PORTCALLBox)
            '      CurrRow = Row3
            '    CurrCol = 300
            DrawLabel(INTERLOCKLabel)
            DrawBox(INTERLOCKBox)

            PathLabel(Port).Location = New System.Drawing.Point(Col1, Row5 + 3)
            PathBox(Port).Location = New System.Drawing.Point(Col1 + 100, Row5)

            PortTab(CurrentPort).Controls.Add(PathLabel(Port))
            PortTab(CurrentPort).Controls.Add(PathBox(Port))

            CurrRow = Row6
            CurrCol = 8
            DrawLabel(RigControlLabel)
            CurrCol = 40
            DrawBox(RigControlBox)
            RigControlBox.Text = RigControl(Port)

            CurrRow = Row7
            CurrCol = 8
            DrawLabel(WL2KReportLabel)
            CurrCol = 40
            DrawBox(WL2KReportBox)
            WL2KReportBox.Text = WL2KReport(Port)

            TxtPortCfg(SPEED).Value(NumberOfPorts) = ""
            TxtPortCfg(IOADDR).Value(NumberOfPorts) = ""

            ClearDefaultL2Params(Port)

            Dim i As Integer

            For i = 1 To NumofPortConfigParams

               If TxtPortCfg(i).Checkbox Then

                  TxtPortCfg(i).CfgField.Checked = TxtPortCfg(i).Value(CurrentPort)
               Else
                  Dim xx As String = TxtPortCfg(i).Value(CurrentPort)
                  Try
                     TxtPortCfg(i).CfgField.text = xx 'TxtPortCfg(i).Value(CurrentPort)

                  Catch ex As Exception

                  End Try

               End If

            Next

            Exit Sub

         End If

         If InStr(DLL, "pactor") Or InStr(DLL, "hal") Then

            PortTab(CurrentPort).Controls.Add(PortConfigButton)

            CurrRow = Row3
            CurrCol = 8

            DrawLabel(IOADDRLabel)
            DrawBox(IOADDRBox)
            DrawLabel(SPEEDLabel)
            DrawBox(SPEEDBox)

            DrawLabel(PORTCALLLabel)
            DrawBox(PORTCALLBox)
            '      CurrRow = Row3
            '    CurrCol = 300
            DrawLabel(INTERLOCKLabel)
            DrawBox(INTERLOCKBox)

            CurrRow = Row4
            CurrCol = 8

            DrawBox(PortParamsBox)
            PortParamsBox.Text = PortConfig(CurrentPort)
            HasConfigText = True

            ClearDefaultL2Params(CurrentPort)

            Dim i As Integer

            For i = 1 To NumofPortConfigParams

               If TxtPortCfg(i).Checkbox Then

                  TxtPortCfg(i).CfgField.Checked = TxtPortCfg(i).Value(CurrentPort)
               Else
                  Dim xx As String = TxtPortCfg(i).Value(CurrentPort)
                  Try
                     TxtPortCfg(i).CfgField.text = xx 'TxtPortCfg(i).Value(CurrentPort)

                  Catch ex As Exception

                  End Try

               End If

            Next

            Exit Sub

         End If

         If DLL.StartsWith("telnet") Then

            CurrRow = Row3
            CurrCol = 8

            DrawBox(PortParamsBox)
            PortParamsBox.Text = PortConfig(CurrentPort)
            HasConfigText = True

            ClearDefaultL2Params(CurrentPort)

            PortTab(CurrentPort).Controls.Add(PortConfigButton)

            Exit Sub

         End If

         If DLL = "bpqaxip.dll" Or DLL = "bpqether.dll" Or (PortConfig(CurrentPort) IsNot Nothing AndAlso PortConfig(CurrentPort).Length) Then
            PortTab(CurrentPort).Controls.Add(PortConfigButton)
         End If

      End If

      If TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "INTERNAL" And TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "EXTERNAL" Then ' External 

         If TxtPortCfg(PROTOCOL).Value(CurrentPort) = "KISS" Then

            KISSOptionsBox.Location = New System.Drawing.Point(Col4 + 70, Row1)
            KISSOPTIONSLabel.Location = New System.Drawing.Point(Col4, Row1 + 10)
            PortTab(CurrentPort).Controls.Add(KISSOptionsBox)
            PortTab(CurrentPort).Controls.Add(KISSOPTIONSLabel)

         End If

         DrawLabel(CHANNELLabel)
         DrawBox(CHANNELBox)


         DrawLabel(IOADDRLabel)
         DrawBox(IOADDRBox)
         DrawLabel(INTLEVELLabel)
         DrawBox(INTLEVELBox)
         DrawLabel(SPEEDLabel)
         DrawBox(SPEEDBox)

         DrawLabel(TXDELAYLabel)
         DrawBox(TXDELAYBox)
         DrawLabel(TXTAILLabel)
         DrawBox(TXTAILBox)
         DrawLabel(SLOTTIMELabel)
         DrawBox(SLOTTIMEBox)
         DrawLabel(PERSISTLabel)
         DrawBox(PERSISTBox)

         DrawcheckBox(FULLDUPBox)


      End If

      If TxtPortCfg(PORTTYPE).Value(CurrentPort) = "EXTERNAL" Then ' External 
         PortTab(CurrentPort).Controls.Add(DLLNAMELabel)
         PortTab(CurrentPort).Controls.Add(DLLNAMEBox)
      End If

      If TxtPortCfg(PORTTYPE).Value(CurrentPort) > "2" Then     ' HDLC Cards

         '   DrawcheckBox(SOFTDCDBox)
         '  DrawLabel(CWIDLabel)
         ' DrawBox(CWIDBox)
         'DrawcheckBox(CWIDTYPEBox)

      End If

      If CurrCol <> 8 Then
         CurrCol = 8
         CurrRow = CurrRow + RowSpacing
      End If

      DrawLabel(PORTCALLLabel)
      DrawBox(PORTCALLBox)
      DrawLabel(PORTALIASLabel)
      DrawBox(PORTALIASBox)
      DrawLabel(PORTALIAS2Label)
      DrawBox(PORTALIAS2Box)
      DrawLabel(BCALLLabel)
      DrawBox(BCALLBox)

      DrawLabel(FRACKLabel)
      DrawBox(FRACKBox)
      DrawLabel(RESPTIMELabel)
      DrawBox(RESPTIMEBox)

      DrawLabel(RETRIESLabel)
      DrawBox(RETRIESBox)
      DrawLabel(PACLENLabel)
      DrawBox(PACLENBox)

      DrawLabel(MAXFRAMELabel)
      DrawBox(MAXFRAMEBox)

      DrawLabel(QUALITYLabel)
      DrawBox(QUALITYBox)
      DrawLabel(MINQUALLabel)
      DrawBox(MINQUALBox)

      DrawLabel(NODESPACLENLabel)
      DrawBox(NODESPACLENBox)


      DrawLabel(QUALADJUSTLabel)
      DrawBox(QUALADJUSTBox)
      DrawLabel(DIGIFLAGLabel)
      DrawBox(DIGIFLAGBox)
      DrawLabel(DIGIPORTLabel)
      DrawBox(DIGIPORTBox)
      DrawLabel(DIGIMASKLabel)
      DrawBox(DIGIMASKBox)
      DrawcheckBox(ALIAS_IS_BBSBox)
      DrawcheckBox(L3ONLYBox)
      DrawcheckBox(MHEARDBox)
      DrawLabel(USERSLabel)
      DrawBox(USERSBox)

      DrawLabel(INTERLOCKLabel)
      DrawBox(INTERLOCKBox)
      DrawLabel(TXPORTLabel)
      DrawBox(TXPORTBox)
      DrawLabel(MAXDIGISLabel)
      DrawBox(MAXDIGISBox)

      DrawcheckBox(BBSFLAGBox)
      DrawcheckBox(NoKeepalivesBox)

      If CurrCol <> 8 Then
         CurrCol = 8
         CurrRow = CurrRow + RowSpacing
      End If

      DrawLabel(VALIDCALLSLabel)
      DrawBox(VALIDCALLSBox)

      If CurrCol <> 8 Then
         CurrCol = 8
         CurrRow = CurrRow + RowSpacing
      End If

      DrawLabel(UNPROTOLabel)
      DrawBox(UNPROTOBox)

   End Sub

   Private Sub PTTChanged(ByVal sender As Object, ByVal e As System.EventArgs)
      Dim Port As Integer = sender.Name

      If sender.selectedindex = 0 Then
         PTTCOMM(Port).Visible = False
      Else
         PTTCOMM(Port).Visible = True
         If PTTCOMM(Port).SelectedIndex = -1 Then PTTCOMM(Port).SelectedIndex = 0
      End If

   End Sub

   Public Sub InitLabels()


      PORTNUMLabel.Text = "Portnum"
      PORTNUMLabel.Location = New System.Drawing.Point(Col1, Row1 + 3)
      PORTNUMLabel.Size = LabelSize

      PORTNUMBox.Location = New System.Drawing.Point(Col1 + 70, Row1)
      PORTNUMBox.Size = SmallTextBoxSize

      PortIDLabel.Text = "ID"
      PortIDLabel.Location = New System.Drawing.Point(Col1 + 120, Row1 + 3)
      PortIDLabel.Size = New System.Drawing.Size(30, 17)
      PortIDBox.Location = New System.Drawing.Point(Col1 + 150, Row1)
      PortIDBox.Size = New System.Drawing.Size(240, 22)


      TYPELabel.Text = "Type"
      TYPELabel.Location = New System.Drawing.Point(Col1, Row2 + 3)
      TYPELabel.Size = LabelSize

      TYPE.Location = New System.Drawing.Point(Col1 + 70, Row2)
      TYPE.Size = ComboBoxSize
      TYPE.Items.AddRange(Types)
      TYPE.DropDownStyle = ComboBoxStyle.DropDownList

      PROTOCOLBox = New ComboBox
      PROTOCOLBox.Location = New System.Drawing.Point(Col2a + 70, Row2)
      PROTOCOLBox.Size = ComboBoxSize

      PROTOCOLBox.Items.AddRange(Protos)
      PROTOCOLBox.DropDownStyle = ComboBoxStyle.DropDownList


      PROTOCOLLabel.Text = "Protocol"
      PROTOCOLLabel.Location = New System.Drawing.Point(Col2a, Row2 + 3)
      PROTOCOLLabel.Size = LabelSize

      DLLNAMELabel.Text = "DLLName"
      DLLNAMELabel.Location = New System.Drawing.Point(Col3a, Row2 + 3)
      DLLNAMELabel.Size = New System.Drawing.Size(70, 17)

      AddHandler PortConfigButton.Click, AddressOf PortConfigButton_Click
      PortConfigButton.Location = New System.Drawing.Point(Col4, Row2)
      PortConfigButton.Text = "Config Driver"

      KISSOptionsBox.Location = New System.Drawing.Point(Col4 + 70, Row1)
      KISSOPTIONSLabel.Location = New System.Drawing.Point(Col4, Row1 + 10)


      DLLNAMEBox.Location = New System.Drawing.Point(Col3a + 70, Row2)
      DLLNAMEBox.Size = ComboBoxSize
      DLLNAMEBox.Sorted = True
      DLLNAMEBox.Items.AddRange(DLLNames)
      DLLNAMEBox.DropDownStyle = ComboBoxStyle.DropDown

      IOADDRLabel.Text = "IOAddr"
      IOADDRLabel.Size = New System.Drawing.Size(70, 17)
      IOADDRBox.Size = TextBoxSize
      Form1.ToolTip1.SetToolTip(IOADDRBox, "COM port number for async ports. TCP port for AGWPE ports. IO Port address for HDLC Cards")

      INTLEVELLabel.Text = "IntLevel"
      INTLEVELLabel.Size = New System.Drawing.Size(70, 17)
      INTLEVELBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(INTLEVELBox, "Interrupt number for HDLC Cards")

      SPEEDLabel.Text = "Speed"
      SPEEDLabel.Size = LabelSize
      SPEEDBox.Size = TextBoxSize
      Form1.ToolTip1.SetToolTip(SPEEDBox, "Async port speed for KISS. Radio port speed for HDLC cards")

      CHANNELLabel.Text = "Channel"
      CHANNELLabel.Size = LabelSize
      CHANNELBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(CHANNELBox, "Port on Multiport TNCs. A Letter, A-Z")

      BBSFLAGBox.Text = "BBS Banned"
      BBSFLAGBox.CheckAlign = ContentAlignment.MiddleRight
      BBSFLAGBox.Size = CheckBoxSize

      QUALITYLabel.Text = "Quality"
      QUALITYLabel.Size = LabelSize
      QUALITYBox.Size = SmallTextBoxSize

      MAXFRAMELabel.Text = "MaxFrame"
      MAXFRAMELabel.Size = LabelSize
      MAXFRAMEBox.Size = SmallTextBoxSize

      TXDELAYLabel.Text = "TXDelay"
      TXDELAYLabel.Size = LabelSize
      TXDELAYBox.Size = TextBoxSize

      SLOTTIMELabel.Text = "Slottime"
      SLOTTIMELabel.Size = LabelSize
      SLOTTIMEBox.Size = SmallTextBoxSize

      PERSISTLabel.Text = "Persist"
      PERSISTLabel.Size = LabelSize
      PERSISTBox.Size = SmallTextBoxSize

      FULLDUPBox.Text = "FullDup"
      FULLDUPBox.CheckAlign = ContentAlignment.MiddleRight
      FULLDUPBox.Size = CheckBoxSize

      SOFTDCDBox.Text = "SoftDCD"
      SOFTDCDBox.CheckAlign = ContentAlignment.MiddleRight
      SOFTDCDBox.Size = CheckBoxSize

      FRACKLabel.Text = "Frack"
      FRACKLabel.Size = LabelSize
      FRACKBox.Size = TextBoxSize

      RESPTIMELabel.Text = "Resptime"
      RESPTIMELabel.Size = LabelSize
      RESPTIMEBox.Size = TextBoxSize

      RETRIESLabel.Text = "Retries"
      RETRIESLabel.Size = LabelSize
      RETRIESBox.Size = SmallTextBoxSize

      PACLENLabel.Text = "Paclen"
      PACLENLabel.Size = LabelSize
      PACLENBox.Size = SmallTextBoxSize

      CWIDLabel.Text = "CWID"
      CWIDLabel.Size = LabelSize
      CWIDBox.Size = CallTextBoxSize

      PORTCALLLabel.Text = "Port Call"
      PORTCALLLabel.Size = LabelSize
      PORTCALLBox.Size = CallTextBoxSize

      PORTALIASLabel.Text = "Port Alias"
      PORTALIASLabel.Size = LabelSize
      PORTALIASBox.Size = CallTextBoxSize

      VALIDCALLSLabel.Text = "ValidCalls"
      VALIDCALLSLabel.Size = LabelSize
      VALIDCALLSBox.Size = New System.Drawing.Size(460, 17)

      QUALADJUSTLabel.Text = "QualAdjust"
      QUALADJUSTLabel.Size = LabelSize
      QUALADJUSTBox.Size = SmallTextBoxSize

      DIGIFLAGLabel.Text = "DigiFlag"
      DIGIFLAGLabel.Size = LabelSize
      DIGIFLAGBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(DIGIFLAGBox, "0 - Digipeat disabled, 1 - Digipeat all frame types, 255 - digipeat UI frames only")


      DIGIPORTLabel.Text = "Digiport"
      DIGIPORTLabel.Size = LabelSize
      DIGIPORTBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(DIGIPORTBox, "Port on which frames received here will be transmitted. Zero means tranmit on receiving port")

      DIGIMASKLabel.Text = "Digimask"
      DIGIMASKLabel.Size = LabelSize
      DIGIMASKBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(DIGIMASKBox, "Used for digi'ing UI frames to more than one port")

      USERSLabel.Text = "Users"
      USERSLabel.Size = LabelSize
      USERSBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(USERSBox, "Used to limit number of connects on this port")

      UNPROTOLabel.Text = "Unproto"
      UNPROTOLabel.Size = LabelSize
      UNPROTOBox.Size = New System.Drawing.Size(460, 17)
      Form1.ToolTip1.SetToolTip(UNPROTOBox, "Dest address for beacons on this port. If not set, beacons are not sent on this port.")

      TXTAILLabel.Text = "TxTail"
      TXTAILLabel.Size = LabelSize
      TXTAILBox.Size = TextBoxSize

      ALIAS_IS_BBSBox.Text = "Alias is BBS"
      ALIAS_IS_BBSBox.CheckAlign = ContentAlignment.MiddleRight
      ALIAS_IS_BBSBox.Size = CheckBoxSize

      L3ONLYBox.Text = "L3Only"
      L3ONLYBox.CheckAlign = ContentAlignment.MiddleRight
      L3ONLYBox.Size = CheckBoxSize
      Form1.ToolTip1.SetToolTip(L3ONLYBox, "If set only Node-Node connections are allowed on this port.")

      KISSOPTIONSLabel.Text = "KissOptions"
      KISSOPTIONSLabel.Size = LabelSize
      '       KISSOptionsBox.Size = TextBoxSize

      KISSOptionsBox.CheckOnClick = True
      KISSOptionsBox.FormattingEnabled = True
      KISSOptionsBox.Items.AddRange(New Object() {"Checksum", "Polled", "ACKMode", "Slave", "D700"})
      KISSOptionsBox.Name = "KISSOptionsBox"
      KISSOptionsBox.Size = New System.Drawing.Size(112, 55)
      KISSOptionsBox.TabIndex = 37

      INTERLOCKLabel.Text = "Interlock"
      INTERLOCKLabel.Size = LabelSize
      INTERLOCKBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(INTERLOCKBox, "Fox HDLC ports, ports with same value will not transmit at the same time. For WINMOR/Pactor, ports with the same value share a radio, so only one may be used at a time.")

      NODESPACLENLabel.Text = "NodePacLen"
      NODESPACLENLabel.Size = New System.Drawing.Size(70, 17)
      NODESPACLENBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(NODESPACLENBox, "PACLEN for NODES broadcasts")

      TXPORTLabel.Text = "TxPort"
      TXPORTLabel.Size = LabelSize
      TXPORTBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(TXPORTBox, "TX frames will be redirected to this port.")

      MHEARDBox.Text = "Suppress MHeard"
      MHEARDBox.CheckAlign = ContentAlignment.MiddleRight
      MHEARDBox.Size = CheckBoxSize
      Form1.ToolTip1.SetToolTip(MHEARDBox, "Set to disble HMeard processing on this port")

      CWIDTYPEBox.Text = "CWID OnOff"
      CWIDTYPEBox.CheckAlign = ContentAlignment.MiddleRight
      CWIDTYPEBox.Size = CheckBoxSize
      Form1.ToolTip1.SetToolTip(CWIDTYPEBox, "Set for On/Off keying of CWID" & vbCrLf & "(Default is FSK)")

      MINQUALLabel.Text = "MinQual"
      MINQUALLabel.Size = LabelSize
      MINQUALBox.Size = SmallTextBoxSize
      Form1.ToolTip1.SetToolTip(MINQUALBox, "Minimum quality a node must have to be included in a NODES broadcast.")

      MAXDIGISLabel.Text = "MaxDigis"
      MAXDIGISLabel.Size = LabelSize
      MAXDIGISBox.Size = SmallTextBoxSize

      PORTALIAS2Label.Text = "Port Alias 2"
      PORTALIAS2Label.Size = New System.Drawing.Size(70, 17)
      PORTALIAS2Box.Size = TextBoxSize

      BCALLLabel.Text = "BCALL"
      BCALLLabel.Size = LabelSize
      BCALLBox.Size = CallTextBoxSize
      Form1.ToolTip1.SetToolTip(BCALLBox, "Source call for beacons on this port.")

      PortParamsBox.Size = New System.Drawing.Size(600, 300)
      PortParamsBox.ScrollBars = ScrollBars.Both
      PortParamsBox.WordWrap = False
      PortParamsBox.Multiline = True

      WL2KReportLabel.Size = New System.Drawing.Size(80, 17)
      WL2KReportLabel.Text = "WL2K Param"
      WL2KReportBox.Size = New System.Drawing.Size(500, 13)

      RigControlLabel.Text = "RigControl"
      RigControlLabel.Size = New System.Drawing.Size(80, 17)
      RigControlBox.Size = New System.Drawing.Size(500, 13)

      NoKeepalivesBox.Text = "No Keepalives"
      NoKeepalivesBox.CheckAlign = ContentAlignment.MiddleRight
      NoKeepalivesBox.Size = CheckBoxSize
      Form1.ToolTip1.SetToolTip(NoKeepalivesBox, "If set, node will not attempt to keep links to neighbours open when not it use.")


   End Sub

   Public Sub InitHandlers()

      AddHandler TYPE.SelectedIndexChanged, AddressOf TYPESelectedIndexChanged
      AddHandler PROTOCOLBox.SelectedIndexChanged, AddressOf PROTOCOLSelectedIndexChanged
      AddHandler DLLNAMEBox.SelectedIndexChanged, AddressOf DLLNameSelectedIndexChanged

   End Sub


   Private Sub TYPESelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

      If TYPE.SelectedItem = "DELETED" Then

         DeletedPort(CurrentPort) = True
      Else
         DeletedPort(CurrentPort) = False


         'Dim i As Integer, SavePort As Integer = CurrentPort

         'Form1.TabControl2.TabPages.RemoveAt(NumberOfPorts - 1)

         'Dim p As Integer

         'For p = 1 To NumofPortConfigParams

         'ParamAtLine(TxtPortCfg(p).LineNo(SavePort)) = -1

         'Next

         'CurrentPort = SavePort

         'For i = CurrentPort To NumberOfPorts
         'CopyPort(i, i + 1)
         'Next

         'ClearPort(NumberOfPorts)
         'NumberOfPorts = NumberOfPorts - 1
         'If CurrentPort > NumberOfPorts Then CurrentPort = 1
         'LoadPortParams(CurrentPort)
         'RefreshPortPage()
         'Return

      End If

      TxtPortCfg(PORTTYPE).Value(CurrentPort) = TYPE.SelectedItem
      If Adding = False Then RefreshPortPage()

   End Sub
   Private Sub PROTOCOLSelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

      TxtPortCfg(PROTOCOL).Value(CurrentPort) = PROTOCOLBox.SelectedItem
      If Adding = False Then RefreshPortPage()

   End Sub

   Private Sub DLLNameSelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

      TxtPortCfg(DLLNAME).Value(CurrentPort) = DLLNAMEBox.SelectedItem

      '     If MyLabel3(CurrentPort) Is Nothing Then

      '    CreateWINMORControls(CurrentPort)

      '   End If

      If Adding = False Then RefreshPortPage()

   End Sub




   Public Sub DrawLabel(ByVal obj As Label)

      obj.Location = New System.Drawing.Point(CurrCol, CurrRow + 3)

      PortTab(CurrentPort).Controls.Add(obj)

   End Sub

   Public Sub DrawBox(ByVal obj As TextBox)

      obj.Location = New System.Drawing.Point(CurrCol + 70, CurrRow)

      PortTab(CurrentPort).Controls.Add(obj)

      CurrCol = CurrCol + 150
      If CurrCol > 550 Then

         CurrCol = 8
         CurrRow = CurrRow + RowSpacing

      End If

   End Sub

   Public Sub DrawComboBox(ByVal obj As ComboBox)

      obj.Location = New System.Drawing.Point(CurrCol + 70, CurrRow)

      PortTab(CurrentPort).Controls.Add(obj)

      CurrCol = CurrCol + 200
      If CurrCol > 500 Then

         CurrCol = 8
         CurrRow = CurrRow + RowSpacing

      End If

   End Sub
   Public Sub DrawcheckBox(ByVal obj As CheckBox)

      obj.Location = New System.Drawing.Point(CurrCol, CurrRow)

      PortTab(CurrentPort).Controls.Add(obj)

      CurrCol = CurrCol + 150
      If CurrCol > 500 Then

         CurrCol = 8
         CurrRow = CurrRow + RowSpacing

      End If

   End Sub


   Public Sub ValidatedSub(ByVal sender As Object, ByVal e As System.EventArgs)

      Form1.ErrorProvider1.SetError(sender, "")

   End Sub

   Public Sub ValidatingCombo(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      If sender.name = PortBeingValidated.ToString Then

         If sender.selectedindex = -1 Then

            Form1.ErrorProvider1.SetError(sender, "A value must be selected")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

         End If
      End If
   End Sub

   Public Sub OptionalCallValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      sender.Text = LTrim(RTrim(UCase(sender.Text)))

      If sender.Text.Length > 9 Then

         Form1.ErrorProvider1.SetError(sender, "Too Long (max 9 chars")
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub
   Public Sub AliasValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      sender.Text = LTrim(RTrim(UCase(sender.Text)))

      If sender.Text.Length > 6 Then

         Form1.ErrorProvider1.SetError(sender, "Too Long (max 6 chars")
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub
   Public Sub ApplValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      sender.Text = LTrim(RTrim(UCase(sender.Text)))

      If sender.Text.Length > 16 Then

         Form1.ErrorProvider1.SetError(sender, "Too Long (max 16 chars")
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub

   Private Sub ValidatingPort(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      Dim row As Integer

      Dim c As TextBox, port As Integer, minport As Integer
      c = DirectCast(sender, TextBox)

      For row = 0 To 31

         If RoutePort(row) Is c Then Exit For

      Next

      If RouteCall(row).Text = String.Empty Then minport = 0 Else minport = 1

      port = Val(c.Text)

      If port < minport Or port > NumberOfPorts Then

         Form1.ErrorProvider1.SetError(c, "Must be between " & minport & " and  " & NumberOfPorts)
         e.Cancel = True
         c.Select(0, c.Text.Length)

      End If

   End Sub

   Public Sub ValidatingNumber(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      Dim port As Integer

      port = Val(sender.Text)
      sender.Text = port

      If port < 0 Or port > sender.Max Then


         Form1.ErrorProvider1.SetError(sender, "Must be between 0 and " & sender.Max)
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub

   Public Sub ValidatingNumberorNull(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      Dim port As Integer

      If LTrim(RTrim(sender.text)) = "" Then Exit Sub

      port = Val(sender.Text)
      sender.Text = port

      If port < 0 Or port > sender.Max Then


         Form1.ErrorProvider1.SetError(sender, "Must be between 0 and " & sender.Max)
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub
   Public Sub ValidatingNumberNonZero(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      If sender.name = PortBeingValidated.ToString Then

         Dim port As Integer

         port = Val(sender.Text)
         sender.Text = port

         If port < 1 Or port > sender.Max Then


            Form1.ErrorProvider1.SetError(sender, "Must be between 1 and " & sender.Max)
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

         End If
      End If

   End Sub

   Public Sub SetupAppl()

      Dim i As Integer, Row As Integer, QualCol As Integer, TypeCol As Integer
      Dim LabelCol As Integer, NameCol As Integer, CallCol As Integer, AliasCol As Integer
      Dim RowIncr As Integer, ApplAliasCol

      '        Dim factor As Integer = 120 / 96
      Dim factor As Integer = 1


      RowIncr = 28 * factor
      Row = 140 - RowIncr '151 - RowIncr
      LabelCol = 10
      NameCol = 60
      ApplAliasCol = 170
      CallCol = 280
      AliasCol = 370
      QualCol = 465 * factor
      TypeCol = 525 * factor

      Form1.ApplNameLabel.Location = New System.Drawing.Point(NameCol, Row)
      Form1.ApplCmdAliasLabel.Location = New System.Drawing.Point(ApplAliasCol, Row)
      Form1.ApplCallLabel.Location = New System.Drawing.Point(CallCol, Row)
      Form1.ApplAliasLabel.Location = New System.Drawing.Point(AliasCol, Row)
      Form1.ApplQualLabel.Location = New System.Drawing.Point(QualCol, Row)
      Form1.ApplTypeLabel.Location = New System.Drawing.Point(TypeCol, Row)

      For i = 1 To 32

         ApplLabel(i) = New Label
         ApplLabel(i).Location = New System.Drawing.Point(LabelCol, Row + 3 + RowIncr * i)
         ApplLabel(i).Size = New System.Drawing.Size(45, 22)
         ApplLabel(i).Text = "Appl " & i
         Form1.ApplsTab.Controls.Add(ApplLabel(i))


         ApplName(i) = New BPQCFG.ApplNameTextBox

         ApplName(i).Location = New System.Drawing.Point(NameCol, Row + RowIncr * i)
         ApplName(i).Size = New System.Drawing.Size(100, 22)
         ApplName(i).Text = ""

         Form1.ApplsTab.Controls.Add(ApplName(i))

         ApplCmdAlias(i) = New BPQCFG.ApplNameTextBox

         ApplCmdAlias(i).Location = New System.Drawing.Point(ApplAliasCol, Row + RowIncr * i)
         ApplCmdAlias(i).Size = New System.Drawing.Size(100, 22)
         ApplCmdAlias(i).Text = ""

         Form1.ApplsTab.Controls.Add(ApplCmdAlias(i))

         ApplCall(i) = New BPQCFG.CallsignTextBox

         ApplCall(i).Location = New System.Drawing.Point(CallCol, Row + RowIncr * i)
         ApplCall(i).Size = CallTextBoxSize
         ApplCall(i).Text = ""

         Form1.ApplsTab.Controls.Add(ApplCall(i))

         ApplAlias(i) = New BPQCFG.AliasTextBox

         ApplAlias(i).Location = New System.Drawing.Point(AliasCol, Row + RowIncr * i)
         ApplAlias(i).Size = CallTextBoxSize
         ApplAlias(i).Text = ""

         Form1.ApplsTab.Controls.Add(ApplAlias(i))

         ApplQual(i) = New BPQCFG.NumTextBox(255)

         ApplQual(i).Location = New System.Drawing.Point(QualCol, Row + RowIncr * i)
         ApplQual(i).Size = New System.Drawing.Size(39, 22)
         ApplQual(i).Text = "0"

         Form1.ApplsTab.Controls.Add(ApplQual(i))

         ApplType(i) = New System.Windows.Forms.ComboBox

         ApplType(i).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
         ApplType(i).FormattingEnabled = True
         ApplType(i).Items.AddRange(New Object() {"BPQ32", "AGWPE", "DED Host"})
         ApplType(i).Location = New System.Drawing.Point(TypeCol, Row + RowIncr * i)
         ApplType(i).Size = New System.Drawing.Size(74, 21)

         AddHandler ApplType(i).SelectedIndexChanged, AddressOf ApplTypeChanged
         AddHandler ApplType(i).Validated, AddressOf ValidatedSub

         Form1.ApplsTab.Controls.Add(ApplType(i))
         Debug.Print(Row + RowIncr * i)
      Next

      Dim mask As Integer

      mask = OriginalAGWAppl

      For i = 1 To 8

         If (mask And 1) = 1 Then ApplType(i).SelectedIndex = 1
         mask = mask >> 1

      Next

   End Sub


   Public Sub MultiLineValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

      '      sender.Text = LTrim(RTrim(UCase(sender.Text)))

      If sender.Text.Length > sender.maxlen Then

         Form1.ErrorProvider1.SetError(sender, "Too Long (max " & sender.maxlen & " chars")
         e.Cancel = True
         sender.Select(0, sender.Text.Length)

      End If

   End Sub

   Sub ApplTypeChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

      Dim i As Integer

      AGWAppl = 0
      For i = 8 To 1 Step -1

         AGWAppl = AGWAppl << 1
         If ApplType(i).SelectedIndex = 1 Then AGWAppl = AGWAppl + 1

      Next

      Form1.AGWApplVal.Text = "0x" & Hex(AGWAppl)

   End Sub
   Public Sub SetAGWAppl()

      My.Computer.Registry.SetValue _
      (RegTree & "\Software\G8BPQ\BPQ32\AGWtoBPQ", "ApplMask", AGWAppl)

   End Sub


   Function GetKissMode()

      Dim indexChecked As Integer
      Dim KissMode As Integer = 0

      For Each indexChecked In KISSOptionsBox.CheckedIndices
         ' The indexChecked variable contains the index of the item.
         KissMode = KissMode + (1 << indexChecked)
      Next

      GetKissMode = KissMode

   End Function
   Public Function DEDApplChanged() As Boolean

      Dim i As Integer

      For i = 1 To 8

         If ((DEDMask(i) >> (i - 1)) And 1) = 1 And ApplType(i).SelectedIndex <> 2 Then
            DEDApplChanged = True
            Exit Function
         End If

         If ((DEDMask(i) >> (i - 1)) And 1) <> 1 And ApplType(i).SelectedIndex = 2 Then
            DEDApplChanged = True
            Exit Function
         End If

      Next

      DEDApplChanged = False

   End Function

   Public Sub SetDEDAppl()

      Dim i As Integer

      Try
         If osInfo.Version.Major >= 6 Then
            My.Computer.Registry.CurrentUser.DeleteSubKey("Software\G8BPQ\BPQ32\DEDAppl")
         Else
            My.Computer.Registry.LocalMachine.DeleteSubKey("Software\G8BPQ\BPQ32\DEDAppl")
         End If

      Catch ex As Exception
      End Try

      For i = 1 To 8

         If ApplType(i).SelectedIndex = 2 Then

            My.Computer.Registry.SetValue _
                (RegTree & "\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(i).Text, 1 << (i - 1))
         End If

      Next

   End Sub
   Private Sub PortConfigButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)


      PortConfigDlg.ConfigText.Text = PortConfig(CurrentPort)

      PortConfigDlg.Visible = True
      PortConfigDlg.Activate()

   End Sub

   Public Sub InitTextCfgKeywords()

      Dim i As Integer

      TxtCfg(3).Key = "PASSWORD"
      TxtCfg(3).CfgField = Form1.PasswordBox

      TxtCfg(4).Key = "LOCATOR"
      TxtCfg(4).CfgField = Form1.LocatorBox

      TxtCfg(5).Key = "AUTOSAVE"
      TxtCfg(5).CfgField = Form1.AutoSaveBox
      TxtCfg(5).Checkbox = True

      TxtCfg(6).Key = "NODECALL"
      TxtCfg(6).CfgField = Form1.NodeCallBox

      TxtCfg(7).Key = "NODEALIAS"
      TxtCfg(7).CfgField = Form1.NodeAliasBox

      TxtCfg(BBSCALL).Key = "BBSCALL"
      TxtCfg(BBSCALL).CfgField = ApplCall(1)

      TxtCfg(BBSALIAS).Key = "BBSALIAS"
      TxtCfg(BBSALIAS).CfgField = ApplAlias(1)

      TxtCfg(10).Key = "IDMSG:"
      TxtCfg(10).CfgField = Form1.IDMsgBox
      TxtCfg(10).MultiLineText = True

      TxtCfg(11).Key = "INFOMSG:"
      TxtCfg(11).CfgField = Form1.InfoMsgBox
      TxtCfg(11).MultiLineText = True

      TxtCfg(12).Key = "BTEXT:"
      TxtCfg(12).CfgField = Form1.BTEXTBox
      TxtCfg(12).MultiLineText = True

      TxtCfg(13).Key = "CTEXT:"
      TxtCfg(13).CfgField = Form1.CTEXTBox
      TxtCfg(13).MultiLineText = True

      TxtCfg(14).Key = "FULL_CTEXT"
      TxtCfg(14).CfgField = Form1.FullCTEXT
      TxtCfg(14).Checkbox = True

      TxtCfg(15).Key = "OBSINIT"
      TxtCfg(15).CfgField = Form1.ObsInitBox

      TxtCfg(16).Key = "OBSMIN"
      TxtCfg(16).CfgField = Form1.ObsMinBox

      TxtCfg(17).Key = "NODESINTERVAL"
      TxtCfg(17).CfgField = Form1.NodesIntervalBox

      TxtCfg(18).Key = "IDINTERVAL"
      TxtCfg(18).CfgField = Form1.IDIntervalBox

      TxtCfg(19).Key = "BTINTERVAL"
      TxtCfg(19).CfgField = Form1.BTIntervalBox

      TxtCfg(20).Key = "L3TIMETOLIVE"
      TxtCfg(20).CfgField = Form1.L3TTLBox

      TxtCfg(21).Key = "L4RETRIES"
      TxtCfg(21).CfgField = Form1.L4RetriesBox

      TxtCfg(22).Key = "L4TIMEOUT"
      TxtCfg(22).CfgField = Form1.L4TimeOutBox

      TxtCfg(23).Key = "L4DELAY"
      TxtCfg(23).CfgField = Form1.L4DelayBox

      TxtCfg(24).Key = "L4WINDOW"
      TxtCfg(24).CfgField = Form1.L4WindowBox

      TxtCfg(25).Key = "MAXLINKS"
      TxtCfg(25).CfgField = Form1.MaxLinksBox

      TxtCfg(26).Key = "MAXNODES"
      TxtCfg(26).CfgField = Form1.MaxNodesBox

      TxtCfg(27).Key = "MAXROUTES"
      TxtCfg(27).CfgField = Form1.MaxRoutesBox

      TxtCfg(28).Key = "MAXCIRCUITS"
      TxtCfg(28).CfgField = Form1.MaxCircuitsBox

      TxtCfg(29).Key = "MINQUAL"
      TxtCfg(29).CfgField = Form1.MinQualBox

      TxtCfg(BBSQUAL).Key = "BBSQUAL"
      TxtCfg(BBSQUAL).CfgField = ApplQual(1)

      TxtCfg(31).Key = "BUFFERS"
      TxtCfg(31).CfgField = Form1.BuffersBox

      TxtCfg(32).Key = "PACLEN"
      TxtCfg(32).CfgField = Form1.PACLENBox

      TxtCfg(33).Key = ""
      '   TxtCfg(33).CfgField = Form1.TransDelayBox

      TxtCfg(34).Key = "T3"
      TxtCfg(34).CfgField = Form1.T3Box

      TxtCfg(35).Key = "IDLETIME"
      TxtCfg(35).CfgField = Form1.IdleTimeBox

      TxtCfg(36).Key = "BBS"
      TxtCfg(36).CfgField = Form1.BBSBox
      TxtCfg(36).Checkbox = True

      TxtCfg(37).Key = "NODE"
      TxtCfg(37).CfgField = Form1.NodeBox
      TxtCfg(37).Checkbox = True

      TxtCfg(38).Key = "HIDENODES"
      TxtCfg(38).CfgField = Form1.HideNodesBox
      TxtCfg(38).Checkbox = True

      TxtCfg(39).Key = "ENABLE_LINKED"
      TxtCfg(39).CfgField = Form1.EnableLinked

      TxtCfg(40).Key = "TNCPORT"
      TxtCfg(41).Key = "PORT"
      TxtCfg(42).Key = "ENDPORT"
      TxtCfg(43).Key = "ROUTES:"
      TxtCfg(44).Key = "APPLICATIONS"

      TxtCfg(45).Key = "APPL1CALL"
      TxtCfg(45).CfgField = ApplCall(1)

      TxtCfg(46).Key = "APPL1ALIAS"
      TxtCfg(46).CfgField = ApplAlias(1)

      TxtCfg(47).Key = "APPL1QUAL"
      TxtCfg(47).CfgField = ApplQual(1)

      TxtCfg(48).Key = "APPL2CALL"
      TxtCfg(48).CfgField = ApplCall(2)

      TxtCfg(49).Key = "APPL2ALIAS"
      TxtCfg(49).CfgField = ApplAlias(2)

      TxtCfg(50).Key = "APPL2QUAL"
      TxtCfg(50).CfgField = ApplQual(2)

      TxtCfg(51).Key = "APPL3CALL"
      TxtCfg(51).CfgField = ApplCall(3)

      TxtCfg(52).Key = "APPL3ALIAS"
      TxtCfg(52).CfgField = ApplAlias(3)

      TxtCfg(53).Key = "APPL3QUAL"
      TxtCfg(53).CfgField = ApplQual(3)

      TxtCfg(54).Key = "APPL4CALL"
      TxtCfg(54).CfgField = ApplCall(4)

      TxtCfg(55).Key = "APPL4ALIAS"
      TxtCfg(55).CfgField = ApplAlias(4)

      TxtCfg(56).Key = "APPL4QUAL"
      TxtCfg(56).CfgField = ApplQual(4)

      TxtCfg(57).Key = "APPL5CALL"
      TxtCfg(57).CfgField = ApplCall(5)

      TxtCfg(58).Key = "APPL5ALIAS"
      TxtCfg(58).CfgField = ApplAlias(5)

      TxtCfg(59).Key = "APPL5QUAL"
      TxtCfg(59).CfgField = ApplQual(5)

      TxtCfg(60).Key = "APPL6CALL"
      TxtCfg(60).CfgField = ApplCall(6)

      TxtCfg(61).Key = "APPL6ALIAS"
      TxtCfg(61).CfgField = ApplAlias(6)

      TxtCfg(62).Key = "APPL6QUAL"
      TxtCfg(62).CfgField = ApplQual(6)

      TxtCfg(63).Key = "APPL7CALL"
      TxtCfg(63).CfgField = ApplCall(7)

      TxtCfg(64).Key = "APPL7ALIAS"
      TxtCfg(64).CfgField = ApplAlias(7)

      TxtCfg(65).Key = "APPL7QUAL"
      TxtCfg(65).CfgField = ApplQual(7)

      TxtCfg(66).Key = "APPL8CALL"
      TxtCfg(66).CfgField = ApplCall(8)

      TxtCfg(67).Key = "APPL8ALIAS"
      TxtCfg(67).CfgField = ApplAlias(8)

      TxtCfg(68).Key = "APPL8QUAL"
      TxtCfg(68).CfgField = ApplQual(8)

      TxtCfg(69).Key = "MAXHOPS"
      TxtCfg(69).CfgField = Form1.MaxHopsBox

      TxtCfg(70).Key = "MAXRTT"
      TxtCfg(70).CfgField = Form1.MaxRTTBox

      TxtCfg(71).Key = "IPGATEWAY"
      '      TxtCfg(71).CfgField = Form1.IPGatewayBox
      '     TxtCfg(71).Checkbox = True

      TxtCfg(72).Key = "C_IS_CHAT"
      TxtCfg(72).CfgField = Form1.CIsChatBox
      TxtCfg(72).Checkbox = True


      TxtCfg(APPLPARAM).Key = "APPLICATION"

      TxtCfg(IPGATEWAY).Key = "IPGATEWAY"
      TxtCfg(IPGATEWAY).MultiLineText = True



      For i = 1 To NumofPortConfigParams
         ReDim TxtPortCfg(i).Value(32)
         ReDim TxtPortCfg(i).LineNo(32)
      Next

      TxtPortCfg(1).Key = "PORTNUM"
      TxtPortCfg(1).CfgField = PORTNUMBox

      TxtPortCfg(2).Key = "ID"
      TxtPortCfg(2).CfgField = PortIDBox
      TxtPortCfg(2).Len = 30

      TxtPortCfg(3).Key = "TYPE"
      TxtPortCfg(3).CfgField = TYPE

      TxtPortCfg(4).Key = "DLLNAME"
      TxtPortCfg(4).CfgField = DLLNAMEBox
      TxtPortCfg(4).Len = 16

      TxtPortCfg(5).Key = "PROTOCOL"
      TxtPortCfg(5).CfgField = PROTOCOLBox

      TxtPortCfg(6).Key = "KISSOPTIONS"
      TxtPortCfg(6).CfgField = KISSOptionsBox

      TxtPortCfg(7).Key = "IOADDR"
      TxtPortCfg(7).CfgField = IOADDRBox

      TxtPortCfg(8).Key = "INTLEVEL"
      TxtPortCfg(8).CfgField = INTLEVELBox

      TxtPortCfg(9).Key = "SPEED"
      TxtPortCfg(9).CfgField = SPEEDBox

      TxtPortCfg(10).Key = "CHANNEL"
      TxtPortCfg(10).CfgField = CHANNELBox

      TxtPortCfg(11).Key = "MHEARD"
      TxtPortCfg(11).CfgField = MHEARDBox
      TxtPortCfg(11).Checkbox = True
      TxtPortCfg(11).SetValue = "N"
      TxtPortCfg(11).UnSetValue = "Y"

      TxtPortCfg(12).Key = "QUALITY"
      TxtPortCfg(12).CfgField = QUALITYBox

      TxtPortCfg(13).Key = "MAXFRAME"
      TxtPortCfg(13).CfgField = MAXFRAMEBox

      TxtPortCfg(14).Key = "TXDELAY"
      TxtPortCfg(14).CfgField = TXDELAYBox

      TxtPortCfg(15).Key = "TXTAIL"
      TxtPortCfg(15).CfgField = TXTAILBox

      TxtPortCfg(16).Key = "SLOTTIME"
      TxtPortCfg(16).CfgField = SLOTTIMEBox

      TxtPortCfg(17).Key = "PERSIST"
      TxtPortCfg(17).CfgField = PERSISTBox

      TxtPortCfg(18).Key = "FULLDUP"
      TxtPortCfg(18).CfgField = FULLDUPBox
      TxtPortCfg(18).Checkbox = True
      TxtPortCfg(18).SetValue = "1"

      TxtPortCfg(19).Key = "SOFTDCD"
      TxtPortCfg(19).CfgField = SOFTDCDBox
      TxtPortCfg(19).Checkbox = True
      TxtPortCfg(19).SetValue = "1"

      TxtPortCfg(20).Key = "FRACK"
      TxtPortCfg(20).CfgField = FRACKBox

      TxtPortCfg(21).Key = "RESPTIME"
      TxtPortCfg(21).CfgField = RESPTIMEBox

      TxtPortCfg(22).Key = "RETRIES"
      TxtPortCfg(22).CfgField = RETRIESBox

      TxtPortCfg(23).Key = "PACLEN"
      TxtPortCfg(23).CfgField = PACLENBox

      TxtPortCfg(24).Key = "MAXDIGIS"
      TxtPortCfg(24).CfgField = MAXDIGISBox

      TxtPortCfg(25).Key = "DIGIFLAG"
      TxtPortCfg(25).CfgField = DIGIFLAGBox

      TxtPortCfg(26).Key = "DIGIPORT"
      TxtPortCfg(26).CfgField = DIGIPORTBox


      TxtPortCfg(27).Key = "CWID"
      TxtPortCfg(27).CfgField = CWIDBox
      TxtPortCfg(27).Len = 10

      TxtPortCfg(28).Key = "CWIDTYPE"
      TxtPortCfg(28).CfgField = CWIDTYPEBox
      TxtPortCfg(28).Checkbox = True
      TxtPortCfg(28).SetValue = "ONOFF"

      TxtPortCfg(29).Key = "PORTCALL"
      TxtPortCfg(29).CfgField = PORTCALLBox
      TxtPortCfg(29).Len = 10

      TxtPortCfg(30).Key = "PORTALIAS"
      TxtPortCfg(30).CfgField = PORTALIASBox
      TxtPortCfg(30).Len = 10

      TxtPortCfg(31).Key = "PORTALIAS2"
      TxtPortCfg(31).CfgField = PORTALIAS2Box
      TxtPortCfg(31).Len = 10

      TxtPortCfg(32).Key = "ALIAS_IS_BBS"
      TxtPortCfg(32).CfgField = ALIAS_IS_BBSBox
      TxtPortCfg(32).Checkbox = True
      TxtPortCfg(32).SetValue = "1"

      TxtPortCfg(33).Key = "BBSFLAG"
      TxtPortCfg(33).CfgField = BBSFLAGBox
      TxtPortCfg(33).Checkbox = True
      TxtPortCfg(33).SetValue = "1"

      TxtPortCfg(34).Key = "MINQUAL"
      TxtPortCfg(34).CfgField = MINQUALBox

      TxtPortCfg(35).Key = "NODESPACLEN"
      TxtPortCfg(35).CfgField = NODESPACLENBox

      TxtPortCfg(36).Key = "QUALADJUST"
      TxtPortCfg(36).CfgField = QUALADJUSTBox

      TxtPortCfg(37).Key = "BCALL"
      TxtPortCfg(37).CfgField = BCALLBox
      TxtPortCfg(37).Len = 10

      TxtPortCfg(38).Key = "UNPROTO"
      TxtPortCfg(38).CfgField = UNPROTOBox
      TxtPortCfg(38).Len = 72

      TxtPortCfg(39).Key = "L3ONLY"
      TxtPortCfg(39).CfgField = L3ONLYBox
      TxtPortCfg(39).Checkbox = True
      TxtPortCfg(39).SetValue = "1"

      TxtPortCfg(40).Key = "INTERLOCK"
      TxtPortCfg(40).CfgField = INTERLOCKBox

      TxtPortCfg(41).Key = "TXPORT"
      TxtPortCfg(41).CfgField = TXPORTBox

      TxtPortCfg(42).Key = "USERS"
      TxtPortCfg(42).CfgField = USERSBox

      TxtPortCfg(43).Key = "VALIDCALLS"
      TxtPortCfg(43).CfgField = VALIDCALLSBox
      TxtPortCfg(43).Len = 256

      TxtPortCfg(44).Key = "DIGIMASK"
      TxtPortCfg(44).CfgField = DIGIMASKBox

      TxtPortCfg(45).Key = "NOKEEPALIVES"
      TxtPortCfg(45).CfgField = NoKeepalivesBox
      TxtPortCfg(45).Checkbox = True
      TxtPortCfg(45).SetValue = "1"

   End Sub

   Function GetTextKissOptions(ByVal Value As String) As Integer

      Dim i As Integer, KissParam As String, KissOption As Integer = 0

      Value = Value & ","

      Do

         i = InStr(Value, ",")

         KissParam = Microsoft.VisualBasic.Left(Value, i - 1)
         Value = Mid(Value, i + 1)

         If KissParam = "CHECKSUM" Then KissOption = KissOption + 1 _
          Else If KissParam = "POLLED" Then KissOption = KissOption + 2 _
          Else If KissParam = "ACKMODE" Then KissOption = KissOption + 4 _
          Else If KissParam = "SLAVE" Then KissOption = KissOption + 8 _
          Else If KissParam = "D700" Then KissOption = KissOption + 16 Else Return False

      Loop While Value <> ""

      Return KissOption

   End Function

   Public Sub Exit_Click(ByVal sender As Object, ByVal e As System.EventArgs)

      SavePortInfo()

      If MsgBox("Changes have not been saved - do you want to save before exiting?", _
           MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
      End If

      End

   End Sub

   Public Sub CreateWINMORControls(ByVal Port As Integer)

      MyLabel1(Port) = New Label
      MyLabel2(Port) = New Label
      MyLabel3(Port) = New Label
      MyLabel4(Port) = New Label

      WINMORPTT(Port) = New NonNullCombobox
      WINMORPTT(Port).Items.AddRange(New Object() {"VOX", "CI-V", "RTS", "DTR", "DTRRTS"})
      WINMORPTT(Port).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
      WINMORPTT(Port).Name = Port
      WINMORPTT(Port).Size = New System.Drawing.Size(75, 21)

      AddHandler WINMORPTT(Port).SelectedIndexChanged, AddressOf PTTChanged

      DriveLevel(Port) = New BPQCFG.NumTextBox(100)
      WINMORPort(Port) = New BPQCFG.NumTextBox(65535)

      CWID(Port) = New System.Windows.Forms.CheckBox
      BusyLock(Port) = New System.Windows.Forms.CheckBox
      DebugLog(Port) = New System.Windows.Forms.CheckBox
      BW(Port) = New NonNullCombobox
      PTTCOMM(Port) = New System.Windows.Forms.ComboBox

      MyLabel1(Port).Size = New System.Drawing.Size(28, 13)
      MyLabel1(Port).Text = "PTT"
      '
      MyLabel2(Port).Size = New System.Drawing.Size(61, 13)
      MyLabel2(Port).Text = "Drive Level"
      '
      MyLabel3(Port).Size = New System.Drawing.Size(90, 13)
      MyLabel3(Port).Text = "Winmor TNC Port"

      MyLabel4(Port).Size = New System.Drawing.Size(28, 13)
      MyLabel4(Port).Text = "BW"

      AddHandler WINMORPort(Port).Validating, AddressOf ValidatingNumberNonZero
      '
      DriveLevel(Port).Name = "DriveLevel"
      DriveLevel(Port).Size = New System.Drawing.Size(39, 20)
      DriveLevel(Port).Text = "100"
      '
      'WinmorPort
      '
      WINMORPort(Port).Name = Port
      WINMORPort(Port).Size = New System.Drawing.Size(39, 20)
      WINMORPort(Port).Text = "8500"

      'WINMORPTT
      '
      PTTCOMM(Port).Size = New System.Drawing.Size(70, 21)
      PTTCOMM(Port).Visible = False    ' Default is VOX

      BW(Port).Name = Port
      BW(Port).Size = New System.Drawing.Size(55, 21)
      '
      PTTCOMM(Port).Visible = False    ' Default is VOX

      For Each sp As String In My.Computer.Ports.SerialPortNames
         PTTCOMM(Port).Items.Add(sp)
      Next

      BW(Port).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
      BW(Port).Items.AddRange(New Object() {"500", "1600"})
      BW(Port).Name = Port
      BW(Port).Size = New System.Drawing.Size(55, 21)
      '
      'CWID
      '
      CWID(Port).Name = "CWID"
      CWID(Port).Size = New System.Drawing.Size(85, 17)
      CWID(Port).Text = "Enble CWID"
      '
      'BusyLock
      '
      BusyLock(Port).AutoSize = True
      BusyLock(Port).Name = "BusyLock"
      BusyLock(Port).Size = New System.Drawing.Size(76, 17)
      BusyLock(Port).TabIndex = 50
      BusyLock(Port).Text = "Busy Lock"
      BusyLock(Port).UseVisualStyleBackColor = True
      '
      'DebugLog
      '
      DebugLog(Port).AutoSize = True
      DebugLog(Port).Name = "DebugLog"
      DebugLog(Port).Size = New System.Drawing.Size(79, 17)
      DebugLog(Port).TabIndex = 49
      DebugLog(Port).Text = "Debug Log"
      DebugLog(Port).UseVisualStyleBackColor = True

      PathLabel(Port) = New Label
      PathBox(Port) = New TextBox

      PathLabel(Port).Size = New System.Drawing.Size(95, 13)
      PathLabel(Port).Text = "Path to Winmor TNC"

      PathBox(Port).Size = New System.Drawing.Size(400, 22)
      PathBox(Port).Text = ""

   End Sub

   Function ProcessWinmorLine(ByVal buf As String, ByVal Port As Integer) As Integer

      Dim i As Integer

      Try

         buf = UCase(buf)

         If buf.StartsWith("ADDR") Then

            Dim Fields() As String = Split(buf)

            WINMORPort(Port).Text = Fields(2)

            WINMORPTT(Port).SelectedItem = "VOX"      ' Default

            i = InStr(buf, "PTT")

            If i Then

               Fields = Split(Mid(buf, i))
               WINMORPTT(Port).SelectedIndex = WINMORPTT(Port).FindStringExact(Fields(1))

            End If

            i = InStr(buf, "PATH")

            If i Then

               Fields = Split(Mid(buf, i), " ", 2)
               PathBox(Port).Text = Fields(1)
            End If
         ElseIf buf.Contains("BUSYLOCK") Then
            BusyLock(Port).Checked = buf.Contains("TRUE")
         ElseIf buf.Contains("CWID") Then
            CWID(Port).Checked = buf.Contains("TRUE")
         ElseIf buf.Contains("DEBUGLOG") Then
            DebugLog(Port).Checked = buf.Contains("TRUE")
         ElseIf buf.Contains("DRIVELEVEL") Then
            DriveLevel(Port).Text = Mid(buf, 12)
         ElseIf buf.Contains("BW ") Then
            BW(Port).SelectedIndex = BW(Port).FindStringExact(Mid(buf, 4))
         Else
            Return 0       ' Leave any unrecognised lines as comments
         End If

         SimpleForm.SaveWINMOR(Port)

         Return -1

      Catch ex As Exception

      End Try

   End Function

   Sub CreateWINMORTab(ByVal Port As Integer)

      Dim Row1 As Integer = 35
      Dim Row2 As Integer = 65
      Dim Row3 As Integer = 95

      SimpleForm.CreateTab(Port)

      If MyLabel3(Port) Is Nothing Then

         CreateWINMORControls(Port)

      End If


      MyLabel1(Port).Location = New System.Drawing.Point(360, Row1 + 3)
      MyLabel2(Port).Location = New System.Drawing.Point(161, Row1 + 3)
      MyLabel3(Port).Location = New System.Drawing.Point(21, Row1 + 3)
      MyLabel4(Port).Location = New System.Drawing.Point(270, Row1 + 3)

      PORTTABS(Port).Controls.Add(MyLabel1(Port))
      PORTTABS(Port).Controls.Add(MyLabel2(Port))
      PORTTABS(Port).Controls.Add(MyLabel3(Port))
      PORTTABS(Port).Controls.Add(MyLabel4(Port))

      PORTTABS(Port).Name = "WINMOR"
      PORTTABS(Port).Text = "WINMOR"

      PORTTABS(Port).Controls.Add(WINMORPTT(Port))
      PORTTABS(Port).Controls.Add(DriveLevel(Port))
      PORTTABS(Port).Controls.Add(WINMORPort(Port))
      PORTTABS(Port).Controls.Add(CWID(Port))
      PORTTABS(Port).Controls.Add(BusyLock(Port))
      PORTTABS(Port).Controls.Add(DebugLog(Port))
      PORTTABS(Port).Controls.Add(BW(Port))
      PORTTABS(Port).Controls.Add(PTTCOMM(Port))

      DriveLevel(Port).Location = New System.Drawing.Point(223, Row1)
      WINMORPort(Port).Location = New System.Drawing.Point(118, Row1)
      AddHandler WINMORPort(Port).Validating, AddressOf ValidatingNumberNonZero

      WINMORPTT(Port).Location = New System.Drawing.Point(390, Row1)
      PTTCOMM(Port).Location = New System.Drawing.Point(475, Row1)

      BW(Port).Location = New System.Drawing.Point(300, Row1)
      CWID(Port).Location = New System.Drawing.Point(198, Row2)
      BusyLock(Port).Location = New System.Drawing.Point(109, Row2)
      DebugLog(Port).Location = New System.Drawing.Point(Col1, Row2)


      PathLabel(Port).Location = New System.Drawing.Point(21, Row3 + 3)
      PathBox(Port).Location = New System.Drawing.Point(118, Row3)

      PORTTABS(Port).Controls.Add(PathLabel(Port))
      PORTTABS(Port).Controls.Add(PathBox(Port))


      SaveProc(Port) = AddressOf SimpleForm.SaveWINMOR

   End Sub

   Public Sub SetDefaultL2Params(ByVal NumberOfPorts As Integer)

      TxtPortCfg(FRACK).Value(NumberOfPorts) = 7000
      TxtPortCfg(RESPTIME).Value(NumberOfPorts) = 1000
      TxtPortCfg(RETRIES).Value(NumberOfPorts) = 10
      TxtPortCfg(PACLEN).Value(NumberOfPorts) = 128 ' 236
      TxtPortCfg(MAXFRAME).Value(NumberOfPorts) = 2
      TxtPortCfg(CHANNEL).Value(NumberOfPorts) = "A"
      TxtPortCfg(TXDELAY).Value(NumberOfPorts) = 300
      TxtPortCfg(SLOTTIME).Value(NumberOfPorts) = 100
      TxtPortCfg(PERSIST).Value(NumberOfPorts) = 64

   End Sub

   Public Sub ClearDefaultL2Params(ByVal NumberOfPorts As Integer)

      TxtPortCfg(FRACK).Value(NumberOfPorts) = ""
      TxtPortCfg(RESPTIME).Value(NumberOfPorts) = ""
      TxtPortCfg(RETRIES).Value(NumberOfPorts) = ""
      TxtPortCfg(PACLEN).Value(NumberOfPorts) = ""
      TxtPortCfg(MAXFRAME).Value(NumberOfPorts) = ""
      TxtPortCfg(CHANNEL).Value(NumberOfPorts) = ""
      TxtPortCfg(TXDELAY).Value(NumberOfPorts) = ""
      TxtPortCfg(SLOTTIME).Value(NumberOfPorts) = ""
      TxtPortCfg(PERSIST).Value(NumberOfPorts) = ""

   End Sub

   Sub ClearPort(ByVal Port As Integer)

      Dim p As Integer

      For p = 1 To NumofPortConfigParams

         TxtPortCfg(p).Value(Port) = Nothing
         TxtPortCfg(p).LineNo(Port) = 0

5:    Next

      PortConfig(Port) = Nothing

   End Sub

   Sub CopyPort(ByVal Port As Integer, ByVal From As Integer)

      Dim p As Integer

      For p = 1 To NumofPortConfigParams
         TxtPortCfg(p).Value(Port) = TxtPortCfg(p).Value(From)
         '       ParamAtLine(TxtPortCfg(p).LineNo(Port)) = ParamAtLine(TxtPortCfg(p).LineNo(From))
      Next

      PortConfig(Port) = PortConfig(From)

   End Sub

   Sub ClearConfig()

      Dim i As Integer, j As Integer

      For j = 1 To 32

         For i = 1 To NumofPortConfigParams
            TxtPortCfg(i).Value(j) = Nothing
            TxtPortCfg(i).LineNo(j) = 0
         Next

         PortConfig(j) = Nothing
         DeletedPort(j) = False

      Next

      For i = 0 To 31
         RouteCall(i).Text = ""
         RouteQual(i).Text = ""
         RoutePort(i).Text = ""
         RouteMaxFrame(i).Text = ""
         RouteFrack(i).Text = ""
         RoutePaclen(i).Text = ""
         Routeinp3(i).Text = ""
         RouteComment(i) = ""
         RouteLineno(i) = 0

      Next

   End Sub

End Module

