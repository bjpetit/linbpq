Imports System.Windows.Forms

Public Class EditTracksForm

    Dim Selecteditem As Integer

    Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub EditTitles_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Dim i As Integer

        For i = 1 To Form1.NumberofTracks

            Titles.Items.Add(Form1.Tracks(i).Title)

        Next

    End Sub

    Private Sub Titles_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Titles.SelectedIndexChanged

        If Titles.SelectedIndex = -1 Then
            Titles.SelectedIndex = 0
        End If


        Selecteditem = Titles.SelectedIndex

        EditItem.Text = Titles.SelectedItem
        DateTimePicker1.Value = Form1.Tracks(Selecteditem + 1).TrackDate
        Starttime.Text = Form1.Tracks(Selecteditem + 1).TrackDate
        Endtime.Text = DateTime.FromOADate(Form1.Tracks(Selecteditem + 1).Trackpoints(Form1.Tracks(Selecteditem + 1).NumberofPoints).Timestamp / 86400 + Form1.DateOffset.ToOADate)

        NoofPoints.Text = Form1.Tracks(Selecteditem + 1).NumberofPoints

    End Sub

    Private Sub EditItem_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EditItem.TextChanged

    End Sub

    Private Sub EditSave_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EditSave.Click

        Dim SaveItem As Integer
        Dim SaveText As String

        SaveItem = Selecteditem
        SaveText = EditItem.Text

        Form1.Tracks(SaveItem + 1).Title = SaveText
        Titles.Items.RemoveAt(SaveItem)
        Titles.Items.Insert(SaveItem, SaveText)

    End Sub

    Private Sub DateTimePicker1_ValueChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles DateTimePicker1.ValueChanged

    End Sub

    Private Sub ChangeDate_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ChangeDate.Click

        Dim Offset As TimeSpan, i As Integer, x As Date, y As Date
        Dim Adjust As TimeSpan

        x = Form1.Tracks(Selecteditem + 1).TrackDate
        y = DateTimePicker1.Value
        Offset = DateTimePicker1.Value - Form1.Tracks(Selecteditem + 1).TrackDate
        Adjust = New TimeSpan(0, 0, 12, 0, 0)

        Offset = Offset + Adjust

        Form1.Tracks(Selecteditem + 1).TrackDate = Form1.Tracks(Selecteditem + 1).TrackDate + Offset
        For i = 1 To Form1.Tracks(Selecteditem + 1).NumberofPoints

            Form1.Tracks(Selecteditem + 1).Trackpoints(i).Timestamp = Form1.Tracks(Selecteditem + 1).Trackpoints(i).Timestamp + Offset.TotalSeconds

        Next

    End Sub

    Private Sub Delete_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Delete.Click

        Dim i As Integer

        Dim SaveItem As Integer

        SaveItem = Selecteditem

        Form1.NumberofTracks = Form1.NumberofTracks - 1

        For i = Selecteditem + 1 To Form1.NumberofTracks

            Form1.Tracks(i) = Form1.Tracks(i + 1)

        Next

        Titles.Items.Clear()

        For i = 1 To Form1.NumberofTracks

            Titles.Items.Add(Form1.Tracks(i).Title)

        Next

        Titles.SelectedIndex = SaveItem

    End Sub

    Private Sub Combine_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Combine.Click

        Dim i As Integer, j As Integer

        j = Form1.Tracks(Selecteditem + 1).NumberofPoints

        ReDim Preserve Form1.Tracks(Selecteditem + 1).Trackpoints(j + Form1.Tracks(Selecteditem + 2).NumberofPoints)

        For i = 1 To Form1.Tracks(Selecteditem + 2).NumberofPoints

            j = j + 1

            Form1.Tracks(Selecteditem + 1).Trackpoints(j) = Form1.Tracks(Selecteditem + 2).Trackpoints(i)

        Next

        Form1.Tracks(Selecteditem + 1).NumberofPoints = j

    End Sub
End Class
