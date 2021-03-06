#pragma once

#include "mg_common.h"
#include "mg_macros.h"

namespace mg {

constexpr f64 Pi = 3.14159265358979323846;

/* Generate a power table for a particular base and type */
template <int N>
int (&Power(int Base))[N];

bool IsEven(i64 X);
bool IsOdd (i64 X);
v3i  IsEven(const v3i& P);
v3i  IsOdd (const v3i& P);
mg_T(t) bool IsBetween(t Val, t A, t B);

mg_T(t) int Exponent(t Val);

mg_TT(t, u = t) t Prod(const v2<u>& Vec);
mg_TT(t, u = t) t Prod(const v3<u>& Vec);

/* 2D vector math */
mg_T(t) v2<t> operator+(const v2<t>& Lhs, const v2<t>& Rhs);
mg_T(t) v2<t> operator-(const v2<t>& Lhs, const v2<t>& Rhs);
mg_T(t) v2<t> operator/(const v2<t>& Lhs, const v2<t>& Rhs);
mg_T(t) v2<t> operator*(const v2<t>& Lhs, const v2<t>& Rhs);
mg_T(t) v2<t> operator+(const v2<t>& Lhs, t Val);
mg_T(t) v2<t> operator-(const v2<t>& Lhs, t Val);
mg_T(t) v2<t> operator/(const v2<t>& Lhs, t Rhs);
mg_T(t) v2<t> operator*(const v2<t>& Lhs, t Val);

mg_T(t) v2<t> Min(const v2<t>& Lhs, const v2<t>& Rhs);
mg_T(t) v2<t> Max(const v2<t>& Lhs, const v2<t>& Rhs);

// TODO: generalize the t parameter to u for the RHS
/* 3D vector math */
mg_T(t) v3<t> operator+(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator-(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator*(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator/(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator&(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator%(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> operator+(const v3<t>& Lhs, t Val);
mg_T(t) v3<t> operator-(const v3<t>& Lhs, t Val);
mg_T(t) v3<t> operator-(t Val, const v3<t>& Lhs);
mg_T(t) v3<t> operator*(const v3<t>& Lhs, t Val);
mg_T(t) v3<t> operator/(const v3<t>& Lhs, t Val);
mg_T(t) v3<t> operator&(const v3<t>& Lhs, t Val);
mg_T(t) v3<t> operator%(const v3<t>& Lhs, t Val);
mg_T(t) bool  operator==(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator!=(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator<=(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator< (const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator> (const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator>=(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) bool  operator==(const v3<t>& Lhs, t Val);
mg_T(t) bool  operator!=(const v3<t>& Lhs, t Val);
mg_TT(t, u) v3<t> operator>>(const v3<t>& Lhs, const v3<u>& Rhs);
mg_TT(t, u) v3<t> operator>>(const v3<t>& Lhs, u Val);
mg_TT(t, u) v3<u> operator>>(u Val, const v3<t>& Rhs);
mg_TT(t, u) v3<t> operator<<(const v3<t>& Lhs, const v3<u>& Rhs);
mg_TT(t, u) v3<t> operator<<(const v3<t>& Lhs, u Val);
mg_TT(t, u) v3<u> operator<<(u Val, const v3<t>& Rhs);

mg_T(t) v3<t> Min(const v3<t>& Lhs, const v3<t>& Rhs);
mg_T(t) v3<t> Max(const v3<t>& Lhs, const v3<t>& Rhs);

i8 Log2Floor(i64 Val);
i8 Log8Floor(i64 Val);

mg_T(t) v3<t> Ceil(const v3<t>& Vec);
mg_T(t) v2<t> Ceil(const v2<t>& Vec);

i64 Pow(i64 Base, int Exp);
i64 NextPow2(i64 Val);

int GeometricSum(int Base, int N);

mg_T(t) t Lerp(t V0, t V1, f64 T);
// bilinear interpolation
mg_T(t) t BiLerp(t V00, t V10, t V01, t V11, const v2d& T);
// trilinear interpolation
mg_T(t) t TriLerp(
  t V000, t V100, t V010, t V110, t V001,
  t V101, t V011, t V111, const v3d& T);

} // namespace mg

#include "mg_math.inl"
