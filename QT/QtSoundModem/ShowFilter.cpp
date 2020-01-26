// UZ7HO Soundmodem Port

#include "UZ7HOStuff.h"
#include <QPainter>

// This displays a graph of the filter characteristics

  // Complex numbers, with precision specified in TFloat (Types unit)

/*

{ Unit Complexs

This unit implements complex number arithmic, including the basic
operations addition, substraction, multiplication, division,
magnitude and phase.

Copyright: Nils Haeck M.Sc. (email : n.haeck@simdesign.nl)
	For more information visit http ://ww.simdesign.nl
Original date of publication : 10 Mar 2003

This unit requires these other units :
-Math : Delphi mathematics unit
- Types : Additional mathematical variable types

****************************************************************

The contents of this file are subject to the Mozilla Public
License Version 1.1 (the "License"); you may not use this file
except in compliance with the License.You may obtain a copy of
the License at :
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an
"AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
implied.See the License for the specific language governing
rights and limitations under the License.
}

*/

typedef struct TComplex_t
{
	float Re; // Real part
	float Im; // Imaginary part
} TComplex;

// Zero value
struct TComplex_t ComplexZero = { 0.0f, 0.0f};

//// Set a complex number
//struct TComplex_t Complex(Re: TFloat; Im: TFloat) : TComplex;

// Add complex numbers (Result = C1 + C2)

//struct TComplex_t * ComplexAdd(struct TComplex_t * C1, struct TComplex_t * C2);

// Substract complex numbers (Result = C1 - C2)
//function ComplexSub(const C1, C2: TComplex) : TComplex;

// Multiply complex numbers (Result = C1 * C2)
//function ComplexMul(const C1, C2: TComplex) : TComplex;

// Scale complex numbers (Result = Scale * C)
//function ComplexScl(Scale: TFloat; const C : TComplex) : TComplex;

// Get the magnitude of the complex number C
//function ComplexMag(const C : TComplex) : TFloat;

// Get the phase of the complex number C (in radians, between -pi and pi)
//function ComplexPhase(const C : TComplex) : TFloat;
/*

implementation

function Complex(Re: TFloat; Im: TFloat) : TComplex;
// Set a complex number
{
Result.Re : = Re;
Result.Im : = Im;
}

function ComplexAdd(const C1, C2: TComplex) : TComplex;
// Add complex numbers (Result = C1 + C2)
{
Result.Re : = C1.Re + C2.Re;
Result.Im : = C1.Im + C2.Im;
}

function ComplexSub(const C1, C2: TComplex) : TComplex;
// Substract complex numbers (Result = C1 - C2)
{
Result.Re : = C1.Re - C2.Re;
Result.Im : = C1.Im - C2.Im;
}

function ComplexMul(const C1, C2: TComplex) : TComplex;
// Multiply complex numbers (Result = C1 * C2)
{
Result.Re : = C1.Re * C2.Re - C1.Im * C2.Im;
Result.Im : = C1.Im * C2.Re + C1.Re * C2.Im;
}

function ComplexScl(Scale: TFloat; const C : TComplex) : TComplex;
// Scale complex numbers (Result = Scale * C)
{
Result.Re : = Scale * C.Re;
Result.Im : = Scale * C.Im;
}
*/

float ComplexMag(struct TComplex_t * C)
{
	// Get the magnitude of the complex number C

	return sqrtf(C->Re) + (C->Im * C->Im);
}
/*

function ComplexPhase(const C : TComplex) : TFloat;
// Get the phase of the complex number C (in radians, between -pi and pi)
const
c2Pi = 2 * pi;
cPid2 = 0.5 * pi;
{
// Both zero
if (C.Re = 0) and (C.Im = 0) then {
Result : = 0;
exit;
}

// Non-zero case
if abs(C.Re) > abs(C.Im) then {
Result : = ArcTan(C.Im / C.Re); {-45 to 45 deg, 135 to - 135 deg}
if C.Re < 0 then Result : = Result + pi;
end else {
Result : = cPid2 - ArcTan(C.Re / C.Im); {45 to 135, -45 to - 135}
if C.Im < 0 then Result : = Result + pi;
}
if Result > pi then Result : = Result - c2pi;
}

end.
*/

