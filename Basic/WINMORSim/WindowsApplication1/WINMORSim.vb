Public Class WINMORSim

   Private Sub AxWinsock1_ConnectEvent(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AxWinsock1.ConnectEvent

   End Sub

   Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      AxWinsock1.Listen()
      AxWinsock2.Listen()

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
      Label1.Text = "Connected"

   End Sub

   Private Sub AxWinsock1_DataArrival(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_DataArrivalEvent) Handles AxWinsock1.DataArrival

      Dim Msg As String
      Dim i As Integer

      Msg = Space(e.bytesTotal)
      AxWinsock1.GetData(Msg)

      Dim Lines() As String = Msg.Split(vbCr)

      For i = 0 To Lines.Length - 2
         CmdTextBox.AppendText(Lines(i) + vbCrLf)
         ProcessCmdLine(Lines(i))
      Next

   End Sub

   Private Sub AxWinsock1_CloseEvent(ByVal sender As Object, ByVal e As System.EventArgs) Handles AxWinsock1.CloseEvent

      Label1.Text = "Disconnected"

      Try
         AxWinsock1.Close()
         AxWinsock1.Listen()

      Catch ex As Exception

      End Try

   End Sub

   Private Sub AxWinsock2_ConnectionRequest(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_ConnectionRequestEvent) Handles AxWinsock2.ConnectionRequest

      ' Check if the control's State is closed. If not,
      ' close the connection before accepting the new
      ' connection.
      Try
         AxWinsock2.Close()
         ' Accept the request with the requestID
         ' parameter.
         AxWinsock2.Accept(e.requestID)

      Catch ex As Exception

      End Try
      Label2.Text = "Connected"

   End Sub

   Private Sub AxWinsock2_DataArrival(ByVal sender As Object, ByVal e As AxMSWinsockLib.DMSWinsockControlEvents_DataArrivalEvent) Handles AxWinsock2.DataArrival

      Dim Msg As String
      Dim i As Integer

      Msg = Space(e.bytesTotal)
      AxWinsock2.GetData(Msg)

      Dim Lines() As String = Msg.Split(vbCr)

      For i = 0 To Lines.Length - 2
         DataTextBox.AppendText(Lines(i) + vbCrLf)
         ProcessDataLine(Lines(i))
      Next

   End Sub

   Private Sub ProcessCmdLine(ByVal Msg As String)

      If Msg = "C 2E1BHT" Then
         AxWinsock1.SendData("CONNECTED 2E1BHT" & vbCr)
         AxWinsock2.SendData("[XXX-BFH$]" & vbCr)
         AxWinsock2.SendData(">" & vbCr)
         Exit Sub
      End If

      AxWinsock1.SendData(Msg & vbCr)

   End Sub

   Private Sub ProcessDataLine(ByVal Msg As String)

      If Msg = "C GM8BPQ" Then
         AxWinsock1.SendData("CONNECTED GM8BPQ" & vbCr)
         AxWinsock2.SendData("[XXX-BFH$]" & vbCr)
         AxWinsock2.SendData(">" & vbCr)
         Exit Sub
      End If

      If Msg = "FQ" Then
         AxWinsock1.SendData("DISCONNECTED" & vbCr)
         Exit Sub
      End If

      If Msg = "FF" Then
         AxWinsock2.SendData("FQ" & vbCr)
         Exit Sub
      End If

      If Mid(Msg, Msg.Length, 1) = ">" Then
         AxWinsock2.SendData("[XXX-BFH$]" & vbCr)
         AxWinsock2.SendData("FF" & vbCr)
      End If

   End Sub

   Private Sub AxWinsock2_CloseEvent(ByVal sender As Object, ByVal e As System.EventArgs) Handles AxWinsock2.CloseEvent

      Label2.Text = "Disconnected"

      Try
         AxWinsock2.Close()
         AxWinsock2.Listen()

      Catch ex As Exception

      End Try

   End Sub

   Private Sub SendConnected_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SendConnected.Click

      AxWinsock1.SendData("CONNECTED GM8BPQ" & vbCr)

   End Sub

   Private Sub AxWinsock2_ConnectEvent(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AxWinsock2.ConnectEvent

   End Sub

   Private Sub DIS_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles DIS.Click

      AxWinsock1.SendData("DISCONNECTED" & vbCr)

   End Sub
End Class
