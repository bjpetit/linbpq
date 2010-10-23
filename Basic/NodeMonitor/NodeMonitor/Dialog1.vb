Imports System.Windows.Forms

Public Class ConfigBox

   Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click

      My.Settings.FileName = FileNameBox.Text
      My.Settings.OutputFileName = OutputFileBox.Text
      My.Settings.NodesFileName = NodeOutputFileBox.Text
      My.Settings.UserName = UserBox.Text
      My.Settings.Password = PasswordBox.Text
      My.Settings.URL = URLBox.Text
      My.Settings.NodeURL = NodeURLBox.Text
      My.Settings.Port = CInt(PortNum.Text)
      My.Settings.UseNode = MonitorNode.Checked
      My.Settings.UseUDP = MonitorUDP.Checked

      My.Settings.Save()

      MsgBox("Restart program to apply changes")

      Me.DialogResult = System.Windows.Forms.DialogResult.OK
      Me.Close()
   End Sub

   Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
      Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Close()
   End Sub

   Private Sub Dialog1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      AutoUpdateBox.Checked = My.Settings.AutoUpdate
      FileNameBox.Text = My.Settings.FileName
      OutputFileBox.Text = My.Settings.OutputFileName
      NodeOutputFileBox.Text = My.Settings.NodesFileName
      UserBox.Text = My.Settings.UserName
      PasswordBox.Text = My.Settings.Password
      URLBox.Text = My.Settings.URL
      NodeURLBox.Text = My.Settings.NodeURL
      PortNum.Text = My.Settings.Port.ToString
      MonitorNode.Checked = My.Settings.UseNode
      MonitorUDP.Checked = My.Settings.UseUDP

   End Sub

 
End Class