/*

{ Unit FFTs

This unit provides a forward and inverse FFT pascal implementation
for complex number series.

The formal definition of the complex DFT is :
y[k] = sum(x[m] * exp(-i * 2 * pi*k*m / n), m = 0..n - 1), k = 0..n - 1

Copyright : Nils Haeck M.Sc. (email : n.haeck@simdesign.nl)
	For more information visit http ://www.simdesign.nl
Original date of publication : 10 Mar 2003

This unit requires these other units :
-Complexs : Complex number unit
- Types : Additional mathematical variable types
- SysUtils : Delphi system utilities

****************************************************************

The contents of this file are subject to the Mozilla Public
License Version 1.1 (the "License"); you may not use this file
except in compliance with the License.You may obtain a copy of
the License at :
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an
"AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
implied.See the License for the specific language governing
rights and limitations under the License.
}
unit FFTs;

interface

	uses
	Complexs, Types1, SysUtils;

const
*/

#define cMaxPrimeFactor 1021
#define cMaxPrimeFactorDiv2 = (cMaxPrimeFactor + 1) / 2
#define cMaxFactorCount = 20


//sErrPrimeTooLarge = 'Prime factor for FFT length too large. Change value for cMaxPrimeFactor in FFTs unit';

/*
ForwardFFT:
Perform a complex FFT on the data in Source, put result in Dest.This routine
works best for Count as a power of 2, but also works usually faster than DFT
by factoring the series.Only in cases where Count is a prime number will this
method be identical to regular complex DFT.

The largest prime factor in Count should be less or equal to cMaxPrimeFactor.

The remaining factors are handled by optimised partial FFT code, that can be
found in the FFT_X procedures

Inputs :
Source: this can be any zero - based array type of TComplex
	Count : The number of elements in the array.

	Outputs :
	Dest : this can be any zero - based array type of TComplex, and will contain
	the FFT transformed data(frequency spectrum).Source may be equal to
	Dest.In this case, the original series will be overwritten with the new
	fourier - transformed series.
}
procedure ForwardFFT(const Source : array of TComplex; var Dest : array of TComplex; Count: integer);

{ Perform the inverse FFT on the Source data, and put result in Dest.This is based
on the forward FFT with some additional customisation.The result of a forward
FFT followed by an inverse FFT should yield the same data, except for rounding
errors.
}
procedure InverseFFT(const Source : array of TComplex; var Dest : array of TComplex; Count: integer);

implementation

const
// Some helper constants for the FFT optimisations

*/

#define c3 -1.5000000000000E+00f //  cos(2*pi / 3) - 1;
#define c32 8.6602540378444E-01f //  sin(2*pi / 3);

#define u5 1.2566370614359E+00f //  2*pi / 5;
#define c51 -1.2500000000000E+00f // (cos(u5) + cos(2*u5))/2 - 1;
#define c52 5.5901699437495E-01f // (cos(u5) - cos(2*u5))/2;
#define c53 -9.5105651629515E-0f //- sin(u5);
#define c54 -1.5388417685876E+00f //-(sin(u5) + sin(2*u5));
#define c55 3.6327126400268E-01f // (sin(u5) - sin(2*u5));
#define c8 = 7.0710678118655E-01f //  1 / sqrt(2);

