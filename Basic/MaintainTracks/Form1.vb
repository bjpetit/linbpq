Public Class Form1

    Public Structure Posn

        Public Timestamp As Integer
        Public Lat As Double
        Public Lon As Double
        Public Course As Double
        Public Speed As Double

    End Structure


    Public Structure TracksStruct

        Public Title As String
        Public TrackDate As Date
        Public NumberofPoints As Integer
        Public OrigFile As String
        Public Colour As Integer
        Public Trackpoints() As Posn

    End Structure


    Dim folderBrowserDialog1 As FolderBrowserDialog

    Public Tracks() As TracksStruct
    Public AISTracks() As TracksStruct
    Public Track As TracksStruct

    Public deDupedTracks() As TracksStruct
    Public SkipTrack() As Boolean
    Public Trackpoints() As Posn

    Public NumberofTracks = 0
    Public NumberofAISTracks = 0
    Public NumberofTrackPoints = 0

    Public DateOffset As Date

    Dim textfile2 As System.IO.StreamWriter


    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

        textfile2 = My.Computer.FileSystem.OpenTextFileWriter("c:\trk.txt", False, System.Text.Encoding.ASCII)

        DateOffset = CDate("january 1, 1970")


    End Sub



    Dim fn As String


    Private Sub OpenTLOG_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OpenTLOG.Click

        Dim TLOGDir As String

        Dim result As DialogResult

        folderBrowserDialog1 = New System.Windows.Forms.FolderBrowserDialog()

        folderBrowserDialog1.RootFolder = Environment.SpecialFolder.MyComputer
        folderBrowserDialog1.SelectedPath = "C:\"

        result = folderBrowserDialog1.ShowDialog()

        If (result = System.Windows.Forms.DialogResult.OK) Then

            TLOGDir = folderBrowserDialog1.SelectedPath


            If Not (TLOGDir Is Nothing) Then

                ReadTLOG(TLOGDir)

            End If
        End If
    End Sub



    Private Sub ReadTLOG(ByVal TlogDIR As String)

        Dim Textfile As System.IO.StreamReader
        Dim Seq As Integer
        Dim Line As String
        Dim i As Integer
        Dim recs As Integer

        For Seq = 1 To 9999

            fn = TlogDIR & "\tlog" & Seq & ".txt"

            Try

                Textfile = My.Computer.FileSystem.OpenTextFileReader(fn, System.Text.Encoding.ASCII)

            Catch ex As Exception

                Exit For

            End Try


            Do
                Line = Textfile.ReadLine

                If Microsoft.VisualBasic.Left(Line, 4) = "4002" Then

                    recs = ProcessHeader(Line)

                    NumberofTrackPoints = 0

                    ' Object Variable "Track" points to the currentt entry

                    If recs > 0 Then

                        For i = 1 To recs

                            Line = Textfile.ReadLine
                            ProcessTrackLine(Line)

                        Next

                        textfile2.WriteLine(Track.Trackpoints(1).Timestamp & " " & Track.Title & " " & fn)

                        Track.TrackDate = DateTime.FromOADate(Track.Trackpoints(1).Timestamp / 86400 + DateOffset.ToOADate)

                        '                     TestStr = Format(Track.TrackDate, "yyyyMMdd")

                        If Track.Colour = 16776960 Then

                            NumberofAISTracks = NumberofAISTracks + 1

                            ReDim Preserve AISTracks(NumberofAISTracks)

                            AISTracks(NumberofAISTracks) = Track

                        Else

                            NumberofTracks = NumberofTracks + 1

                            ReDim Preserve Tracks(NumberofTracks)

                            Tracks(NumberofTracks) = Track

                        End If

                    End If

                Else

                    '          Debug.Print(Line)

                End If

            Loop Until Textfile.EndOfStream

            Textfile.Close()

        Next

        Debug.Print(NumberofTracks)
        Debug.Print(NumberofAISTracks)

    End Sub


    Function ProcessHeader(ByVal Line) As Integer

        Dim Seq As Integer
        Dim Title As String
        Dim Field3 As String
        Dim Field4 As String
        Dim Colour As Integer
        Dim Field6 As Integer
        Dim Field7 As String
        Dim Field8 As String
        Dim Field9 As String
        Dim RecCount As Integer

        Dim i As Integer, Start As Integer

        i = InStr(Mid(Line, 6), Chr(9))

        Start = 6

        Seq = Mid(Line, Start, i)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Title = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field3 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field4 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Colour = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field6 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field7 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field8 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field9 = Mid(Line, Start, i - 1)
        Start = Start + i

        RecCount = Mid(Line, Start)

        '      If Colour = 16776960 Then Return 0 ' AIS Track

        If (RecCount < 3) Then

            Debug.Print(Title & " " & fn & " " & RecCount)
            RecCount = 0

        Else

            Track.NumberofPoints = RecCount
            Track.Title = Title
            Track.OrigFile = fn
            Track.Colour = Colour

            ReDim Preserve Track.Trackpoints(RecCount)

        End If

        Return RecCount


    End Function

    Sub ProcessTrackLine(ByVal Line)

        Dim Seq As Integer
        Dim Title As String
        Dim Field3 As String
        Dim Lat As Double
        Dim Lon As Double
        Dim Field6 As String
        Dim Field7 As String
        Dim Field8 As String
        Dim Field9 As String
        Dim Field10 As String
        Dim COG As Double
        Dim SOG As Double


        Dim i As Integer, Start As Integer


        i = InStr(Mid(Line, 6), Chr(9))

        Start = 6

        Seq = Mid(Line, Start, i)

        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Title = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field3 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Lat = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Lon = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field6 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field7 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field8 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field9 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        Field10 = Mid(Line, Start, i - 1)
        Start = Start + i

        i = InStr(Mid(Line, Start), Chr(9))
        COG = Mid(Line, Start, i - 1)
        Start = Start + i

        SOG = Mid(Line, Start)

        NumberofTrackPoints = NumberofTrackPoints + 1

        Track.Trackpoints(NumberofTrackPoints).Timestamp = Field3
        Track.Trackpoints(NumberofTrackPoints).Course = COG
        Track.Trackpoints(NumberofTrackPoints).Speed = SOG
        Track.Trackpoints(NumberofTrackPoints).Lat = Lat
        Track.Trackpoints(NumberofTrackPoints).Lon = Lon

    End Sub

    Private Sub LookforDups_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RemoveDups.Click

        Dim i As Integer, j As Integer, Dups As Integer, n As Integer

        ReDim SkipTrack(NumberofTracks)

        Dim textfile As System.IO.StreamWriter

        ReDim deDupedTracks(NumberofTracks)

        Dups = 0


        For i = 1 To NumberofTracks - 1

            For j = i + 1 To NumberofTracks

                If (Tracks(i).Trackpoints(1).Timestamp = Tracks(j).Trackpoints(1).Timestamp) Then
                    If (Tracks(i).NumberofPoints = Tracks(j).NumberofPoints) Then

                        ' Print it titles different

                        If Tracks(i).Title <> Tracks(j).Title Then

                            Debug.Print(Tracks(i).Title)
                            Debug.Print(Tracks(j).Title)

                        End If

                        SkipTrack(j) = True
                        Dups = Dups + 1
                        Exit For

                    End If
                End If
            Next j


        Next i

        '       n = n + 1
        '      deDupedTracks(n) = Tracks(i)
        n = 0


        Debug.Print(Dups)

        If Dups > 0 Then

            textfile = My.Computer.FileSystem.OpenTextFileWriter("c:\trkdup.txt", False, System.Text.Encoding.ASCII)

            For i = 1 To NumberofTracks

                If SkipTrack(i) Then

                    textfile.WriteLine(Tracks(i).Trackpoints(1).Timestamp & " " & Tracks(i).Title & " " & Tracks(i).OrigFile)

                Else

                    n = n + 1
                    deDupedTracks(n) = Tracks(i)

                End If

            Next

            '           For i = 1 To n

            ReDim Preserve deDupedTracks(n)

            Tracks = deDupedTracks

            '
            '           Next

            NumberofTracks = n

            textfile.Close()

        End If

    End Sub



    Private Sub CreateV3TLOG_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CreateV3TLOG.Click

        Dim textfile As System.IO.StreamWriter
        Dim Line As String
        Dim i As Integer, j As Integer
        Dim Tab As Char = Chr(9)
        Dim Tkp As Posn

        '        4002	1	20070619 Isle of Lewis 20070619	 	0	255	0	00:00:10	0.1	0.005	90

        textfile = My.Computer.FileSystem.OpenTextFileWriter("c:\TLOGV3.txt", False, System.Text.Encoding.ASCII)

        For i = 1 To NumberofTracks

            Line = "4002" & Tab & i & Tab & Tracks(i).Title & Tab & " " & Tab & "0" & Tab & Tracks(i).Colour & Tab & "0" & Tab & "00:00:10" & Tab & "0.1" & Tab & "0.005" & Tab & Tracks(i).NumberofPoints
            textfile.WriteLine(Line)

            For j = 1 To Tracks(i).NumberofPoints
                '4001	0	1	1182247303	3478.614532	321.892103	TK0001	 	0	255	0	0.000000	0.000000


                Trackpoints = Tracks(i).Trackpoints
                Tkp = Trackpoints(j)

                Line = "4001" & Tab & "0" & Tab & j & Tab & Tkp.Timestamp & Tab & Tkp.Lat & Tab & Tkp.Lon & Tab & "TK" & j & Tab & " " & Tab & "0" & Tab & Tracks(i).Colour & Tab & "0" & Tab & Tkp.Course & Tab & Tkp.Speed

                textfile.WriteLine(Line)

            Next
        Next

        textfile.Close()

    End Sub

    Private Sub EditTitles_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EditTitles.Click

        EditTracksForm.Visible = True

    End Sub

    Private Sub SortButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SortButton.Click

        Dim keys(NumberofTracks) As Integer
        Dim Pointers(NumberofTracks) As Integer
        Dim SortedTracks() As TracksStruct

        Dim i As Integer

        keys(0) = -2000000000

        For i = 1 To NumberofTracks
            keys(i) = Tracks(i).Trackpoints(1).Timestamp
            Pointers(i) = i
        Next

        ReDim SortedTracks(NumberofTracks)

        Array.Sort(keys, Pointers)

        For i = 1 To NumberofTracks

            SortedTracks(i) = Tracks(Pointers(i))

        Next

        Tracks = SortedTracks

    End Sub

    Private Sub CreateV5CSV_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CreateV5CSV.Click

        Dim textfile As System.IO.StreamWriter
        Dim Line As String
        Dim i As Integer, j As Integer
        Dim Quote As Char = Chr(34)
        Dim Tkp As Posn
        Dim Mins As Double, Degs As Double
        Dim LatString As String
        Dim LonString As String
        Dim startdate As Date = CDate("jan 1, 1900")
        Dim enddate As Date = CDate("dec 31, 2100")
        '       Dim startdate As Date = CDate("may 8, 2004")
        '       Dim enddate As Date = CDate("may 28, 2004")

        '        4002	1	20070619 Isle of Lewis 20070619	 	0	255	0	00:00:10	0.1	0.005	90
        ' TK01, "Port Appin to Ballachulish", "Track", 1, 1, 1000, 0, 255, 3600, 900, 15, 10
        ' TP02, 56.553617, -5.41615, 0.0, 1157454701, 0.0, 0.0, 0
        ' TP02, 56.5535, -5.416317, 0.0, 1157455441, 0.04, 218.3, 0

        textfile = My.Computer.FileSystem.OpenTextFileWriter("c:\TLOGV5.CSV", False, System.Text.Encoding.ASCII)

        For i = 1 To NumberofTracks

            If Tracks(i).TrackDate > startdate And Tracks(i).TrackDate <= enddate Then


                Line = "TK01, " & Quote & Tracks(i).Title & Quote & ", " & Quote & Mid(Tracks(i).Title, 1, 4) & " Tracks" & Quote & ", 1,1, 1000, 0, " & Tracks(i).Colour & ", 3600, 900, 15, 10"
                textfile.WriteLine(Line)

                For j = 1 To Tracks(i).NumberofPoints

                    ' TP02, 56.5535, -5.416317, 0.0, 1157455441, 0.04, 218.3, 0

                    Trackpoints = Tracks(i).Trackpoints
                    Tkp = Trackpoints(j)

                    Mins = Tkp.Lat Mod 60
                    Degs = (Tkp.Lat - Mins) / 60
                    Degs = Degs + Mins / 60
                    LatString = Format(Degs, "0.#######")

                    Mins = Tkp.Lon Mod 60
                    Degs = (Tkp.Lon - Mins) / 60
                    Degs = Degs + Mins / 60
                    LonString = Format(-Degs, "0.#######")

                    Line = "TP02, " & LatString & ", " & LonString & ", " & "0.0" & ", " & Tkp.Timestamp & ", " & Tkp.Course & ", " & Tkp.Speed & ", "

                    textfile.WriteLine(Line)

                Next

            End If
        Next

        textfile.Close()

    End Sub

    Private Sub Archive_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Archive.Click


        FileOpen(1, "c:\SaveTracks.bin", OpenMode.Binary)

        FilePut(1, Tracks, 1, True, False)

        FileClose(1)

    End Sub


    Private Sub LoadArchive_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LoadArchive.Click, LoadArchive.Click

        FileOpen(1, "c:\SaveTracks.bin", OpenMode.Binary)

        ReDim Tracks(1)

        FileGet(1, Tracks, 1, True, False)

        NumberofTracks = Tracks.Length - 1

        FileClose(1)

    End Sub

    Private Sub DateStamp_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles DateStamp.Click

        Dim i As Integer
        Dim Title As String, DateStr As String


        For i = 1 To NumberofTracks

            Title = Tracks(i).Title

            If Microsoft.VisualBasic.Left(Title, 4) = "1970" Or Microsoft.VisualBasic.Left(Title, 3) = "200" Then

                Title = LTrim(RTrim(Mid(Title, 9)))

            End If

            If LCase(Microsoft.VisualBasic.Left(Title, 9)) = "track of " Then

                Title = LTrim(RTrim(Mid(Title, 10)))

            End If

            If LCase(Microsoft.VisualBasic.Left(Title, 5)) = "track" Then

                Title = Mid(Title, 6)

            End If

            DateStr = Format(Tracks(i).TrackDate, "yyyyMMdd")

            Tracks(i).Title = DateStr & " " & LTrim(RTrim(Title))

        Next

    End Sub

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Read_CSV.Click
        Dim tim As Long, Lat As Double, Lon As Double, COG As Double, SOG As Double
        Dim i As Integer
        Dim val As String
        Dim fileReader As System.IO.StreamReader
        fileReader = My.Computer.FileSystem.OpenTextFileReader("c:\Oban-Canna 1.csv")
        Dim line As String

        Do

            line = fileReader.ReadLine()
            If line Is Nothing Then Exit Do

            '  TP02, 55.641467, -4.819167, 0.0, 1207743623, 0.0, 0.0, 0

            'Public Title As String
            'Public TrackDate As Date
            'Public NumberofPoints As Integer
            'Public OrigFile As String
            'Public Colour As Integer
            'Public Trackpoints() As Posn


            '  TK01, "20080628 Oban Marina to Canna 1", "2008 Tracks", 1, 0, 255, 0, 255, 3600, 300, 15, 10

            If Microsoft.VisualBasic.Left(line, 4) = "TK01" Then

                Dim Seq As Integer
                Dim Title As String
                Dim Field3 As String
                Dim Field4 As String
                Dim Field5 As Integer
                Dim RecCount As Integer

                Dim Start As Integer

                Start = 6

                i = InStr(Mid(line, Start), ",")
                Title = Mid(line, Start, i - 1)
                Start = Start + i

                i = InStr(Mid(line, Start), ",")
                Field3 = Mid(line, Start, i - 1)
                Start = Start + i

                i = InStr(Mid(line, Start), ",")
                Field4 = Mid(line, Start, i - 1)
                Start = Start + i

                i = InStr(Mid(line, Start), ",")
                Field5 = Mid(line, Start, i - 1)
                Start = Start + i

                i = InStr(Mid(line, Start), ",")
                RecCount = Mid(line, Start, i - 1)
                Track.NumberofPoints = RecCount
                Track.Title = Title
                Track.OrigFile = fn
                Track.Colour = 255

                ReDim Preserve Track.Trackpoints(RecCount)

                NumberofTracks = NumberofTracks + 1

                ReDim Preserve Tracks(NumberofTracks)

                Tracks(NumberofTracks) = Track

                NumberofTrackPoints = 0

            End If



            If Microsoft.VisualBasic.Left(line, 4) = "TP02" Then

                '  TP02, 56.417967, -5.4971, 0.0, 1199167087, 0.0, 0.0, 0


                i = InStr(Mid(line, 6), ",")

                val = Mid(line, 6, i - 1)

                Lat = val
                line = Mid(line, i + 6)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                Lon = val

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                tim = val

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                SOG = val

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                COG = val
                NumberofTrackPoints = NumberofTrackPoints + 1

                If NumberofTrackPoints = 1 Then
                    Tracks(NumberofTracks).TrackDate = DateTime.FromOADate(tim / 86400 + DateOffset.ToOADate)
                End If

                Tracks(NumberofTracks).Trackpoints(NumberofTrackPoints).Timestamp = tim
                Tracks(NumberofTracks).Trackpoints(NumberofTrackPoints).Course = SOG
                Tracks(NumberofTracks).Trackpoints(NumberofTrackPoints).Speed = COG
                Tracks(NumberofTracks).Trackpoints(NumberofTrackPoints).Lat = Lat * 60
                Tracks(NumberofTracks).Trackpoints(NumberofTrackPoints).Lon = Lon * -60

            End If

        Loop

        fileReader.Close()

    End Sub
End Class
