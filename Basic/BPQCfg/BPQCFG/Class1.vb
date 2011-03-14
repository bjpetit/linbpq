Public Class NumTextBox

    Inherits TextBox

    Private MaxValue As Integer

    Sub New(ByVal Max As Integer)

        AddHandler Me.Validating, AddressOf ValidatingNumber
        AddHandler Me.Validated, AddressOf ValidatedSub
        Me.Max = Max

    End Sub

    Public Property Max() As Integer
        Get
            Max = MaxValue
        End Get
        Set(ByVal max As Integer)
            MaxValue = max
        End Set
    End Property

    Private Sub InitializeComponent()
        Me.SuspendLayout()
        Me.ResumeLayout(False)

    End Sub
End Class

Public Class NumOrEmptyTextBox

    Inherits TextBox

    Private MaxValue As Integer

   Sub New(ByVal Max As Integer)

      AddHandler Me.Validating, AddressOf ValidatingNumberorNull
      AddHandler Me.Validated, AddressOf ValidatedSub
      Me.Max = Max

   End Sub

    Public Property Max() As Integer
        Get
            Max = MaxValue
        End Get
        Set(ByVal max As Integer)
            MaxValue = max
        End Set
    End Property

    Private Sub InitializeComponent()
        Me.SuspendLayout()
        Me.ResumeLayout(False)

    End Sub
End Class
Public Class DTNumTextBox

    Inherits TextBox

    Private MaxValue As Integer

    Sub New()

        AddHandler Me.Validating, AddressOf ValidatingNumber
        AddHandler Me.Validated, AddressOf ValidatedSub

    End Sub

    Public Property Max() As Integer
        Get
            Max = MaxValue
        End Get
        Set(ByVal max As Integer)
            MaxValue = max
        End Set
    End Property

End Class

Public Class CallsignTextBox
    Inherits TextBox


    Sub New()

        AddHandler Me.Validating, AddressOf OptionalCallValidating
        AddHandler Me.Validated, AddressOf ValidatedSub

    End Sub


End Class

Public Class NonNullCombobox
   Inherits Combobox


   Sub New()

      AddHandler Me.Validating, AddressOf ValidatingCombo
      AddHandler Me.Validated, AddressOf ValidatedSub

   End Sub


End Class

Public Class AliasTextBox
    Inherits TextBox


    Sub New()

        AddHandler Me.Validating, AddressOf AliasValidating
        AddHandler Me.Validated, AddressOf ValidatedSub

    End Sub


End Class
Public Class ApplNameTextBox
    Inherits TextBox


    Sub New()

        AddHandler Me.Validating, AddressOf ApplValidating
        AddHandler Me.Validated, AddressOf ValidatedSub

    End Sub


End Class
Public Class MultiLineTextBox
    Inherits TextBox

    Private MaxValue As Integer

    Sub New()

        AddHandler Me.Validating, AddressOf MultiLineValidating
        AddHandler Me.Validated, AddressOf ValidatedSub

    End Sub

    Public Property MaxLen() As Integer
        Get
            MaxLen = MaxValue
        End Get
        Set(ByVal max As Integer)
            MaxValue = max
        End Set
    End Property

End Class