/*

// Base 1 and Base 0 arrays
TIdx0FactorArray = array[0..cMaxFactorCount] of integer;
TIdx1FactorArray = array[1..cMaxFactorCount] of integer;

// Factorise the series with length Count into FactorCount factors, stored in Fact
procedure Factorize(Count: integer; var FactorCount : integer; var Fact : TIdx1FactorArray);
var
i : integer;
Factors: TIdx1FactorArray;
const
// Define specific FFT lengths (radices) that we can process with optimised routines
cRadixCount = 6;
cRadices: array[1..6] of integer =
	(2, 3, 4, 5, 8, 10);
		  {

			  if Count = 1 then {
				  FactorCount : = 1;
		  Factors[1]  : = 1;
		  end else {
			  FactorCount : = 0;
		  }

		  // Factorise the original series length Count into known factors and rest value
	  i: = cRadixCount;
		  while (Count > 1) AND(i > 0) do {
			  if Count mod cRadices[i] = 0 then {
				  Count : = Count div cRadices[i];
		  inc(FactorCount);
		  Factors[FactorCount] : = cRadices[i];
		  end else
			  dec(i);
		  }

		  // substitute factors 2*8 with more optimal 4*4
		  if Factors[FactorCount] = 2 then {
			  i : = FactorCount - 1;
		  while (i > 0) AND(Factors[i] < > 8) do
			  dec(i);
		  if i > 0 then {
			  Factors[FactorCount] : = 4;
		  Factors[i] : = 4;
		  }
		  }

		  // Analyse the rest value and see if it can be factored in primes
		  if Count > 1 then {
			  for i : = 2 to trunc(sqrt(Count)) do {
				  while Count mod i = 0 do {
					  Count : = Count div i;
		  inc(FactorCount);
		  Factors[FactorCount] : = i;
		  }
		  }

		  if (Count > 1) then {
			  inc(FactorCount);
		  Factors[FactorCount] : = Count;
		  }
		  }

		  // Reverse factors so that primes are first
		  for i : = 1 to FactorCount do
			  Fact[i] : = Factors[FactorCount - i + 1];

		  }

		  { Reorder the series in X to a permuted sequence in Y so that the later step can
			  be done in place, and the final FFT result is in correct order.
			  The series X and Y must be different series!
		  }
		  procedure ReorderSeries(Count: integer; var Factors : TIdx1FactorArray; var Remain : TIdx0FactorArray;
		  const X : array of TComplex; var Y : array of TComplex);
		  var
			  i, j, k: integer;
	  Counts: TIdx1FactorArray;
		  {
			  FillChar(Counts, SizeOf(Counts), 0);

	  k: = 0;
		  for i : = 0 to Count - 2 do {
			  Y[i] : = X[k];
	  j: = 1;
	  k: = k + Remain[j];
		  Counts[1] : = Counts[1] + 1;
		  while Counts[j] >= Factors[j] do {
			  Counts[j] : = 0;
	  k: = k - Remain[j - 1] + Remain[j + 1];
		  inc(j);
		  inc(Counts[j]);
		  }
		  }

		  Y[Count - 1] : = X[Count - 1];
		  }

		  procedure FFT_2(var Z : array of TComplex);
		  var
			  T1 : TComplex;
		  {
			  T1 : = ComplexAdd(Z[0], Z[1]);
		  Z[1] : = ComplexSub(Z[0], Z[1]);
		  Z[0] : = T1;
		  }

		  procedure FFT_3(var Z : array of TComplex);
		  var
			  T1, M1, M2, S1: TComplex;
		  {
			  T1 : = ComplexAdd(Z[1], Z[2]);
		  Z[0] : = ComplexAdd(Z[0], T1);
	  M1: = ComplexScl(c31, T1);
		  M2.Re : = c32 * (Z[1].Im - Z[2].Im);
		  M2.Im : = c32 * (Z[2].Re - Z[1].Re);
	  S1: = ComplexAdd(Z[0], M1);
		  Z[1] : = ComplexAdd(S1, M2);
		  Z[2] : = ComplexSub(S1, M2);
		  }

		  procedure FFT_4(var Z : array of TComplex);
		  var
			  T1, T2, M2, M3: TComplex;
		  {
			  T1 : = ComplexAdd(Z[0], Z[2]);
	  T2: = ComplexAdd(Z[1], Z[3]);

	  M2: = ComplexSub(Z[0], Z[2]);
		  M3.Re : = Z[1].Im - Z[3].Im;
		  M3.Im : = Z[3].Re - Z[1].Re;

		  Z[0] : = ComplexAdd(T1, T2);
		  Z[2] : = ComplexSub(T1, T2);
		  Z[1] : = ComplexAdd(M2, M3);
		  Z[3] : = ComplexSub(M2, M3);
		  }

		  procedure FFT_5(var Z : array of TComplex);
		  var
			  T1, T2, T3, T4, T5: TComplex;
		  M1, M2, M3, M4, M5: TComplex;
		  S1, S2, S3, S4, S5: TComplex;
		  {
			  T1 : = ComplexAdd(Z[1], Z[4]);
	  T2: = ComplexAdd(Z[2], Z[3]);
	  T3: = ComplexSub(Z[1], Z[4]);
	  T4: = ComplexSub(Z[3], Z[2]);

	  T5: = ComplexAdd(T1, T2);
		  Z[0] : = ComplexAdd(Z[0], T5);
	  M1: = ComplexScl(c51, T5);
	  M2: = ComplexScl(c52, ComplexSub(T1, T2));

		  M3.Re : = -c53 * (T3.Im + T4.Im);
		  M3.Im : = c53 * (T3.Re + T4.Re);
		  M4.Re : = -c54 * T4.Im;
		  M4.Im : = c54 * T4.Re;
		  M5.Re : = -c55 * T3.Im;
		  M5.Im : = c55 * T3.Re;

	  S3: = ComplexSub(M3, M4);
	  S5: = ComplexAdd(M3, M5);;
	  S1: = ComplexAdd(Z[0], M1);
	  S2: = ComplexAdd(S1, M2);
	  S4: = ComplexSub(S1, M2);

		  Z[1] : = ComplexAdd(S2, S3);
		  Z[2] : = ComplexAdd(S4, S5);
		  Z[3] : = ComplexSub(S4, S5);
		  Z[4] : = ComplexSub(S2, S3);
		  }

		  procedure FFT_8(var Z : array of TComplex);
		  var
			  A, B: array[0..3] of TComplex;
	  Gem: TFloat;
		  {
			  A[0] : = Z[0]; B[0] : = Z[1];
		  A[1] : = Z[2]; B[1] : = Z[3];
		  A[2] : = Z[4]; B[2] : = Z[5];
		  A[3] : = Z[6]; B[3] : = Z[7];

		  FFT_4(A);
		  FFT_4(B);

	  Gem: = c8 * (B[1].Re + B[1].Im);
		  B[1].Im : = c8 * (B[1].Im - B[1].Re);
		  B[1].Re : = Gem;
	  Gem: = B[2].Im;
		  B[2].Im : = -B[2].Re;
		  B[2].Re : = Gem;
	  Gem: = c8 * (B[3].Im - B[3].Re);
		  B[3].Im : = -c8 * (B[3].Re + B[3].Im);
		  B[3].Re : = Gem;

		  Z[0] : = ComplexAdd(A[0], B[0]); Z[4] : = ComplexSub(A[0], B[0]);
		  Z[1] : = ComplexAdd(A[1], B[1]); Z[5] : = ComplexSub(A[1], B[1]);
		  Z[2] : = ComplexAdd(A[2], B[2]); Z[6] : = ComplexSub(A[2], B[2]);
		  Z[3] : = ComplexAdd(A[3], B[3]); Z[7] : = ComplexSub(A[3], B[3]);
		  }

		  procedure FFT_10(var Z : array of TComplex);
		  var
			  A, B: array[0..4] of TComplex;
		  {
			  A[0] : = Z[0];  B[0] : = Z[5];
		  A[1] : = Z[2];  B[1] : = Z[7];
		  A[2] : = Z[4];  B[2] : = Z[9];
		  A[3] : = Z[6];  B[3] : = Z[1];
		  A[4] : = Z[8];  B[4] : = Z[3];

		  FFT_5(A);
		  FFT_5(B);

		  Z[0] : = ComplexAdd(A[0], B[0]); Z[5] : = ComplexSub(A[0], B[0]);
		  Z[6] : = ComplexAdd(A[1], B[1]); Z[1] : = ComplexSub(A[1], B[1]);
		  Z[2] : = ComplexAdd(A[2], B[2]); Z[7] : = ComplexSub(A[2], B[2]);
		  Z[8] : = ComplexAdd(A[3], B[3]); Z[3] : = ComplexSub(A[3], B[3]);
		  Z[4] : = ComplexAdd(A[4], B[4]); Z[9] : = ComplexSub(A[4], B[4]);
		  }

		  {
			  Synthesize the FFT by taking the even factors and the odd factors multiplied by
				  complex sinusoid
		  }
		  procedure SynthesizeFFT(Sofar, Radix, Remain: integer; var Y : array of TComplex);
		  var
			  GroupOffset, DataOffset, Position: integer;
		  GroupNo, DataNo, BlockNo, SynthNo: integer;
	  Omega: double;
		  S, CosSin: TComplex;
		  Synth, Trig, Z: array[0..cMaxPrimeFactor - 1] of TComplex;

		  // Local function
		  procedure InitializeTrigonomials(Radix: integer);
		  // Initialize trigonomial coefficients
		  var
			  i : integer;
	  W: double;
	  X: TComplex;
		  {
			  W : = 2 * pi / Radix;
		  Trig[0] : = Complex(1.0, 0.0);
	  X: = Complex(cos(W), -sin(W));
		  Trig[1] : = X;
		  for i : = 2 to Radix - 1 do
			  Trig[i] : = ComplexMul(X, Trig[i - 1]);
		  }

		  // Local Function
		  procedure FFT_Prime(Radix: integer);
		  // This is the general DFT, which can't be made any faster by factoring because
		  // Radix is a prime number
		  var
			  i, j, k, N, AMax: integer;
		  Re, Im: TComplex;
		  V, W: array[0..cMaxPrimeFactorDiv2 - 1] of TComplex;
		  {
			  N : = Radix;
	  AMax: = (N + 1) div 2;
		  for j : = 1 to AMax - 1 do {
			  V[j].Re : = Z[j].Re + Z[n - j].Re;
		  V[j].Im : = Z[j].Im - Z[n - j].Im;
		  W[j].Re : = Z[j].Re - Z[n - j].Re;
		  W[j].Im : = Z[j].Im + Z[n - j].Im;
		  }

		  for j : = 1 to AMax - 1 do {
			  Z[j] : = Z[0];
		  Z[N - j] : = Z[0];
	  k: = j;
		  for i : = 1 to AMax - 1 do {
			  Re.Re : = Trig[k].Re * V[i].Re;
		  Im.Im : = Trig[k].Im * V[i].Im;
		  Re.im : = Trig[k].Re * W[i].Im;
		  Im.Re : = Trig[k].Im * W[i].Re;

		  Z[N - j].Re : = Z[N - j].Re + Re.Re + Im.Im;
		  Z[N - j].Im : = Z[N - j].Im + Re.Im - Im.Re;
		  Z[j].Re   : = Z[j].Re + Re.Re - Im.Im;
		  Z[j].Im   : = Z[j].Im + Re.Im + Im.Re;

	  k: = k + j;
		  if k >= N then
			  k : = k - N;
		  }
		  }

		  for j : = 1 to AMax - 1 do {
			  Z[0].Re : = Z[0].Re + V[j].Re;
		  Z[0].Im : = Z[0].Im + W[j].Im;
		  }
		  }

		  // main
		  {
			  // Initialize trigonomial coefficients
			  InitializeTrigonomials(Radix);

	  Omega: = 2 * pi / (Sofar * Radix);
	  CosSin: = Complex(cos(Omega), -sin(Omega));
	  S: = Complex(1.0, 0.0);
	  DataOffset: = 0;
	  GroupOffset: = 0;
	  Position: = 0;

		  for DataNo : = 0 to Sofar - 1 do {

			  if Sofar > 1 then {

				  Synth[0] : = Complex(1.0, 0.0);
		  Synth[1] : = S;
		  for SynthNo : = 2 to Radix - 1 do
			  Synth[SynthNo] : = ComplexMul(S, Synth[SynthNo - 1]);
	  S: = ComplexMul(CosSin, S);

		  }

		  for GroupNo : = 0 to Remain - 1 do {

			  if (Sofar > 1) AND(DataNo > 0) then {

				  Z[0] : = Y[Position];
	  BlockNo: = 1;
		  repeat
			  inc(Position, Sofar);
		  Z[BlockNo] : = ComplexMul(Synth[BlockNo], Y[Position]);
		  inc(BlockNo);
		  until BlockNo >= Radix;

		  end else {

			  for BlockNo : = 0 to Radix - 1 do {
				  Z[BlockNo] : = Y[Position];
		  inc(Position, Sofar);
		  }

		  }

		  case Radix of
		  2:  FFT_2(Z);
			  3:  FFT_3(Z);
			  4:  FFT_4(Z);
			  5:  FFT_5(Z);
			  8:  FFT_8(Z);
			  10: FFT_10(Z);
			  else
				  // Any larger prime number than 5 (so 7, 11, 13, etc, up to cMaxPrimeFactor)
				  FFT_Prime(Radix);
		  } //case

	  Position: = GroupOffset;
		  for BlockNo : = 0 to Radix - 1 do {
			  Y[Position] : = Z[blockNo];
		  Inc(Position, Sofar);
		  }
	  GroupOffset: = GroupOffset + Sofar * Radix;
	  Position: = GroupOffset;
		  }
		  inc(DataOffset);
	  GroupOffset: = DataOffset;
	  Position: = DataOffset;
		  }
		  }


void ForwardFFT(TComplex * Source, TComplex * Dest, int  Count)
{
		  // Perform a FFT on the data in Source, put result in Dest. This routine works best
		  // for Count as a power of 2, but also works usually faster than DFT by factoring
		  // the series. Only in cases where Count is a prime number will this method be
		  // identical to regular DFT.
//		  type
//			  PComplexArray = ^TComplexArray;
		  
	TComplex TComplexArray = array[0..0] of ;
		  var
			  i : integer;
	  FactorCount: integer;
	  SofarRadix:  TIdx1FactorArray;
	  ActualRadix: TIdx1FactorArray;
	  RemainRadix: TIdx0FactorArray;
	  TmpDest: PComplexArray;
		  {
			  if Count = 0 then exit;

		  // Decompose the series with length Count into FactorCount factors in ActualRadix
		  Factorize(Count, FactorCount, ActualRadix);

		  // Check if our biggest prime factor is not too large
		  if (ActualRadix[1] > cMaxPrimeFactor) then
			  raise EMathError.Create(sErrPrimeTooLarge);

		  // Setup Sofar and Remain tables
		  RemainRadix[0] : = Count;
		  SofarRadix[1]  : = 1;
		  RemainRadix[1] : = Count div ActualRadix[1];
		  for i : = 2 to FactorCount do {
			  SofarRadix[i] : = SofarRadix[i - 1] * ActualRadix[i - 1];
		  RemainRadix[i] : = RemainRadix[i - 1] div ActualRadix[i];
		  }

		  // Make temp copy if dest = source (otherwise the permute procedure will completely
		  // ruin the structure
		  if @Dest = @Source then {
			  GetMem(TmpDest, SizeOf(TComplex) * Count);
		  Move(Dest, TmpDest^, SizeOf(TComplex) * Count);
		  end else {
			  TmpDest : = @Dest;
			  }

		  // Reorder the series so that the elements are already in the right place for
		  // synthesis
		  ReorderSeries(Count{ , FactorCount }, ActualRadix, RemainRadix, Source, TmpDest^);

		  // Free the temporary copy (if any)
		  if @Dest = @Source then {
			  Move(TmpDest^, Dest, SizeOf(TComplex) * Count);
		  FreeMem(TmpDest);
		  }

		  // Synthesize each of the FFT factored series
		  for i : = 1 to FactorCount do
			  SynthesizeFFT(SofarRadix[i], ActualRadix[i], RemainRadix[i], Dest);
		  }

		  procedure InverseFFT(const Source : array of TComplex; var Dest : array of TComplex; Count: integer);
		  // Perform the inverse FFT on the Source data, and put result in Dest. It performs
		  // the forward FFT and then divides elements by N
		  var
			  i : integer;
	  S: TFloat;
	  TmpSource: array of TComplex;
		  {
			  if Count = 0 then exit;

		  // Since TmpSource is local, we do not have to free it again,
		  // it will be freed automatically when out of scope
		  SetLength(TmpSource, Count);

		  // Create a copy with inverted imaginary part.
		  for i : = 0 to Count - 1 do
			  with Source[i] do
			  TmpSource[i] : = Complex(Re, -Im);
		  ForwardFFT(TmpSource, Dest, Count);

		  // Scale by 1/Count, and inverse the imaginary part
	  S: = 1.0 / Count;
		  for i : = 0 to Count - 1 do {
			  Dest[i].Re : = S * Dest[i].Re;
		  Dest[i].Im : = -S * Dest[i].Im;
		  }
		  }

		  end.
*/

