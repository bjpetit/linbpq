Public Class FrameInfo
    ' This class is used to define and return properties about the (up to) 256 FrameType codes.  
#Region "Declarations"
    Private strFrameType(255) As String
    Public bytValidFrameTypes() As Byte
#End Region ' Declarations

#Region "Properties"
    ' Property to retrieve Name (string) from Frame type (byte)
    Public ReadOnly Property Name(bytID As Byte) As String
        Get
            If bytID < &H20 Then
                Return strFrameType(0)
            ElseIf bytID >= &HE0 Then
                Return strFrameType(&HE0)
            Else
                Return strFrameType(bytID)
            End If
        End Get
    End Property 'Name

    ' Gets the number of samples (@12 KHz) needed to complete the frame after the Frame ID is detected 
    ' Value is increased by factor of 1.005 (5000 ppm)  to accomodate sample rate offsets in Transmitter and Receiver 
    Public ReadOnly Property SamplesToComplete(bytID As Byte) As Integer
        Get
            ' Note these samples DO NOT include the PSK reference symbols which are accomodated in DemodPSK
            If (bytID >= 0 And bytID <= &H1F) Or (bytID >= &HE0) Then Return 0 '  DataACK and DataNAK
            If (bytID = &H23 Or bytID = &H26 Or bytID = &H29 Or bytID = &H2C Or bytID = &H2D Or bytID = &H2E) Then Return 0 ' Short control frames
            If (bytID >= &H30 And bytID <= &H38) Then Return CInt(1.005 * (6 + 6 + 2) * 4 * 240) ' ID Frame Call sign + Grid Square, Connect request frames
            If (bytID >= &H39 And bytID <= &H3C) Then Return CInt(1.005 * 240 * 3 * 4) ' Con ACK with timing data 

            If bytID = &H40 Or bytID = &H41 Then Return CInt(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)) ' 1 carrier 100 baud 4PSK
            If bytID = &H42 Or bytID = &H43 Then Return CInt(1.005 * (120 + (1 + 16 + 2 + 8) * 4 * 120)) ' 1 carrier 100 baud 4PSK Short
            If bytID = &H44 Or bytID = &H45 Then Return CInt(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) \ 3))) ' 1 carrier 100 baud 8PSK
            If bytID = &H46 Or bytID = &H47 Then Return CInt(1.005 * (240 * 4 * (1 + 32 + 2 + 8))) ' 1 carrier 50 baud 4FSK  
            If bytID = &H48 Or bytID = &H49 Then Return CInt(1.005 * (240 * 4 * (1 + 16 + 2 + 4))) ' 1 carrier 50 baud 4FSK short 
            If bytID = &H4A Or bytID = &H4B Then Return CInt(1.005 * (120 * 4 * (1 + 64 + 2 + 16))) ' 1 carrier 100 baud 4FSK 
            If bytID = &H4C Or bytID = &H4D Then Return CInt(1.005 * (120 * 4 * (1 + 32 + 2 + 8))) ' 1 carrier 100 baud 4FSK short 
            If bytID = &H4E Or bytID = &H4F Then Return CInt(1.005 * (480 * (8 * (1 + 24 + 2 + 6)) \ 3)) ' 1 carrier 25 baud 8FSK 

            If bytID = &H50 Or bytID = &H51 Then Return CInt(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)) ' 2 carrier 100 baud 4PSK
            If bytID = &H52 Or bytID = &H53 Then Return CInt(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) \ 3))) ' 2 carrier 100 baud 8PSK
            If bytID = &H54 Or bytID = &H55 Then Return CInt(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)) ' 2 carrier 167 baud 4PSK
            If bytID = &H56 Or bytID = &H57 Then Return CInt(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) \ 3))) ' 2 carrier 167 baud 8PSK
            If bytID = &H58 Or bytID = &H59 Then Return CInt(1.005 * (480 * 2 * (1 + 32 + 2 + 8))) ' 1 carrier 25 baud 16FSK (in testing) 
            If bytID = &H5A Or bytID = &H5B Then Return CInt(1.005 * (480 * 2 * (1 + 16 + 2 + 4))) ' 1 carrier 25 baud 16FSK Short (in testing) 

            If bytID = &H60 Or bytID = &H61 Then Return CInt(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)) ' 4 carrier 100 baud 4PSK
            If bytID = &H62 Or bytID = &H63 Then Return CInt(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) \ 3))) ' 4 carrier 100 baud 8PSK
            If bytID = &H64 Or bytID = &H65 Then Return CInt(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)) ' 4 carrier 167 baud 4PSK
            If bytID = &H66 Or bytID = &H67 Then Return CInt(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) \ 3))) ' 4 carrier 167 baud 8PSK
            If bytID = &H68 Or bytID = &H69 Then Return CInt(1.005 * (120 * 4 * (1 + 64 + 2 + 16))) ' 2 carrier 100 baud 4FSK 

            If bytID = &H70 Or bytID = &H71 Then Return CInt(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)) ' 8 carrier 100 baud 4PSK
            If bytID = &H72 Or bytID = &H73 Then Return CInt(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) \ 3))) ' 8 carrier 100 baud 8PSK
            If bytID = &H74 Or bytID = &H75 Then Return CInt(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)) ' 8 carrier 167 baud 4PSK
            If bytID = &H76 Or bytID = &H77 Then Return CInt(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) \ 3))) ' 2 carrier 167 baud 8PSK ' 8 carrier 167 baud 4PSK
            If bytID = &H78 Or bytID = &H79 Then Return CInt(1.005 * (120 * 4 * (1 + 64 + 2 + 16))) ' 4 carrier 100 baud 4FSK 
            ' experimental 600 baud for VHF/UHF FM
            If bytID = &H7A Or bytID = &H7B Then Return CInt(1.005 * (20 * 4 * 3 * (1 + 200 + 2 + 50))) ' 1 carrier 600 baud 4FSK (3 groups of 200 bytes each for RS compatibility) 
            If bytID = &H7C Or bytID = &H7D Then Return CInt(1.005 * (20 * 4 * (1 + 200 + 2 + 50))) ' 1 carrier 600 baud 4FSK short

            ' experimental SOUNDINGs
            If bytID = &HD0 Then Return CInt(1.005 * 60 * 18 * 40)
            Return -1 ' No frame type match
        End Get
    End Property ' SamplesToComplete

