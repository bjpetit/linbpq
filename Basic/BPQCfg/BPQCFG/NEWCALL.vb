Imports System.Windows.Forms

Public Class NEWCALL

    Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub NodeCall_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles NodeCall.TextChanged
        If NodeCall.Text.Length > 3 Then
            OK_Button.Enabled = True
        Else
            OK_Button.Enabled = False
        End If

    End Sub
End Class