float pnt_graph_buf[4096];
float graph_buf[4096];
float prev_graph_buf[4096];
float src_graph_buf[4096];
float graph_f;
float RealOut[4096];
short RealIn[4096];
float ImagOut[4096];

#define Image1Width 642
#define Image1Height 312



void filter_grid(QPainter * Painter)
{
	int col = 20;
	int row = 8;
	int top_margin = 10;
	int bottom_margin = 20;
	int left_margin = 30;
	int right_margin = 10;

	int x, y, i;
	float kx, ky;

	QPen pen;  // creates a default pen

	pen.setStyle(Qt::DotLine);
	Painter->setPen(pen);


	ky = 35;

	kx = (Image1Width - left_margin - right_margin - 2) / col;

	for (y = 0; y < row; y++)
	{
		Painter->drawLine(
			left_margin + 1,
			top_margin + round(ky*y) + 1,
			Image1Width - right_margin - 1,
			top_margin + round(ky*y) + 1);
	}

	for (x = 0; x < col; x++)
	{
		Painter->drawLine(
			left_margin + round(kx*x) + 1,
			top_margin + 1,
			left_margin + round(kx*x) + 1,
			Image1Height - bottom_margin - 1);
	}
//Form4.Image1.Canvas.Pen.Color: = clBlack;
//Form4.Image1.Canvas.Pen.Style: = psSolid;

	pen.setStyle(Qt::SolidLine);
	Painter->setPen(pen);

	for (y = 0; y < row / 2; y++)
	{
		char Textxx[20];

		sprintf(Textxx, "%d", y * -20);

		Painter->drawLine(
	 		left_margin + 1,
			top_margin + round(ky*y * 2) + 1,
			Image1Width - right_margin - 1,
			top_margin + round(ky*y * 2) + 1);
		
		Painter->drawText(
			1,
			top_margin + round(ky*y * 2) + 1,
			100, 20, 0, Textxx);

	}


	for (x = 0; x <= col / 5; x++)
	{
		char Textxx[20];

		sprintf(Textxx, "%d", x * 1000);

		Painter->drawLine(
			left_margin + round(kx*x * 5) + 1,
			top_margin + 1,
			left_margin + round(kx*x * 5) + 1,
			Image1Height - bottom_margin - 1);

		Painter->drawText(
			top_margin + round(kx*x * 5) + 8,
			Image1Height - 15,
			100, 20, 0, Textxx);
	}
//Form4.Image1.Canvas.TextOut(10, top_margin - 5, 'dB');
//Form4.Image1.Canvas.TextOut(top_margin + 8, Form4.Image1.Height - 15, 'Hz');

}

