
Imports System.Threading



Public Class SerialLink

   Dim TCPOpen As Boolean

   Private Sub SerialLink_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      TCPHostName.Text = My.Settings.TCPHost
      TCPPort.Text = My.Settings.TCPPort

      OptSerial.Checked = My.Settings.OptSerial
      OptTCPMaster.Checked = My.Settings.OptTCPMaster

      OptTCPSlave.Checked = My.Settings.OptTCPSlave

      For Each sp As String In My.Computer.Ports.SerialPortNames
         LocalCOM.Items.Insert(0, sp)
         RemoteCOM.Items.Insert(0, sp)
      Next

      LocalCOM.SelectedIndex = LocalCOM.FindString(My.Settings.LocalCOM)
      RemoteCOM.SelectedIndex = RemoteCOM.FindString(My.Settings.RemoteCOM)

      LocallBaud.Items.Add("38400")
      LocallBaud.Items.Add("19200")
      LocallBaud.Items.Add("9600")
      LocallBaud.Items.Add("4800")
      LocallBaud.Items.Add("2400")
      LocallBaud.Items.Add("1200")

      LocallBaud.SelectedIndex = LocallBaud.FindString(My.Settings.LocalBaud)

      RemoteBAUD.Items.Add("38400")
      RemoteBAUD.Items.Add("19200")
      RemoteBAUD.Items.Add("9600")
      RemoteBAUD.Items.Add("4800")
      RemoteBAUD.Items.Add("2400")
      RemoteBAUD.Items.Add("1200")

      RemoteBAUD.SelectedIndex = RemoteBAUD.FindString(My.Settings.RemoteBaud)


   End Sub

   Private Sub SerialPort1_DataReceived(ByVal sender As Object, ByVal e As System.IO.Ports.SerialDataReceivedEventArgs) Handles SerialPort1.DataReceived

      Dim len As Integer = SerialPort1.BytesToRead
      Dim Msg(len) As Byte
      Dim i As Integer

      For i = 0 To len - 1
         Msg(i) = SerialPort1.ReadByte()
      Next
      SerialPort2.Write(Msg, 0, len)

   End Sub

   Private Sub SerialPort2_DataReceived(ByVal sender As Object, ByVal e As System.IO.Ports.SerialDataReceivedEventArgs) Handles SerialPort2.DataReceived

      Dim len As Integer
      Dim LastLen As Integer = SerialPort2.BytesToRead
waitloop:
      Thread.Sleep(10)
      len = SerialPort2.BytesToRead
      If len > LastLen Then
         LastLen = len
         GoTo waitloop
      End If

      Dim Msg(len - 1) As Byte
      Dim i As Integer

      For i = 0 To len - 1
         Msg(i) = SerialPort2.ReadByte

      Next

      If OptSerial.Checked Then
         SerialPort1.Write(Msg, 0, len)
      Else
         If TCPOpen Then AxWinsock1.SendData(Msg)
      End If

   End Sub


   Private Sub Open_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Open.Click

      My.Settings.TCPHost = TCPHostName.Text
      My.Settings.TCPPort = TCPPort.Text

      My.Settings.OptSerial = OptSerial.Checked
      My.Settings.OptTCPMaster = OptTCPMaster.Checked
      My.Settings.OptTCPSlave = OptTCPSlave.Checked

      SerialPort2.Close()

      Try

         If OptSerial.Checked Then

            SerialPort1.Open()

         ElseIf OptTCPSlave.Checked Then

            AxWinsock1.Close()

            AxWinsock1.RemoteHost = "0.0.0.0"
            AxWinsock1.RemotePort = 0
            AxWinsock1.LocalPort = CInt(TCPPort.Text)

            AxWinsock1.Listen()
            TCPState.Text = "Listen"


         ElseIf OptTCPMaster.Checked Then

            AxWinsock1.Close()
            AxWinsock1.RemoteHost = TCPHostName.Text
            AxWinsock1.LocalPort = 0
            AxWinsock1.RemotePort = CInt(TCPPort.Text)
            AxWinsock1.Connect()
            TCPState.Text = "Connecting"

         End If

         SerialPort2.Open()

      Catch ex As Exception
         Debug.Print(ex.Message)

      End Try

      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen

   End Sub

   Private Sub Close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CloseButton.Click

      If OptSerial.Checked Then

         SerialPort1.Close()

      ElseIf OptTCPSlave.Checked Then

         AxWinsock1.Close()
         TCPState.Text = "Closed"
         TCPOpen = False


      ElseIf OptTCPMaster.Checked Then

         AxWinsock1.Close()
         TCPState.Text = "Closed"
         TCPOpen = False

      End If

      SerialPort2.Close()

      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen

   End Sub

   Private Sub AxWinsock1_ConnectEvent(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AxWinsock1.ConnectEvent

      TCPState.Text = "Connected"
      TCPOpen = True

   End Sub

   Private Sub AxWinsock1_ConnectionRequest(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_ConnectionRequestEvent) Handles AxWinsock1.ConnectionRequest

      ' Check if the control's State is closed. If not,
      ' close the connection before accepting the new
      ' connection.
      Try
         AxWinsock1.Close()
         ' Accept the request with the requestID
         ' parameter.
         AxWinsock1.Accept(e.requestID)

      Catch ex As Exception

      End Try
      TCPState.Text = "Connected"
      TCPOpen = True

   End Sub

   Private Sub AxWinsock1_DataArrival(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_DataArrivalEvent) Handles AxWinsock1.DataArrival

      Dim Len As Integer = e.bytesTotal
      Dim Msg(Len) As Byte

      AxWinsock1.GetData(Msg)

      SerialPort2.Write(Msg, 0, Msg.Length)


   End Sub

   Private Sub AxWinsock1_CloseEvent(ByVal sender As Object, ByVal e As System.EventArgs) Handles AxWinsock1.CloseEvent

      TCPState.Text = "Closed"
      TCPOpen = False

      Try
         AxWinsock1.Close()

         If OptTCPSlave.Checked Then

            AxWinsock1.Listen()
            TCPState.Text = "Listen"

         End If

      Catch ex As Exception

      End Try

   End Sub


   Private Sub LocalCOM_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LocalCOM.SelectedIndexChanged

      If SerialPort2.IsOpen Then SerialPort2.Close()

      SerialPort2.PortName = sender.SelectedItem()
      My.Settings.LocalCOM = sender.SelectedItem()

      CheckBox2.Text = sender.SelectedItem()

      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen


   End Sub

   Private Sub LocalBAUD_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LocallBaud.SelectedIndexChanged

      SerialPort2.BaudRate = sender.SelectedItem()
      My.Settings.LocalBaud = sender.SelectedItem()


   End Sub

   Private Sub ReemoteCOM_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoteCOM.SelectedIndexChanged

      If SerialPort1.IsOpen Then SerialPort1.Close()

      SerialPort1.PortName = sender.SelectedItem()
      CheckBox1.Text = sender.SelectedItem()
      My.Settings.RemoteCOM = sender.SelectedItem()


      CheckBox1.Checked = SerialPort1.IsOpen
      CheckBox2.Checked = SerialPort2.IsOpen


   End Sub

   Private Sub RemoteBAUD_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoteBAUD.SelectedIndexChanged

      SerialPort1.BaudRate = sender.SelectedItem()
      My.Settings.RemoteBaud = sender.SelectedItem()

   End Sub
End Class
