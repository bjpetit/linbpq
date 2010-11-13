VERSION 4.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   2916
   ClientLeft      =   120
   ClientTop       =   2988
   ClientWidth     =   7500
   Height          =   3456
   Left            =   72
   LinkTopic       =   "Form1"
   ScaleHeight     =   2916
   ScaleWidth      =   7500
   Top             =   2496
   Width           =   7596
   Begin VB.CommandButton Command3 
      Caption         =   "Command3"
      Height          =   492
      Left            =   4320
      TabIndex        =   10
      Top             =   240
      Width           =   1692
   End
   Begin VB.CheckBox TestFlag 
      Caption         =   "Test?"
      Height          =   375
      Left            =   240
      TabIndex        =   9
      Top             =   2280
      Value           =   1  'Checked
      Width           =   1095
   End
   Begin VB.TextBox SearchMask 
      Height          =   375
      Left            =   6120
      TabIndex        =   8
      Text            =   "File*.tif"
      Top             =   960
      Width           =   975
   End
   Begin VB.TextBox DestSuffix 
      Height          =   375
      Left            =   6120
      TabIndex        =   7
      Text            =   ".tif"
      Top             =   1680
      Width           =   375
   End
   Begin VB.TextBox SourceDir 
      Height          =   375
      Left            =   960
      TabIndex        =   4
      Text            =   "C:\Photos\Scanned Slides\New Folder\"
      Top             =   960
      Width           =   4935
   End
   Begin VB.TextBox NewDirRoot 
      Height          =   375
      Left            =   960
      TabIndex        =   3
      Text            =   "C:\Photos\Scanned Slides\New Folder\Pic"
      Top             =   1680
      Width           =   4935
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Command2"
      Height          =   495
      Left            =   2640
      TabIndex        =   2
      Top             =   240
      Width           =   1455
   End
   Begin VB.TextBox Offset 
      Height          =   375
      Left            =   1800
      TabIndex        =   1
      Text            =   "9000"
      Top             =   360
      Width           =   615
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Command1"
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   1455
   End
   Begin VB.Label Label2 
      Caption         =   "Dest "
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   1720
      Width           =   615
   End
   Begin VB.Label Label1 
      Caption         =   "Source"
      Height          =   375
      Left            =   240
      TabIndex        =   5
      Top             =   1020
      Width           =   615
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Option Explicit

Dim num As Long
Private Sub Command1_Click()

Dim prefix As String
Dim fn As String

prefix = SourceDir & SearchMask
Debug.Print prefix

fn = Dir(prefix)

Rename (fn)

Loop1:
fn = Dir
If fn = "" Then

    Offset = num
    Exit Sub
End If

Rename (fn)

GoTo Loop1

End Sub


Private Sub Command2_Click()
Dim num As Long
Dim prefix As String
Dim NewName As String
Dim fn As String

prefix = "K:\Photos\Scanned Slides\Random\File-*.tif"

fn = Dir(prefix)

num = 1

NewName = NewDirRoot & Format(num, "00") & ".tif"
fn = "K:\Photos\Scanned Slides\Random\" & fn
num = num + 1

'
Debug.Print fn, NewName

If TestFlag Then Exit Sub

Name fn As NewName

Loop1:
fn = Dir
If fn = "" Then

    Offset = num
    Exit Sub
End If


NewName = NewDirRoot & Format(num, "00") & ".tif"
fn = "K:\Photos\Scanned Slides\Random\" & fn
num = num + 1

'
Debug.Print fn, NewName

If TestFlag Then Exit Sub

Name fn As NewName

GoTo Loop1


End Sub



Public Sub Rename(fn)

Dim NewName As String

num = Mid(fn, 5, 4)
num = num + Offset

NewName = NewDirRoot & Format(num, "00") & DestSuffix
fn = SourceDir & fn

Debug.Print fn, NewName

If TestFlag Then Exit Sub

Name fn As NewName

End Sub

Private Sub Text1_Change()

End Sub


Private Sub Command3_Click()

Dim num As Long
Dim prefix As String
Dim NewName As String
Dim fn As String
Dim TestFlag As Boolean
TestFlag = True
prefix = SourceDir & "*.*"

fn = Dir(prefix)

num = 1

NewName = NewDirRoot & Format(num + Offset, "000") & ".jpg"
fn = SourceDir & fn
num = num + 1

'
Debug.Print fn, NewName

If TestFlag = True Then Name fn As NewName

Loop1:
fn = Dir
If fn = "" Then

    Offset = Offset + num
    Exit Sub
End If


NewName = NewDirRoot & Format(num + Offset, "000") & ".jpg"
fn = SourceDir & fn
num = num + 1

'
Debug.Print fn, NewName

If TestFlag = True Then Name fn As NewName

GoTo Loop1


End Sub






