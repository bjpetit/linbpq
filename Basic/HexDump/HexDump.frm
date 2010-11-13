VERSION 4.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   7800
   ClientLeft      =   4320
   ClientTop       =   1968
   ClientWidth     =   10116
   BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
      Name            =   "Terminal"
      Size            =   10.8
      Charset         =   255
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Height          =   8340
   Left            =   4272
   LinkTopic       =   "Form1"
   ScaleHeight     =   7800
   ScaleWidth      =   10116
   Top             =   1476
   Width           =   10212
   Begin VB.CommandButton Command1 
      Caption         =   "Command1"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   9.6
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   5880
      TabIndex        =   2
      Top             =   120
      Width           =   1095
   End
   Begin VB.TextBox Text2 
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   7.8
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   120
      TabIndex        =   1
      Text            =   "c:\IMGP0187_tag.jpg"
      Top             =   120
      Width           =   3495
   End
   Begin VB.TextBox Text1 
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "Fixedsys"
         Size            =   10.8
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   7092
      Left            =   120
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Top             =   600
      Width           =   9732
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Private Sub Command1_Click()
Dim op As String

Text1 = ""

addr = 0

On Error GoTo xx
Close 1
On Error GoTo 0
Debug.Print Text2
Open Text2 For Binary As 1

lenx = LOF(1)
Debug.Print lenx, Hex(lenx)

For j = 1 To (lenx / 16) + 1

If lenx - addr > 15 Then Cnt = 16 Else Cnt = lenx - addr

rec = Input(Cnt, 1)
'If addr < 55000 Then GoTo skip
If addr > 65535 Then GoTo xx
'If addr > 2048 Then GoTo xx

addrx = Hex(addr)
If Len(addrx) = 1 Then addrx = "0" & addrx
If Len(addrx) = 2 Then addrx = "0" & addrx
If Len(addrx) = 3 Then addrx = "0" & addrx

op = addrx & "  "

For i = 1 To Cnt


hexch = Hex(Asc(Mid$(rec, i, 1))) + " "

If Len(hexch) = 2 Then hexch = "0" & hexch
op = op + hexch

If Asc(Mid(rec, i, 1)) < 32 Then Mid(rec, i, 1) = "."

Next i


op = op + "  " + rec
Text1 = Text1 + op + vbCrLf
skip:

addr = addr + Cnt
Next j
xx:
Debug.Print addr
Close i
End Sub