#End Region ' Properties

#Region "Public Subs/Functions"

    ' Function to determine if a valide frame type
    Public Function IsValidFrameType(bytType As Byte) As Boolean
        ' used in the minimum distance decoder (update if frames added or removed)
        If bytType >= 0 And bytType <= &H1F Then Return True
        If bytType = &H23 Or bytType = &H26 Or bytType = &H29 Or bytType = &H2C Or bytType = &H2D Or bytType = &H2E Then Return True
        If bytType >= &H30 And bytType <= &H3C Then Return True
        If bytType >= &H40 And bytType <= &H4F Then Return True
        If bytType >= &H50 And bytType <= &H5B Then Return True
        If bytType >= &H60 And bytType <= &H69 Then Return True
        If bytType >= &H70 And bytType <= &H7D Then Return True
        If bytType >= &HE0 And bytType <= &HFF Then Return True
        If bytType = &HD0 Then Return True
        Return False
    End Function  'IsValidFrameType

    ' Function to determine if frame type is short control frame
    Public Function IsShortControlFrame(bytType As Byte) As Boolean
        If bytType <= &H1F Then Return True ' NAK
        If bytType = &H23 Or bytType = &H26 Or bytType = &H29 Or bytType = &H2C Or bytType = &H2D Or bytType = &H2E Then Return True ' BREAK, IDLE, DISC, END, ConRejBusy, ConRejBW
        If bytType >= &HE0 Then Return True ' ACK
        Return False
    End Function 'IsShortControlFrame

    ' Function to determine if it is a data frame (Even OR Odd) 
    Public Function IsDataFrame(intFrameType As Byte) As Boolean
        Dim strFrame As String = Name(intFrameType)
        If IsNothing(strFrame) Then Return False
        Return (strFrame.EndsWith(".E") Or strFrame.EndsWith(".O"))
    End Function 'IsDataFrame

    ' Function to get base (even) data modes by bandwidth for ARQ sessions
    Public Function GetDataModes(intBW As Integer) As Byte()
        ' Revised version 0.3.5
        ' idea is to use this list in the gear shift algorithm to select modulation mode based on bandwidth and robustness.
        ' Sequence modes in approximate order of robustness ...most robust first, shorter frames of same modulation first
        Select Case intBW
            Case 200 ' Streamlined 0.3.1.6
                '8FSK.200.25, 4FSK.200.50, 4PSK.200.100, 8PSK.200.100
                '  (288, 429, 768, 1296 byte/min)
                Dim byt200 As Byte() = {&H4E, &H46, &H40, &H44}
                Return byt200
            Case 500 ' streamlined 0.3.1.6
                '16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4PSK.500.100, 8PSK.500.100, 8PSK.500.167)
                ' (329, 429, 881, 1536, 2592, 4305 bytes/min)
                Dim byt500 As Byte() = {&H5A, &H58, &H4A, &H50, &H52, &H56}
                Return byt500
            Case 1000 ' Streamlined 0.3.1.6
                '16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4FSK.1000.100, 4PSK.1000.100, 8PSK.1000.100, 8PSK.1000.167
                '(329, 429, 881, 1762, 3072, 5184, 8610 bytes/min) 
                Dim byt1000 As Byte() = {&H5A, &H58, &H4A, &H68, &H60, &H62, &H66}
                Return byt1000
            Case 2000
                If MCB.TuningRange > 0 Then ' Streamlined 0.3.1.6
                    ' These do not include the 600 baud modes for FM only.
                    '16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4FSK.1000.100, 4FSK.2000.100, 4PSK.2000.100, 8PSK.2000.100, 8PSK.2000.167)
                    '(329, 429, 881, 1762, 3624, 6144, 10386, 17220 bytes/min) 
                    Dim byt2000 As Byte() = {&H5A, &H58, &H4A, &H68, &H78, &H70, &H72, &H76}
                    Return byt2000
                Else
                    ' These include the 600 baud modes for FM only.
                    ' The following is temporary, Plan to replace 8PSK 8 carrier modes with high baud 4PSK and 8PSK.

                    ' 4FSK.500.100S, 4FSK.500.100, 4FSK.2000.600S, 4FSK.200.600, 4PSK.2000.100, 8PSK.2000.100)
                    ' (696, 881, 4338, 5863, 6144, 10386 bytes/min)
                    Dim byt2000 As Byte() = {&H4C, &H4A, &H7C, &H7A, &H70, &H72}
                    Return byt2000
                End If

        End Select
        Return Nothing
    End Function ' GetDataModes

    ' Function to look up frame info from bytFrameType
    Public Function FrameInfo(bytFrameType As Byte, ByRef blnOdd As Boolean, ByRef intNumCar As Integer, ByRef strMod As String, ByRef intBaud As Integer, ByRef intDataLen As Integer, ByRef intRSLen As Integer, ByRef bytQualThres As Byte, ByRef strType As String)
        ' Used to "lookup" all parameters by frame Type. 
        ' Returns True if all fields updated otherwise false (improper bytFrameType)

        ' 1 Carrier 4FSK control frames 
        If bytFrameType >= 0 And bytFrameType <= &H1F Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 0 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 40
        ElseIf bytFrameType = &H23 Or bytFrameType = &H26 Or bytFrameType = &H29 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 0 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 60
        ElseIf bytFrameType = &H2C Or bytFrameType = &H2D Or bytFrameType = &H2E Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 0 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 60
        ElseIf bytFrameType >= &H30 And bytFrameType <= &H38 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 14 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 50
        ElseIf bytFrameType >= &H39 And bytFrameType <= &H3C Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 3 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 50
        ElseIf bytFrameType >= &HE0 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 0 : intRSLen = 0 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 60

            ' 1 Carrier Data modes
            ' 100 baud PSK (200 baud not compatible with 200 Hz bandwidth)  (Note 1 carrier modes Qual Threshold reduced to 30 (was 50) for testing April 20, 2015
        ElseIf bytFrameType = &H40 Or bytFrameType = &H41 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 64 : intRSLen = 32 : strMod = "4PSK" : intBaud = 100 : bytQualThres = 30
        ElseIf bytFrameType = &H42 Or bytFrameType = &H43 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 16 : intRSLen = 8 : strMod = "4PSK" : intBaud = 100 : bytQualThres = 30
        ElseIf bytFrameType = &H44 Or bytFrameType = &H45 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 108 : intRSLen = 36 : strMod = "8PSK" : intBaud = 100 : bytQualThres = 30

            ' 50 baud 4FSK 200 Hz bandwidth
        ElseIf bytFrameType = &H46 Or bytFrameType = &H47 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 32 : intRSLen = 8 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 30
        ElseIf bytFrameType = &H48 Or bytFrameType = &H49 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 16 : intRSLen = 4 : strMod = "4FSK" : intBaud = 50 : bytQualThres = 30
            ' 100 baud 4FSK 500 Hz bandwidth
        ElseIf bytFrameType = &H4A Or bytFrameType = &H4B Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 64 : intRSLen = 16 : strMod = "4FSK" : intBaud = 100 : bytQualThres = 30
        ElseIf bytFrameType = &H4C Or bytFrameType = &H4D Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 32 : intRSLen = 8 : strMod = "4FSK" : intBaud = 100 : bytQualThres = 30
            ' 25 baud 8FSK 200 Hz bandwidth (in testing)
        ElseIf bytFrameType = &H4E Or bytFrameType = &H4F Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 24 : intRSLen = 6 : strMod = "8FSK" : intBaud = 25 : bytQualThres = 30
            ' 25 baud 16FSK 500 Hz bandwidth 
        ElseIf bytFrameType = &H58 Or bytFrameType = &H59 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 32 : intRSLen = 8 : strMod = "16FSK" : intBaud = 25 : bytQualThres = 30
        ElseIf bytFrameType = &H5A Or bytFrameType = &H5B Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 16 : intRSLen = 4 : strMod = "16FSK" : intBaud = 25 : bytQualThres = 30
            ' 600 baud 4FSK 2000 Hz bandwidth 
        ElseIf bytFrameType = &H7A Or bytFrameType = &H7B Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 600 : intRSLen = 150 : strMod = "4FSK" : intBaud = 600 : bytQualThres = 30
        ElseIf bytFrameType = &H7C Or bytFrameType = &H7D Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 1 : intDataLen = 200 : intRSLen = 50 : strMod = "4FSK" : intBaud = 600 : bytQualThres = 30

            ' 2 Carrier Data Modes
            ' 100 baud 
        ElseIf bytFrameType = &H50 Or bytFrameType = &H51 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 2 : intDataLen = 64 : intRSLen = 32 : strMod = "4PSK" : intBaud = 100 : bytQualThres = 50
        ElseIf bytFrameType = &H52 Or bytFrameType = &H53 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 2 : intDataLen = 108 : intRSLen = 36 : strMod = "8PSK" : intBaud = 100 : bytQualThres = 50
            ' 167 baud testing
        ElseIf bytFrameType = &H54 Or bytFrameType = &H55 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 2 : intDataLen = 120 : intRSLen = 40 : strMod = "4PSK" : intBaud = 167 : bytQualThres = 50
        ElseIf bytFrameType = &H56 Or bytFrameType = &H57 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 2 : intDataLen = 159 : intRSLen = 60 : strMod = "8PSK" : intBaud = 167 : bytQualThres = 50

            ' 4 Carrier Data Modes
            ' 100 baud
        ElseIf bytFrameType = &H60 Or bytFrameType = &H61 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 4 : intDataLen = 64 : intRSLen = 32 : strMod = "4PSK" : intBaud = 100 : bytQualThres = 50
        ElseIf bytFrameType = &H62 Or bytFrameType = &H63 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 4 : intDataLen = 108 : intRSLen = 36 : strMod = "8PSK" : intBaud = 100 : bytQualThres = 50
            '167 Baud
        ElseIf bytFrameType = &H64 Or bytFrameType = &H65 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 4 : intDataLen = 120 : intRSLen = 40 : strMod = "4PSK" : intBaud = 167 : bytQualThres = 50
        ElseIf bytFrameType = &H66 Or bytFrameType = &H67 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 4 : intDataLen = 159 : intRSLen = 60 : strMod = "8PSK" : intBaud = 167 : bytQualThres = 50
            ' 100 baud 2 carrier 4FSK
        ElseIf bytFrameType = &H68 Or bytFrameType = &H69 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 2 : intDataLen = 64 : intRSLen = 16 : strMod = "4FSK" : intBaud = 100 : bytQualThres = 50

            ' 8 Carrier Data modes
            ' 100 baud
        ElseIf bytFrameType = &H70 Or bytFrameType = &H71 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 8 : intDataLen = 64 : intRSLen = 32 : strMod = "4PSK" : intBaud = 100 : bytQualThres = 50
        ElseIf bytFrameType = &H72 Or bytFrameType = &H73 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 8 : intDataLen = 108 : intRSLen = 36 : strMod = "8PSK" : intBaud = 100
            ' 167 baud
        ElseIf bytFrameType = &H74 Or bytFrameType = &H75 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 8 : intDataLen = 120 : intRSLen = 40 : strMod = "4PSK" : intBaud = 167
        ElseIf bytFrameType = &H76 Or bytFrameType = &H77 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 8 : intDataLen = 159 : intRSLen = 60 : strMod = "8PSK" : intBaud = 167
            ' 100 baud 4 carrier 4FSK
        ElseIf bytFrameType = &H78 Or bytFrameType = &H79 Then : blnOdd = (&H1 And bytFrameType) <> 0 : intNumCar = 4 : intDataLen = 64 : intRSLen = 16 : strMod = "4FSK" : intBaud = 100 : bytQualThres = 50
        Else
            'Logs.Exception("[PSKDataInfo] No data for frame type= H" & Format(bytFrameType, "x"))
            Return False
        End If
        If (bytFrameType >= 0) And (bytFrameType <= &H1F) Then
            strType = strFrameType(&H0)
        ElseIf bytFrameType >= &HE0 Then
            strType = strFrameType(&HE0)
        Else
            strType = strFrameType(bytFrameType)
        End If
        Return True
    End Function '  FrameInfo

    ' Function to look up the byte value from the frame string name
    Public Function FrameCode(strFrameName As String) As Byte
        For i As Integer = 0 To 255
            If strFrameType(i) = strFrameName Then
                Return CByte(i)
            End If
        Next
        Return 0
    End Function 'FrameCode

    ' A function to compute the parity symbol used in the frame type encoding
    Public Function ComputeTypeParity(bytFrameType As Byte) As Byte
        Dim bytMask As Byte = &HC0
        Dim bytParitySum As Byte = &H1
        Dim bytSym As Byte = 0
        For k As Integer = 0 To 3
            bytSym = (bytMask And bytFrameType) >> (2 * (3 - k))
            bytParitySum = bytParitySum Xor bytSym
            bytMask = bytMask >> 2
        Next
        Return bytParitySum And &H3
    End Function ' ComputeTypeParity

    Public Sub New()
        InitFrameNames()
        InitValidFrameTypes()
    End Sub