extern  "C" void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);


void make_graph(float * buf, int buflen, QPainter * Painter)
{
	int top_margin = 10;
	int bottom_margin = 20;
	int left_margin = 30;
	int right_margin = 10;

	int i, y1, y2;
	float kx, ky;
	float pixel;

	if (buflen == 0)
		return;

	kx = (Image1Width - left_margin - right_margin - 2) / buflen;

	for (i = 0; i <= buflen - 2; i++)
	{
		y1 = 1 - round(buf[i]);
	
		if (y1 > Image1Height - top_margin - bottom_margin - 2)
			y1 = Image1Height - top_margin - bottom_margin - 2;

		y2 = 1 - round(buf[i + 1]);

		if (y2 > Image1Height - top_margin - bottom_margin - 2)
			y2 = Image1Height - top_margin - bottom_margin - 2;

		// 150 pixels for 1000 Hz

		// i is the bin number, but bin is not 10 Hz but 12000 /1024
		// so freq = i * 12000 / 1024;
		// and pixel is freq * 300 /1000

		pixel = i * 12000.0f / 1024.0f;
		pixel = pixel * 150.0f /1000.0f;

		Painter->drawLine(
			left_margin + pixel,
			top_margin + y1,
			left_margin + pixel + 1,
			top_margin + y2);

	}
}

