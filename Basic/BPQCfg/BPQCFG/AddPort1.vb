Imports System.Windows.Forms

Public Class AddPort

    Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub PortType_SelectedIndexChanged_1(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles PortType.SelectedIndexChanged
        If PortType.Text <> "" Then
            OK_Button.Enabled = True
        Else
            OK_Button.Enabled = False
        End If

        PORTID.Text = PortType.Text & " Port 1"

    End Sub

End Class