#End Region ' Public Subs/Functions

#Region "Private Subs/Functions"

    ' Subroutine to initialize the String array strFrameType
    Private Sub InitFrameNames()
        ' subject to change assignments

        strFrameType(0) = "DataNAK" ' Range &H00 to &H1F includes 5 bits for quality 1 Car, 200Hz,4FSK

        'Short Control Frames 1 Car, 200Hz,4FSK  ' Reassigned May 22, 2015 for maximum "distance"
        strFrameType(&H23) = "BREAK"
        strFrameType(&H26) = "IDLE"
        strFrameType(&H29) = "DISC"
        strFrameType(&H2C) = "END"
        strFrameType(&H2D) = "ConRejBusy"
        strFrameType(&H2E) = "ConRejBW"


        'Special frames 1 Car, 200Hz,4FSK
        strFrameType(&H30) = "IDFrame"
        strFrameType(&H31) = "ConReq200M"
        strFrameType(&H32) = "ConReq500M"
        strFrameType(&H33) = "ConReq1000M"
        strFrameType(&H34) = "ConReq2000M"
        strFrameType(&H35) = "ConReq200F"
        strFrameType(&H36) = "ConReq500F"
        strFrameType(&H37) = "ConReq1000F"
        strFrameType(&H38) = "ConReq2000F"
        strFrameType(&H39) = "ConAck200"
        strFrameType(&H3A) = "ConAck500"
        strFrameType(&H3B) = "ConAck1000"
        strFrameType(&H3C) = "ConAck2000"
        ' Types &H3D to &H3F reserved

        ' 200 Hz Bandwidth Data 
        ' 1 Car PSK Data Modes 200 HzBW  100 baud 
        strFrameType(&H40) = "4PSK.200.100.E"
        strFrameType(&H41) = "4PSK.200.100.O"
        strFrameType(&H42) = "4PSK.200.100S.E"
        strFrameType(&H43) = "4PSK.200.100S.O"
        strFrameType(&H44) = "8PSK.200.100.E"
        strFrameType(&H45) = "8PSK.200.100.O"

        ' 1 Car 4FSK Data mode 200 HzBW, 50 baud 
        strFrameType(&H46) = "4FSK.200.50.E"
        strFrameType(&H47) = "4FSK.200.50.O"
        strFrameType(&H48) = "4FSK.200.50S.E"
        strFrameType(&H49) = "4FSK.200.50S.O"
        ' 1 Car 8FSK Data mode 200 Hz BW, 25 baud
        strFrameType(&H4E) = "8FSK.200.25.E"
        strFrameType(&H4F) = "8FSK.200.25.O"

        ' 500 Hz bandwidth Data 
        ' 1 Car 4FSK Data mode 500 Hz, 100 baud
        strFrameType(&H4A) = "4FSK.500.100.E"
        strFrameType(&H4B) = "4FSK.500.100.O"
        strFrameType(&H4C) = "4FSK.500.100S.E"
        strFrameType(&H4D) = "4FSK.500.100S.O"
        ' Types &H4Eto &H4F reserved

        ' 2 Car PSK Data Modes 100 baud
        strFrameType(&H50) = "4PSK.500.100.E"
        strFrameType(&H51) = "4PSK.500.100.O"
        strFrameType(&H52) = "8PSK.500.100.E"
        strFrameType(&H53) = "8PSK.500.100.O"

        ' 2 Car Data modes 167 baud  
        strFrameType(&H54) = "4PSK.500.167.E"
        strFrameType(&H55) = "4PSK.500.167.O"
        strFrameType(&H56) = "8PSK.500.167.E"
        strFrameType(&H57) = "8PSK.500.167.O"

        ' 1 Car 16FSK mode 25 baud
        strFrameType(&H58) = "16FSK.500.25.E"
        strFrameType(&H59) = "16FSK.500.25.O"
        strFrameType(&H5A) = "16FSK.500.25S.E"
        strFrameType(&H5B) = "16FSK.500.25S.O"


        ' 1 Khz Bandwidth Data Modes 
        '  4 Car 100 baud PSK
        strFrameType(&H60) = "4PSK.1000.100.E"
        strFrameType(&H61) = "4PSK.1000.100.O"
        strFrameType(&H62) = "8PSK.1000.100.E"
        strFrameType(&H63) = "8PSK.1000.100.O"
        ' 4 car 167 baud PSK
        strFrameType(&H64) = "4PSK.1000.167.E"
        strFrameType(&H65) = "4PSK.1000.167.O"
        strFrameType(&H66) = "8PSK.1000.167.E"
        strFrameType(&H67) = "8PSK.1000.167.O"
        ' 2 Car 4FSK 100 baud
        strFrameType(&H68) = "4FSK.1000.100.E"
        strFrameType(&H69) = "4FSK.1000.100.O"

        ' 2Khz Bandwidth Data Modes 
        '  8 Car 100 baud PSK
        strFrameType(&H70) = "4PSK.2000.100.E"
        strFrameType(&H71) = "4PSK.2000.100.O"
        strFrameType(&H72) = "8PSK.2000.100.E"
        strFrameType(&H73) = "8PSK.2000.100.O"
        '  8 Car 167 baud PSK
        strFrameType(&H74) = "4PSK.2000.167.E"
        strFrameType(&H75) = "4PSK.2000.167.O"
        strFrameType(&H76) = "8PSK.2000.167.E"
        strFrameType(&H77) = "8PSK.2000.167.O"
        ' 4 Car 4FSK 100 baud
        strFrameType(&H78) = "4FSK.2000.100.E"
        strFrameType(&H79) = "4FSK.2000.100.O"
        ' 1 Car 4FSK 600 baud (FM only)
        strFrameType(&H7A) = "4FSK.2000.600.E" 'Experimental 
        strFrameType(&H7B) = "4FSK.2000.600.O" 'Experimental
        strFrameType(&H7C) = "4FSK.2000.600S.E" 'Experimental
        strFrameType(&H7D) = "4FSK.2000.600S.O" 'Experimental


        ' Frame Types &HA0 to &HDF reserved for experimentation 
        strFrameType(&HD0) = "SOUND2K"
        'Data ACK  1 Car, 200Hz,4FSK
        strFrameType(&HE0) = "DataACK" ' Range &HE0 to &HFF includes 5 bits for quality 

    End Sub 'InitFrameType

    ' Subroutine to initialize valid frame types
    Private Sub InitValidFrameTypes()
        ReDim bytValidFrameTypes(255)
        Dim intValidFrameCtr As Int32 = 0

        For i As Integer = 0 To 255
            If IsValidFrameType(i) Then
                bytValidFrameTypes(intValidFrameCtr) = i
                intValidFrameCtr += 1
            End If
        Next
        ReDim Preserve bytValidFrameTypes(intValidFrameCtr - 1)
    End Sub  '   InitValidFrameTypes
#End Region ' Private Subs/Functions
End Class