void make_graph_buf(float * buf, short tap, QPainter * Painter)
{
	float x1, x2, amp;
	int fft_size;
	float max, acc1, acc2;
	int i, k;

	fft_size = 1024; // 12000 / 10; // 10hz on sample;

	for (i = 0; i < tap; i++)
		prev_graph_buf[i]= 0;
	
	for (i = 0; i < fft_size; i++)
		src_graph_buf[i] = 0;

	src_graph_buf[0]= 1;

	FIR_filter(src_graph_buf, fft_size, tap, buf, graph_buf, prev_graph_buf);


	for (k = 0; k < fft_size; k++)
		RealIn[k] = graph_buf[k] * 32768;
	
	FourierTransform(fft_size, RealIn, RealOut, ImagOut, 0);

	for (k = 0; k < (fft_size / 2) - 1; k++)
		pnt_graph_buf[k] = powf(RealOut[k], 2) + powf(ImagOut[k], 2);

	max = 0;

	for (i = 0; i < (fft_size / 2) - 1; i++)
	{
		if (pnt_graph_buf[i] > max)
			max = pnt_graph_buf[i];
	}

	if (max > 0)
	{
		for (i = 0; i < (fft_size / 2) - 1; i++)
			pnt_graph_buf[i] = pnt_graph_buf[i] / max;
	}

	for (i = 0; i < (fft_size / 2) - 1; i++)
	{
		if (pnt_graph_buf[i] > 0)
			pnt_graph_buf[i] = 70 * log10(pnt_graph_buf[i]);

		else

			pnt_graph_buf[i] = 0;
	}

	filter_grid(Painter);

	Painter->setPen(Qt::blue);

	make_graph(pnt_graph_buf, 400, Painter);
}
/*

procedure TForm4.make_graph_emphaser;
var
x1, x2: single;
amp: single;
fft_size: integer;
max, acc1, acc2: single;
i, k: word;
a1, a2: single;
{
fft_size : = 11025 div 10; // 10hz on sample;

for i: = 0 to fft_size - 1 do src_graph_buf[i] : = 0;
src_graph_buf[0]: = 1;

a1: = 0; a2: = 0;

for k: = 0 to fft_size - 1 do
{
//Emphasis filter;

acc1 : = acc1 * 0.95 + src_graph_buf[k] * 0.05;
//acc1:=a1-src_graph_buf[k];
//acc1:=0.35*a1-0.65*src_graph_buf[k];
a1: = src_graph_buf[k];
graph_buf[k]: = acc1;
//
}
{
	for k: = 0 to fft_size - 1 do
		{
		//Emphasis filter;
		//acc1:=a2-graph_buf[k];
		acc1 : = 0.63*a2 - 0.37*graph_buf[k];
a2: = graph_buf[k];
	graph_buf[k]: = acc1;
	//
	}
}
for k: = 0 to fft_size - 1 do arr2[k].Re : = graph_buf[k];
ForwardFFT(arr2, arr1, fft_size);
for k: = 0 to(fft_size div 2) - 1 do pnt_graph_buf[k] : = ComplexMag(arr1[k]);

max: = 0;
for i: = 0 to(fft_size div 2) - 1 do if pnt_graph_buf[i] > max then max : = pnt_graph_buf[i];
if max > 0 then for i: = 0 to(fft_size div 2) - 1 do pnt_graph_buf[i] : = pnt_graph_buf[i] / max;
for i: = 0 to(fft_size div 2) - 1 do
if pnt_graph_buf[i] > 0 then pnt_graph_buf[i]: = 70 * log10(pnt_graph_buf[i])
else pnt_graph_buf[i] : = 0;
filter_grid;
make_graph(pnt_graph_buf, 400);
}

end.
*/
