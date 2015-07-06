Public Class ReedSoloman8
    ' status: Thoroughly checked for mm values of 4 and 8 across various TT values.

    '  Translation to VB.NET done by Rick Muething, Sept 17, 2008

    Private mm As Integer = 8           ' RS code over GF(2**4) - change to suit 
    Private nn As Integer = 255          ' nn=2**mm -1   length of codeword 
    Private tt As Integer = 0           ' number of errors that can be corrected 
    Private kk As Integer = nn - 2 * tt ' the number of information characters  kk = nn-2*tt
    Private pp4() As Integer = {1, 1, 0, 0, 1} 'specify irreducible polynomial coeffts */
    Private pp8() As Integer = {1, 0, 1, 1, 1, 0, 0, 0, 1} 'specify irreducible polynomial coeffts */
    Private pp() As Integer = pp8
    Private alpha_to(nn) As Integer
    Private index_of(nn) As Integer
    Private gg(nn - kk) As Integer
    Private blnErrorsCorrected As Boolean
    Private recd(nn - 1) As Integer
    Private data() As Integer
    Private bb((nn - kk) - 1) As Integer

    ' Property to set max # of corrections and initialize all arrays
    Public Property MaxCorrections() As Integer
        Get
            Return tt
        End Get
        Set(ByVal value As Integer)
            If value >= 1 And value <= (nn - 1) \ 2 Then
                tt = value
                kk = nn - 2 * tt
                ReDim gg(nn - kk)
                ReDim bb((nn - kk) - 1)
                ReDim data((nn - (2 * tt)) - 1) ' the max size of the encoding array
                generate_gf()
                gen_poly()
            End If
        End Set
    End Property  'MaxCorrections

    ' Subroutine to generate the GF
    Private Sub generate_gf()
        ' generate GF(2**mm) from the irreducible polynomial p(X) in pp[0]..pp[mm]
        '   lookup tables:  index->polynomial form   alpha_to[] contains j=alpha**i;
        '                   polynomial form -> index form  index_of[j=alpha**i] = i
        '   alpha=2 is the primitive element of GF(2**mm)

        Dim mask As Integer = 1 '  mask = 1 ;
        alpha_to(mm) = 0 '  alpha_to[mm] = 0 ;
        For i As Integer = 0 To mm - 1 '  for (i=0; i<mm; i++)
            alpha_to(i) = mask '  alpha_to[i] = mask 
            index_of(alpha_to(i)) = i '     index_of[alpha_to[i]] = i ;
            If pp(i) <> 0 Then '     if (pp[i]!=0)
                alpha_to(mm) = alpha_to(mm) Xor mask '       alpha_to[mm] ^= mask ;
            End If
            mask = 2 * mask  '     mask <<= 1 ;
        Next i

        index_of(alpha_to(mm)) = mm   '  index_of[alpha_to[mm]] = mm ;
        mask = mask \ 2 '  mask >>= 1 ;
        For i As Integer = (mm + 1) To (nn - 1)  '  for (i=mm+1; i<nn; i++)
            If alpha_to(i - 1) >= mask Then  '   { if (alpha_to[i-1] >= mask)
                alpha_to(i) = alpha_to(mm) Xor ((alpha_to(i - 1) Xor mask) * 2)  '        alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i-1]^mask)<<1) ;
            Else
                alpha_to(i) = alpha_to(i - 1) * 2 '     else alpha_to[i] = alpha_to[i-1]<<1 ;
            End If
            index_of(alpha_to(i)) = i 'index_of[alpha_to[i]] = i ;
        Next i '   }
        index_of(0) = -1 '  index_of[0] = -1 ;
    End Sub ' generate_gf

    ' Subroutine to generate the polynomial
    Private Sub gen_poly()   'void gen_poly()
        ' Obtain the generator polynomial of the tt-error correcting, length
        '  nn=(2**mm -1) Reed Solomon code  from the product of (X+alpha**i), i=1..2*tt
        Dim i, j As Integer '   register int i,j ;

        gg(0) = 2  '   gg[0] = 2 ;    /* primitive element alpha = 2  for GF(2**mm)  */
        gg(1) = 1  '   gg[1] = 1 ;    /* g(x) = (X+alpha) initially */
        For i = 2 To nn - kk '   for (i=2; i<=nn-kk; i++)
            gg(i) = 1 '    { gg[i] = 1 ;
            For j = i - 1 To 1 Step -1 '      for (j=i-1; j>0; j--)
                If gg(j) <> 0 Then '        if (gg[j] != 0) 
                    gg(j) = gg(j - 1) Xor alpha_to((index_of(gg(j)) + i) Mod nn)   'gg[j] = gg[j-1]^ alpha_to[(index_of[gg[j]]+i)%nn] ;
                Else
                    gg(j) = gg(j - 1) '        else gg[j] = gg[j-1] ;
                End If
            Next j
            gg(0) = alpha_to((index_of(gg(0)) + i) Mod nn)  '      gg[0] = alpha_to[(index_of[gg[0]]+i)%nn] ;     /* gg[0] can never be zero */
        Next i '    }
        '   convert gg[] to index form for quicker encoding 

        For i = 0 To nn - kk '   for (i=0; i<=nn-kk; i++) 
            gg(i) = index_of(gg(i))  ' gg[i] = index_of[gg[i]] ;
        Next i
    End Sub ' gen_poly    ' }

    ' Subroutine to Encode Reed Solomon
    Private Sub encode_rs()      'void encode_rs()
        '   take the string of symbols in data[i], i=0..(k-1) and encode systematically
        '   to produce 2*tt parity symbols in bb[0]..bb[2*tt-1]
        '   data[] is input and bb[] is output in polynomial form.
        '   Encoding is done by using a feedback shift register with appropriate
        '   connections specified by the elements of gg[], which was generated above.
        '   Codeword is   c(X) = data(X)*X**(nn-kk)+ b(X)

        Dim i, j As Integer '   register int i,j ;
        Dim feedback As Integer '   int feedback ;


        For i = 0 To (nn - kk) - 1 '   for (i=0; i<nn-kk; i++)
            bb(i) = 0 ' bb[i] = 0 ;
        Next i

        For i = kk - 1 To 0 Step -1 '   for (i=kk-1; i>=0; i--)
            feedback = index_of(data(i) Xor bb((nn - kk) - 1)) '    {  feedback = index_of[data[i]^bb[nn-kk-1]] ;
            If feedback <> -1 Then '       if (feedback != -1)
                For j = (nn - kk) - 1 To 1 Step -1 '        { for (j=nn-kk-1; j>0; j--)
                    If gg(j) <> -1 Then '            if (gg[j] != -1)
                        bb(j) = bb(j - 1) Xor alpha_to((gg(j) + feedback) Mod nn)  '              bb[j] = bb[j-1]^alpha_to[(gg[j]+feedback)%nn] ;
                    Else        '            else
                        bb(j) = bb(j - 1)  '              bb[j] = bb[j-1] ;
                    End If
                Next j
                bb(0) = alpha_to((gg(0) + feedback) Mod nn) '          bb[0] = alpha_to[(gg[0]+feedback)%nn] ;
                '        }
            Else '       else
                For j = (nn - kk) - 1 To 1 Step -1 '        { for (j=nn-kk-1; j>0; j--)
                    bb(j) = bb(j - 1) '            bb[j] = bb[j-1] ;
                Next j
                bb(0) = 0 '          bb[0] = 0 ;
            End If '        } ;
        Next i  '    } ;
    End Sub ' encode_rs ' } ;

    ' Subroutine to Decode RS
    Private Sub decode_rs()
        '   assume we have received bits grouped into mm-bit symbols in recd[i],
        '   i=0..(nn-1),  and recd[i] is index form (ie as powers of alpha).
        '   We first compute the 2*tt syndromes by substituting alpha**i into rec(X) and
        '   evaluating, storing the syndromes in s[i], i=1..2tt (leave s[0] zero) .
        '   Then we use the Berlekamp iteration to find the error location polynomial
        '   elp[i].   If the degree of the elp is >tt, we cannot correct all the errors
        '   and hence just put out the information symbols uncorrected. If the degree of
        '   elp is <=tt, we substitute alpha**i , i=1..n into the elp to get the roots,
        '   hence the inverse roots, the error location numbers. If the number of errors
        '   located does not equal the degree of the elp, we have more than tt errors
        '   and cannot correct them.  Otherwise, we then solve for the error value at
        '   the error location and correct the error.  The procedure is that found in
        '   Lin and Costello. For the cases where the number of errors is known to be too
        '   large to correct, the information symbols as received are output (the
        '   advantage of systematic encoding is that hopefully some of the information
        '   symbols will be okay and that if we are in luck, the errors are in the
        '   parity part of the transmitted codeword).  Of course, these insoluble cases
        '   can be returned as error flags to the calling routine if desired.   

        Dim i, j, u, q As Integer '   register int i,j,u,q ;
        Dim elp(nn - kk + 1, nn - kk - 1) As Integer      '   int elp[nn-kk+2][nn-kk]
        Dim d(nn - kk + 1) As Integer ' d[nn-kk+2]
        Dim l(nn - kk + 1) As Integer ' l[nn-kk+2]
        Dim u_lu(nn - kk + 1) As Integer ' u_lu[nn-kk+2]
        Dim s(nn - kk + 1) As Integer ' s[nn-kk+1] ;
        Dim count As Integer = 0   '   int count=0
        Dim syn_error As Integer = 0 ' syn_error=0
        Dim root(tt - 1) As Integer '  root[tt]
        Dim loc(tt - 1) As Integer '  loc[tt]
        Dim z(tt) As Integer ' z[tt+1]
        Dim err(nn - 1) As Integer ' err[nn]
        Dim reg(tt) As Integer '  reg[tt+1] ;

        Try
            ' first form the syndromes 

            For i = 1 To nn - kk  '   for (i=1; i<=nn-kk; i++)
                s(i) = 0 '    { s[i] = 0 ;
                For j = 0 To nn - 1 '      for (j=0; j<nn; j++)
                    If recd(j) <> -1 Then '       if (recd[j]!=-1)
                        s(i) = s(i) Xor alpha_to((recd(j) + i * j) Mod nn) '  s[i] ^= alpha_to[(recd[j]+i*j)%nn] ;      /* recd[j] in index form */
                    End If
                Next j
                ' convert syndrome from polynomial form to index form  
                If s(i) <> 0 Then syn_error = 1 '      if (s[i]!=0)  syn_error=1 ;        /* set flag if non-zero syndrome => error */
                s(i) = index_of(s(i)) '      s[i] = index_of[s[i]] ;
            Next i '    } ;

            If syn_error = 1 Then '   if (syn_error)       if errors, try and correct 
                '   compute the error location polynomial via the Berlekamp iterative algorithm,
                '   following the terminology of Lin and Costello :   d[u] is the 'mu'th
                '   discrepancy, where u='mu'+1 and 'mu' (the Greek letter!) is the step number
                '   ranging from -1 to 2*tt (see L&C),  l[u] is the
                '   degree of the elp at that step, and u_l[u] is the difference between the
                '   step number and the degree of the elp.
                '
                ' initialise table entries 
                d(0) = 0 '      d[0] = 0 ;           /* index form */
                d(1) = s(1) '      d[1] = s[1] ;        /* index form */
                elp(0, 0) = 0 '      elp[0][0] = 0 ;      /* index form */
                elp(1, 0) = 1 '      elp[1][0] = 1 ;      /* polynomial form */
                For i = 1 To nn - kk - 1 '      for (i=1; i<nn-kk; i++)
                    elp(0, i) = -1 '        { elp[0][i] = -1 ;   /* index form */
                    elp(1, i) = 0  '          elp[1][i] = 0 ;   /* polynomial form */
                Next i '        }
                l(0) = 0  '      l[0] = 0 ;
                l(1) = 0  '      l[1] = 0 ;
                u_lu(0) = -1 '      u_lu[0] = -1 ;
                u_lu(1) = 0  '      u_lu[1] = 0 ;
                u = 0  '      u = 0 ;
                Do '      do

                    u += 1 '        u++ ;
                    If (d(u) = -1) Then '        if (d[u]==-1)
                        l(u + 1) = l(u) '          { l[u+1] = l[u] ;
                        For i = 0 To l(u) '            for (i=0; i<=l[u]; i++)
                            elp(u + 1, i) = elp(u, i) '             {  elp[u+1][i] = elp[u][i] ;
                            elp(u, i) = index_of(elp(u, i)) '                elp[u][i] = index_of[elp[u][i]] ;
                        Next i  '             }
                        '          }
                    Else '        else
                        ' search for words with greatest u_lu[q] for which d[q]!=0 
                        q = u - 1 '          { q = u-1 ;
                        While (d(q) = -1 And q > 0) ' while ((d[q]==-1) && (q>0)) q-- ;
                            q -= 1
                        End While
                        ' have found first non-zero d[q]  
                        If q > 0 Then '            if (q>0)
                            j = q '             { j=q ;
                            Do '               do
                                j -= 1 '               { j-- ;
                                If ((d(j) <> -1) And (u_lu(q) < u_lu(j))) Then ' if ((d[j]!=-1) && (u_lu[q]<u_lu[j]))
                                    q = j '                   q = j ;
                                End If
                                If j <= 0 Then Exit Do '               }while (j>0) ;
                            Loop
                        End If '             } ;

                        ' have now found q such that d[u]!=0 and u_lu[q] is maximum 
                        ' store degree of new elp polynomial 
                        If (l(u) > l(q) + u - q) Then '            if (l[u]>l[q]+u-q)  l[u+1] = l[u] ;
                            l(u + 1) = l(u)
                        Else '        else  
                            l(u + 1) = l(q) + u - q 'l[u+1] = l[q]+u-q ;
                        End If

                        '/* form new elp(x) */
                        For i = 0 To nn - kk - 1 ' for (i=0; i<nn-kk; i++)
                            elp(u + 1, i) = 0 '  elp[u+1][i] = 0 ;
                        Next i
                        For i = 0 To l(q) ' for (i=0; i<=l[q]; i++)
                            If (elp(q, i) <> -1) Then '              if (elp[q][i]!=-1)
                                elp(u + 1, i + u - q) = alpha_to((d(u) + nn - d(q) + elp(q, i)) Mod nn) ' elp[u+1][i+u-q] = alpha_to[(d[u]+nn-d[q]+elp[q][i])%nn] ;
                            End If
                        Next i
                        For i = 0 To l(u) ' for (i=0; i<=l[u]; i++)
                            elp(u + 1, i) = elp(u + 1, i) Xor elp(u, i) ' { elp[u+1][i] ^= elp[u][i] ;
                            elp(u, i) = index_of(elp(u, i)) ' elp[u][i] = index_of[elp[u][i]] ;  /*convert old elp value to index*/
                        Next i '              }
                    End If '          }
                    u_lu(u + 1) = u - l(u + 1) ' u_lu[u+1] = u-l[u+1] ;

                    ' form (u+1)th discrepancy 
                    If (u < nn - kk) Then ' if (u<nn-kk)    no discrepancy computed on last iteration
                        If (s(u + 1) <> -1) Then ' if (s[u+1]!=-1)
                            d(u + 1) = alpha_to(s(u + 1)) ' d[u+1] = alpha_to[s[u+1]] ;
                        Else ' else
                            d(u + 1) = 0 ' d[u+1] = 0 ;
                        End If
                        For i = 1 To l(u + 1) ' for (i=1; i<=l[u+1]; i++)
                            If ((s(u + 1 - i) <> -1) And (elp(u + 1, i) <> 0)) Then ' if ((s[u+1-i]!=-1) && (elp[u+1][i]!=0)) 
                                d(u + 1) = d(u + 1) Xor alpha_to((s(u + 1 - i) + index_of(elp(u + 1, i))) Mod nn) ' d[u+1] ^= alpha_to[(s[u+1-i]+index_of[elp[u+1][i]])%nn] ;
                            End If
                        Next i
                        d(u + 1) = index_of(d(u + 1)) '  d[u+1] = index_of[d[u+1]] ;    /* put d[u+1] into index form */
                    End If '          }
                    If Not ((u < nn - kk) And (l(u + 1) <= tt)) Then Exit Do '      } while ((u<nn-kk) && (l[u+1]<=tt)) ;
                Loop
                u += 1 '      u++ ;
                If l(u) <= tt Then '      if (l[u]<=tt)         /* can correct error */
                    '       {
                    ' put elp into index form 
                    For i = 0 To l(u) '         for (i=0; i<=l[u]; i++) 
                        elp(u, i) = index_of(elp(u, i)) ' elp[u][i] = index_of[elp[u][i]] ;
                    Next

                    ' find roots of the error location polynomial 
                    For i = 1 To l(u) ' for (i=1; i<=l[u]; i++)
                        reg(i) = elp(u, i) ' reg[i] = elp[u][i] ;
                    Next i
                    count = 0 ' count = 0 ;
                    For i = 1 To nn ' for (i=1; i<=nn; i++)
                        q = 1 ' {  q = 1 ;
                        For j = 1 To l(u) ' for (j=1; j<=l[u]; j++)
                            If (reg(j) <> -1) Then ' if (reg[j]!=-1)
                                reg(j) = (reg(j) + j) Mod nn '  { reg[j] = (reg[j]+j)%nn ;
                                q = q Xor alpha_to(reg(j)) ' q ^= alpha_to[reg[j]] ;
                            End If ' } ;
                        Next j
                        If q = 0 Then 'if (!q)        store root and error location number indices 
                            root(count) = i '{ root[count] = i;
                            loc(count) = nn - i ' loc[count] = nn-i ;
                            count += 1 'count++ ;
                        End If ' };
                    Next i '} ;
                    If count = l(u) Then ' if (count==l[u])     no. roots = degree of elp hence <= tt errors
                        blnErrorsCorrected = True
                        '          {
                        ' form polynomial z(x) 
                        For i = 1 To l(u) 'for (i=1; i<=l[u]; i++)         Z[0] = 1 always - do not need */
                            If ((s(i) <> -1) And (elp(u, i) <> -1)) Then '{ if ((s[i]!=-1) && (elp[u][i]!=-1))
                                z(i) = alpha_to(s(i)) Xor alpha_to(elp(u, i)) ' z[i] = alpha_to[s[i]] ^ alpha_to[elp[u][i]] ;
                            ElseIf ((s(i) <> -1) And (elp(u, i) = -1)) Then '  else if ((s[i]!=-1) && (elp[u][i]==-1))
                                z(i) = alpha_to(s(i)) 'z[i] = alpha_to[s[i]] ;
                            ElseIf ((s(i) = -1) And (elp(u, i) <> -1)) Then ' else if ((s[i]==-1) && (elp[u][i]!=-1))
                                z(i) = alpha_to(elp(u, i))   ' z[i] = alpha_to[elp[u][i]] ;
                            Else ' else
                                z(i) = 0 'z[i] = 0 ;
                            End If
                            For j = 1 To i - 1 ' for (j=1; j<i; j++)
                                If ((s(j) <> -1) And (elp(u, i - j) <> -1)) Then ' if ((s[j]!=-1) && (elp[u][i-j]!=-1))
                                    z(i) = z(i) Xor alpha_to((elp(u, i - j) + s(j)) Mod nn) ' z[i] ^= alpha_to[(elp[u][i-j] + s[j])%nn] ;
                                End If
                            Next j
                            z(i) = index_of(z(i)) ' z[i] = index_of[z[i]] ;         put into index form
                        Next i ' } ;

                        '   evaluate errors at locations given by error location numbers loc[i] 
                        For i = 0 To nn - 1 '           for (i=0; i<nn; i++)
                            err(i) = 0 '             { err[i] = 0 ;
                            If (recd(i) <> -1) Then ' if (recd[i]!=-1)        /* convert recd[] to polynomial form */
                                recd(i) = alpha_to(recd(i)) ' recd[i] = alpha_to[recd[i]] ;
                            Else  ' else  
                                recd(i) = 0 ' recd[i] = 0 ;
                            End If
                        Next i ' }
                        For i = 0 To l(u) - 1 ' for (i=0; i<l[u]; i++)    compute numerator of error term first */
                            err(loc(i)) = 1 ' { err[loc[i]] = 1;       /* accounts for z[0] */
                            For j = 1 To l(u) ' for (j=1; j<=l[u]; j++)
                                If z(j) <> -1 Then ' if (z[j]!=-1)
                                    err(loc(i)) = err(loc(i)) Xor alpha_to((z(j) + j * root(i)) Mod nn) '                  err[loc[i]] ^= alpha_to[(z[j]+j*root[i])%nn] ;
                                End If
                            Next j
                            If (err(loc(i)) <> 0) Then '              if (err[loc[i]]!=0)
                                err(loc(i)) = index_of(err(loc(i))) '               { err[loc[i]] = index_of[err[loc[i]]] ;
                                q = 0 '                 q = 0 ;     /* form denominator of error term */
                                For j = 0 To l(u) - 1 '                 for (j=0; j<l[u]; j++)
                                    If j <> i Then q += index_of(1 Xor alpha_to((loc(j) + root(i)) Mod nn)) ' if (j!=i) q += index_of[1^alpha_to[(loc[j]+root[i])%nn]] ;
                                Next j
                                '                   
                                '                     
                                q = q Mod nn '                 q = q % nn ;
                                err(loc(i)) = alpha_to((err(loc(i)) - q + nn) Mod nn) '                 err[loc[i]] = alpha_to[(err[loc[i]]-q+nn)%nn] ;
                                recd(loc(i)) = recd(loc(i)) Xor err(loc(i)) '                 recd[loc[i]] ^= err[loc[i]] ;  /*recd[i] must be in polynomial form */
                            End If '               }
                        Next i '            }
                        '          }
                    Else '         else    /* no. roots != degree of elp => >tt errors and cannot solve */
                        blnErrorsCorrected = False
                        For i = 0 To nn - 1 '           for (i=0; i<nn; i++)        /* could return error flag if desired */
                            If (recd(i) <> -1) Then '               if (recd[i]!=-1)        /* convert recd[] to polynomial form */
                                recd(i) = alpha_to(recd(i)) '                 recd[i] = alpha_to[recd[i]] ;
                            Else '               else  recd[i] = 0 ;     /* just output received codeword as is */
                                recd(i) = 0 'recd[i] = 0 ;     /* just output received codeword as is */
                            End If
                        Next i
                    End If '       }
                Else '     else         /* elp has degree has degree >tt hence cannot solve */
                    blnErrorsCorrected = False
                    For i = 0 To nn - 1 '       for (i=0; i<nn; i++)       /* could return error flag if desired */
                        If (recd(i) <> -1) Then '          if (recd[i]!=-1)        /* convert recd[] to polynomial form */
                            recd(i) = alpha_to(recd(i)) '            recd[i] = alpha_to[recd[i]] ;
                        Else '          else  
                            recd(i) = 0 ' recd[i] = 0 ;     /* just output received codeword as is */
                        End If '    }
                    Next i
                End If
            Else '   else       /* no non-zero syndromes => no errors: output received codeword */
                blnErrorsCorrected = True
                For i = 0 To nn - 1 '    for (i=0; i<nn; i++)
                    If recd(i) <> -1 Then '       if (recd[i]!=-1)        /* convert recd[] to polynomial form */
                        recd(i) = alpha_to(recd(i)) '         recd[i] = alpha_to[recd[i]] ;
                    Else
                        recd(i) = 0 '       else  recd[i] = 0 ;
                    End If ' }
                Next i
            End If
        Catch ex As Exception
            Logs.Exception("[decode_rs] Err: " & ex.ToString)
        End Try
    End Sub ' decode_rs

    ' subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)
    Public Function RSEncode(ByVal bytData() As Byte) As Byte()
        ' bytData is the symbol array to encode. If less than full RS length it is padded with leading 0's (shortened RS code)
        If bytData.Length > data.Length Then Return Nothing ' too long of data
        Dim intStartIndex As Integer = data.Length - bytData.Length ' set the start point for shortened RS codes

        For i As Integer = 0 To bytData.Length - 1
            data(i + intStartIndex) = bytData(i)
        Next i
        encode_rs() 'create the parity symbols

        Dim bytTemp(bytData.Length + bb.Length - 1) As Byte
        For i As Integer = 0 To bytTemp.Length - 1
            If i < bytData.Length Then
                bytTemp(i) = CByte(data(i + intStartIndex)) ' copy the data including stuffed leading zeros 
            Else
                bytTemp(i) = CByte(bb(i - (data.Length - intStartIndex))) ' append the parity symbols
            End If
        Next
        Return bytTemp
    End Function 'RSEncode

    ' Main RS decode function
    Public Function RSDecode(ByVal bytRcv() As Byte, Optional ByRef blnCorrected As Boolean = False) As Byte()
        ' bytRcv is the received array of symbols (up to 8 bits/symbol) including systematic data + 2 * tt parity symbols
        ' length of bytRcv may be shorter than full length (nn) but must be agreed upon a priori by both encoder and decoder (Shortened code)
        ' Shortening simply appends leading 0's (before encoding and before decoding) and then 
        ' discarding the same 0's at the end.
        ' This verified with several thousand automated random test cases for both 239,255 and 223, 255 RS codes
        ' approximate encode + decode time (with max num of errors) is on the order of 1 ms (Pentium 1700 MHz) 
        ' Note on some intensive test done July 5, 2009 by RM (> 20 Million RS Corrections) there are rare (but possible) conditions
        ' where RSDecode reports blnCorrected = true but there are still bytes with errors. The exact 
        ' mechanism for this is unknown so this now requires a Sum Check confirmation even if the blnCorrected
        ' is reported True. In over 20 Million test cases there was no condition that blnCorrected was reported
        ' true, there was an error, and the 16 bit Sum check did not detect the error.
        'SyncLock objRSLock
        Dim intTrace As Integer = 0
        Dim intIndexSave As Integer
        Dim intIsave As Integer
        Dim intTemp(nn - 1) As Integer
        Try
            Dim blnDecodeFailed As Boolean = False
            blnErrorsCorrected = False
            If bytRcv.Length > nn Or bytRcv.Length < (1 + 2 * tt) Then
                blnCorrected = False
                'ReDim Preserve bytRcv(bytRcv.Length - 2 * tt - 1)
                '  RSDecode = bytRcv 'insufficient length
                Logs.Exception("[ReedSoloman8.RSDecode8] Length Error Rcv:" & bytRcv.Length.ToString & " nn:" & nn.ToString & " tt:" & tt.ToString & "  Return empty array.")
                Dim bytNull(-1) As Byte
                Return bytNull
            End If
            intTrace = 1
            Dim intStartIndex As Integer = nn - bytRcv.Length ' set the start point for shortened RS codes

            For j As Integer = 0 To nn - 1
                If j < intStartIndex Then
                    intTemp(j) = 0
                Else
                    intTemp(j) = bytRcv(j - intStartIndex)
                End If
            Next j

            intTrace = 2
            ' convert to indexed form

            For i As Integer = 0 To recd.Length - 1
                intIsave = i
                intIndexSave = index_of(intTemp(i))
                recd(i) = index_of(intTemp(i))
            Next
            intTrace = 3
            decode_rs()
            intTrace = 4
            blnCorrected = blnErrorsCorrected
            If blnErrorsCorrected Then
                Dim bytTemp(bytRcv.Length - 2 * tt - 1) As Byte
                For i As Integer = 0 To bytTemp.Length - 1
                    bytTemp(i) = CByte(recd(i + intStartIndex))
                Next
                intTrace = 5
                blnCorrected = blnErrorsCorrected
                RSDecode = bytTemp
            Else
                ReDim Preserve bytRcv(bytRcv.Length - 2 * tt - 1)
                RSDecode = bytRcv
            End If
        Catch ex As Exception
            Logs.Exception("[ReedSoloman8.RSDecode] Exception Return input.  i: " & intIsave.ToString & " Index: " & intIndexSave.ToString & " Err: " & ex.ToString)
            blnCorrected = False
            ReDim Preserve bytRcv(bytRcv.Length - 2 * tt - 1)
            RSDecode = bytRcv
        End Try
        'End SyncLock
    End Function  'RSDecode

    ' This is a Reed-Solomon class to encode and decode Reed-Solomon codes. This is adapted
    ' from the C code of  Simon Rockliff, 26th June 1991. For reference the entire C code 
    ' unchanged is contained in the following comments:
    '***************************************************************************************
    '/*             rs.c        */
    '/* This program is an encoder/decoder for Reed-Solomon codes. Encoding is in
    '   systematic form, decoding via the Berlekamp iterative algorithm.
    '   In the present form , the constants mm, nn, tt, and kk=nn-2tt must be
    '   specified  (the double letters are used simply to avoid clashes with
    '   other n,k,t used in other programs into which this was incorporated!)
    '   Also, the irreducible polynomial used to generate GF(2**mm) must also be
    '   entered -- these can be found in Lin and Costello, and also Clark and Cain.

    '   The representation of the elements of GF(2**m) is either in index form,
    '   where the number is the power of the primitive element alpha, which is
    '   convenient for multiplication (add the powers modulo 2**m-1) or in
    '   polynomial form, where the bits represent the coefficients of the
    '   polynomial representation of the number, which is the most convenient form
    '   for addition.  The two forms are swapped between via lookup tables.
    '   This leads to fairly messy looking expressions, but unfortunately, there
    '   is no easy alternative when working with Galois arithmetic.

    '   The code is not written in the most elegant way, but to the best
    '   of my knowledge, (no absolute guarantees!), it works.
    '   However, when including it into a simulation program, you may want to do
    '   some conversion of global variables (used here because I am lazy!) to
    '   local variables where appropriate, and passing parameters (eg array
    '   addresses) to the functions  may be a sensible move to reduce the number
    '   of global variables and thus decrease the chance of a bug being introduced.

    '   This program does not handle erasures at present, but should not be hard
    '   to adapt to do this, as it is just an adjustment to the Berlekamp-Massey
    '   algorithm. It also does not attempt to decode past the BCH bound -- see
    '   Blahut "Theory and practice of error control codes" for how to do this.

    '              Simon Rockliff, University of Adelaide   21/9/89

    '   26/6/91 Slight modifications to remove a compiler dependent bug which hadn't
    '           previously surfaced. A few extra comments added for clarity.
    '           Appears to all work fine, ready for posting to net!

    '                  Notice
    '                 --------
    '   This program may be freely modified and/or given to whoever wants it.
    '   A condition of such distribution is that the author's contribution be
    '   acknowledged by his name being left in the comments heading the program,
    '   however no responsibility is accepted for any financial or other loss which
    '   may result from some unforseen errors or malfunctioning of the program
    '   during use.
    '                                 Simon Rockliff, 26th June 1991
    '*/

    '#include <math.h>
    '#include <stdio.h>
    '#define mm  4           /* RS code over GF(2**4) - change to suit */
    '#define nn  15          /* nn=2**mm -1   length of codeword */
    '#define tt  3           /* number of errors that can be corrected */
    '#define kk  9           /* kk = nn-2*tt  */

    'int pp [mm+1] = { 1, 1, 0, 0, 1} ; /* specify irreducible polynomial coeffts */
    'int alpha_to [nn+1], index_of [nn+1], gg [nn-kk+1] ;
    'int recd [nn], data [kk], bb [nn-kk] ;


    'void generate_gf()
    '/* generate GF(2**mm) from the irreducible polynomial p(X) in pp[0]..pp[mm]
    '   lookup tables:  index->polynomial form   alpha_to[] contains j=alpha**i;
    '                   polynomial form -> index form  index_of[j=alpha**i] = i
    '   alpha=2 is the primitive element of GF(2**mm)
    '*/
    ' {
    '   register int i, mask ;

    '  mask = 1 ;
    '  alpha_to[mm] = 0 ;
    '  for (i=0; i<mm; i++)
    '   { alpha_to[i] = mask ;
    '     index_of[alpha_to[i]] = i ;
    '     if (pp[i]!=0)
    '       alpha_to[mm] ^= mask ;
    '     mask <<= 1 ;
    '   }
    '  index_of[alpha_to[mm]] = mm ;
    '  mask >>= 1 ;
    '  for (i=mm+1; i<nn; i++)
    '   { if (alpha_to[i-1] >= mask)
    '        alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i-1]^mask)<<1) ;
    '     else alpha_to[i] = alpha_to[i-1]<<1 ;
    '     index_of[alpha_to[i]] = i ;
    '   }
    '  index_of[0] = -1 ;
    ' }


    'void gen_poly()
    '/* Obtain the generator polynomial of the tt-error correcting, length
    '  nn=(2**mm -1) Reed Solomon code  from the product of (X+alpha**i), i=1..2*tt
    '*/
    ' {
    '   register int i,j ;

    '   gg[0] = 2 ;    /* primitive element alpha = 2  for GF(2**mm)  */
    '   gg[1] = 1 ;    /* g(x) = (X+alpha) initially */
    '   for (i=2; i<=nn-kk; i++)
    '    { gg[i] = 1 ;
    '      for (j=i-1; j>0; j--)
    '        if (gg[j] != 0)  gg[j] = gg[j-1]^ alpha_to[(index_of[gg[j]]+i)%nn] ;
    '        else gg[j] = gg[j-1] ;
    '      gg[0] = alpha_to[(index_of[gg[0]]+i)%nn] ;     /* gg[0] can never be zero */
    '    }
    '   /* convert gg[] to index form for quicker encoding */
    '   for (i=0; i<=nn-kk; i++)  gg[i] = index_of[gg[i]] ;
    ' }


    'void encode_rs()
    '/* take the string of symbols in data[i], i=0..(k-1) and encode systematically
    '   to produce 2*tt parity symbols in bb[0]..bb[2*tt-1]
    '   data[] is input and bb[] is output in polynomial form.
    '   Encoding is done by using a feedback shift register with appropriate
    '   connections specified by the elements of gg[], which was generated above.
    '   Codeword is   c(X) = data(X)*X**(nn-kk)+ b(X)          */
    ' {
    '   register int i,j ;
    '   int feedback ;

    '   for (i=0; i<nn-kk; i++)   bb[i] = 0 ;
    '   for (i=kk-1; i>=0; i--)
    '    {  feedback = index_of[data[i]^bb[nn-kk-1]] ;
    '       if (feedback != -1)
    '        { for (j=nn-kk-1; j>0; j--)
    '            if (gg[j] != -1)
    '              bb[j] = bb[j-1]^alpha_to[(gg[j]+feedback)%nn] ;
    '            else
    '              bb[j] = bb[j-1] ;
    '          bb[0] = alpha_to[(gg[0]+feedback)%nn] ;
    '        }
    '       else
    '        { for (j=nn-kk-1; j>0; j--)
    '            bb[j] = bb[j-1] ;
    '          bb[0] = 0 ;
    '        } ;
    '    } ;
    ' } ;



    'void decode_rs()
    '/* assume we have received bits grouped into mm-bit symbols in recd[i],
    '   i=0..(nn-1),  and recd[i] is index form (ie as powers of alpha).
    '   We first compute the 2*tt syndromes by substituting alpha**i into rec(X) and
    '   evaluating, storing the syndromes in s[i], i=1..2tt (leave s[0] zero) .
    '   Then we use the Berlekamp iteration to find the error location polynomial
    '   elp[i].   If the degree of the elp is >tt, we cannot correct all the errors
    '   and hence just put out the information symbols uncorrected. If the degree of
    '   elp is <=tt, we substitute alpha**i , i=1..n into the elp to get the roots,
    '   hence the inverse roots, the error location numbers. If the number of errors
    '   located does not equal the degree of the elp, we have more than tt errors
    '   and cannot correct them.  Otherwise, we then solve for the error value at
    '   the error location and correct the error.  The procedure is that found in
    '   Lin and Costello. For the cases where the number of errors is known to be too
    '   large to correct, the information symbols as received are output (the
    '   advantage of systematic encoding is that hopefully some of the information
    '   symbols will be okay and that if we are in luck, the errors are in the
    '   parity part of the transmitted codeword).  Of course, these insoluble cases
    '   can be returned as error flags to the calling routine if desired.   */
    ' {
    '   register int i,j,u,q ;
    '   int elp[nn-kk+2][nn-kk], d[nn-kk+2], l[nn-kk+2], u_lu[nn-kk+2], s[nn-kk+1] ;
    '   int count=0, syn_error=0, root[tt], loc[tt], z[tt+1], err[nn], reg[tt+1] ;

    '/* first form the syndromes */
    '   for (i=1; i<=nn-kk; i++)
    '    { s[i] = 0 ;
    '      for (j=0; j<nn; j++)
    '        if (recd[j]!=-1)
    '          s[i] ^= alpha_to[(recd[j]+i*j)%nn] ;      /* recd[j] in index form */
    '/* convert syndrome from polynomial form to index form  */
    '      if (s[i]!=0)  syn_error=1 ;        /* set flag if non-zero syndrome => error */
    '      s[i] = index_of[s[i]] ;
    '    } ;

    '   if (syn_error)       /* if errors, try and correct */
    '    {
    '/* compute the error location polynomial via the Berlekamp iterative algorithm,
    '   following the terminology of Lin and Costello :   d[u] is the 'mu'th
    '   discrepancy, where u='mu'+1 and 'mu' (the Greek letter!) is the step number
    '   ranging from -1 to 2*tt (see L&C),  l[u] is the
    '   degree of the elp at that step, and u_l[u] is the difference between the
    '   step number and the degree of the elp.
    '*/
    '/* initialise table entries */
    '      d[0] = 0 ;           /* index form */
    '      d[1] = s[1] ;        /* index form */
    '      elp[0][0] = 0 ;      /* index form */
    '      elp[1][0] = 1 ;      /* polynomial form */
    '      for (i=1; i<nn-kk; i++)
    '        { elp[0][i] = -1 ;   /* index form */
    '          elp[1][i] = 0 ;   /* polynomial form */
    '        }
    '      l[0] = 0 ;
    '      l[1] = 0 ;
    '      u_lu[0] = -1 ;
    '      u_lu[1] = 0 ;
    '      u = 0 ;

    '      do
    '      {
    '        u++ ;
    '        if (d[u]==-1)
    '          { l[u+1] = l[u] ;
    '            for (i=0; i<=l[u]; i++)
    '             {  elp[u+1][i] = elp[u][i] ;
    '                elp[u][i] = index_of[elp[u][i]] ;
    '             }
    '          }
    '        else
    '/* search for words with greatest u_lu[q] for which d[q]!=0 */
    '          { q = u-1 ;
    '            while ((d[q]==-1) && (q>0)) q-- ;
    '/* have found first non-zero d[q]  */
    '            if (q>0)
    '             { j=q ;
    '               do
    '               { j-- ;
    '                 if ((d[j]!=-1) && (u_lu[q]<u_lu[j]))
    '                   q = j ;
    '               }while (j>0) ;
    '             } ;

    '/* have now found q such that d[u]!=0 and u_lu[q] is maximum */
    '/* store degree of new elp polynomial */
    '            if (l[u]>l[q]+u-q)  l[u+1] = l[u] ;
    '            else  l[u+1] = l[q]+u-q ;

    '/* form new elp(x) */
    '            for (i=0; i<nn-kk; i++)    elp[u+1][i] = 0 ;
    '            for (i=0; i<=l[q]; i++)
    '              if (elp[q][i]!=-1)
    '                elp[u+1][i+u-q] = alpha_to[(d[u]+nn-d[q]+elp[q][i])%nn] ;
    '            for (i=0; i<=l[u]; i++)
    '              { elp[u+1][i] ^= elp[u][i] ;
    '                elp[u][i] = index_of[elp[u][i]] ;  /*convert old elp value to index*/
    '              }
    '          }
    '        u_lu[u+1] = u-l[u+1] ;

    '/* form (u+1)th discrepancy */
    '        if (u<nn-kk)    /* no discrepancy computed on last iteration */
    '          {
    '            if (s[u+1]!=-1)
    '                   d[u+1] = alpha_to[s[u+1]] ;
    '            else
    '              d[u+1] = 0 ;
    '            for (i=1; i<=l[u+1]; i++)
    '              if ((s[u+1-i]!=-1) && (elp[u+1][i]!=0))
    '                d[u+1] ^= alpha_to[(s[u+1-i]+index_of[elp[u+1][i]])%nn] ;
    '            d[u+1] = index_of[d[u+1]] ;    /* put d[u+1] into index form */
    '          }
    '      } while ((u<nn-kk) && (l[u+1]<=tt)) ;

    '      u++ ;
    '      if (l[u]<=tt)         /* can correct error */
    '       {
    '/* put elp into index form */
    '         for (i=0; i<=l[u]; i++)   elp[u][i] = index_of[elp[u][i]] ;

    '/* find roots of the error location polynomial */
    '         for (i=1; i<=l[u]; i++)
    '           reg[i] = elp[u][i] ;
    '         count = 0 ;
    '         for (i=1; i<=nn; i++)
    '          {  q = 1 ;
    '             for (j=1; j<=l[u]; j++)
    '              if (reg[j]!=-1)
    '                { reg[j] = (reg[j]+j)%nn ;
    '                  q ^= alpha_to[reg[j]] ;
    '                } ;
    '             if (!q)        /* store root and error location number indices */
    '              { root[count] = i;
    '                loc[count] = nn-i ;
    '                count++ ;
    '              };
    '          } ;
    '         if (count==l[u])    /* no. roots = degree of elp hence <= tt errors */
    '          {
    '/* form polynomial z(x) */
    '           for (i=1; i<=l[u]; i++)        /* Z[0] = 1 always - do not need */
    '            { if ((s[i]!=-1) && (elp[u][i]!=-1))
    '                 z[i] = alpha_to[s[i]] ^ alpha_to[elp[u][i]] ;
    '              else if ((s[i]!=-1) && (elp[u][i]==-1))
    '                      z[i] = alpha_to[s[i]] ;
    '                   else if ((s[i]==-1) && (elp[u][i]!=-1))
    '                          z[i] = alpha_to[elp[u][i]] ;
    '                        else
    '                          z[i] = 0 ;
    '              for (j=1; j<i; j++)
    '                if ((s[j]!=-1) && (elp[u][i-j]!=-1))
    '                   z[i] ^= alpha_to[(elp[u][i-j] + s[j])%nn] ;
    '              z[i] = index_of[z[i]] ;         /* put into index form */
    '            } ;

    '  /* evaluate errors at locations given by error location numbers loc[i] */
    '           for (i=0; i<nn; i++)
    '             { err[i] = 0 ;
    '               if (recd[i]!=-1)        /* convert recd[] to polynomial form */
    '                 recd[i] = alpha_to[recd[i]] ;
    '               else  recd[i] = 0 ;
    '             }
    '           for (i=0; i<l[u]; i++)    /* compute numerator of error term first */
    '            { err[loc[i]] = 1;       /* accounts for z[0] */
    '              for (j=1; j<=l[u]; j++)
    '                if (z[j]!=-1)
    '                  err[loc[i]] ^= alpha_to[(z[j]+j*root[i])%nn] ;
    '              if (err[loc[i]]!=0)
    '               { err[loc[i]] = index_of[err[loc[i]]] ;
    '                 q = 0 ;     /* form denominator of error term */
    '                 for (j=0; j<l[u]; j++)
    '                   if (j!=i)
    '                     q += index_of[1^alpha_to[(loc[j]+root[i])%nn]] ;
    '                 q = q % nn ;
    '                 err[loc[i]] = alpha_to[(err[loc[i]]-q+nn)%nn] ;
    '                 recd[loc[i]] ^= err[loc[i]] ;  /*recd[i] must be in polynomial form */
    '               }
    '            }
    '          }
    '         else    /* no. roots != degree of elp => >tt errors and cannot solve */
    '           for (i=0; i<nn; i++)        /* could return error flag if desired */
    '               if (recd[i]!=-1)        /* convert recd[] to polynomial form */
    '                 recd[i] = alpha_to[recd[i]] ;
    '               else  recd[i] = 0 ;     /* just output received codeword as is */
    '       }
    '     else         /* elp has degree has degree >tt hence cannot solve */
    '       for (i=0; i<nn; i++)       /* could return error flag if desired */
    '          if (recd[i]!=-1)        /* convert recd[] to polynomial form */
    '            recd[i] = alpha_to[recd[i]] ;
    '          else  recd[i] = 0 ;     /* just output received codeword as is */
    '    }
    '   else       /* no non-zero syndromes => no errors: output received codeword */
    '    for (i=0; i<nn; i++)
    '       if (recd[i]!=-1)        /* convert recd[] to polynomial form */
    '         recd[i] = alpha_to[recd[i]] ;
    '       else  recd[i] = 0 ;
    ' }



    'main()
    '{
    '  register int i;

    '/* generate the Galois Field GF(2**mm) */
    '  generate_gf() ;
    '  printf("Look-up tables for GF(2**%2d)\n",mm) ;
    '  printf("  i   alpha_to[i]  index_of[i]\n") ;
    '  for (i=0; i<=nn; i++)
    '   printf("%3d      %3d          %3d\n",i,alpha_to[i],index_of[i]) ;
    '  printf("\n\n") ;

    '/* compute the generator polynomial for this RS code */
    '  gen_poly() ;


    '/* for known data, stick a few numbers into a zero codeword. Data is in
    '   polynomial form.
    '*/
    'for  (i=0; i<kk; i++)   data[i] = 0 ;

    '/* for example, say we transmit the following message (nothing special!) */
    'data[0] = 8 ;
    'data[1] = 6 ;
    'data[2] = 8 ;
    'data[3] = 1 ;
    'data[4] = 2 ;
    'data[5] = 4 ;
    'data[6] = 15 ;
    'data[7] = 9 ;
    'data[8] = 9 ;

    '/* encode data[] to produce parity in bb[].  Data input and parity output
    '   is in polynomial form
    '*/
    '  encode_rs() ;

    '/* put the transmitted codeword, made up of data plus parity, in recd[] */
    '  for (i=0; i<nn-kk; i++)  recd[i] = bb[i] ;
    '  for (i=0; i<kk; i++) recd[i+nn-kk] = data[i] ;

    '/* if you want to test the program, corrupt some of the elements of recd[]
    '   here. This can also be done easily in a debugger. */
    '/* Again, lets say that a middle element is changed */
    '  data[nn-nn/2] = 3 ;


    '  for (i=0; i<nn; i++)
    '     recd[i] = index_of[recd[i]] ;          /* put recd[i] into index form */

    '/* decode recv[] */
    '  decode_rs() ;         /* recd[] is returned in polynomial form */

    '/* print out the relevant stuff - initial and decoded {parity and message} */
    '  printf("Results for Reed-Solomon code (n=%3d, k=%3d, t= %3d)\n\n",nn,kk,tt) ;
    '  printf("  i  data[i]   recd[i](decoded)   (data, recd in polynomial form)\n");
    '  for (i=0; i<nn-kk; i++)
    '    printf("%3d    %3d      %3d\n",i, bb[i], recd[i]) ;
    '  for (i=nn-kk; i<nn; i++)
    '    printf("%3d    %3d      %3d\n",i, data[i-nn+kk], recd[i]) ;
    '}
    '*************************************************************************************************
    '   This program may be freely modified and/or given to whoever wants it.
    '   A condition of such distribution is that the author's contribution be
    '   acknowledged by his name being left in the comments heading the program,
    '   however no responsibility is accepted for any financial or other loss which
    '   may result from some unforseen errors or malfunctioning of the program
    '   during use.
    '                                 Simon Rockliff, 26th June 1991

    Public Sub New()

    End Sub
End Class
