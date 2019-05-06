#pragma once

#include "mg_assert.h"

namespace mg {

mg_Ti(t) t
SetBit(t Val, int I) {
  mg_Assert(I < (int)mg_BitSizeOf(t));
  return Val | t((1ull << I));
}

mg_Ti(t) t
UnsetBit(t Val, int I) {
  mg_Assert(I < (int)mg_BitSizeOf(t));
  return Val & t(~(1ull << I));
}

mg_Ti(t) bool
BitSet(t Val, int I) {
  mg_Assert(I < (int)mg_BitSizeOf(t));
  return 1 & (Val >> I);
}

mg_Ti(t) t
FlipBit(t Val, int I) {
  mg_Assert(I < (int)mg_BitSizeOf(t));
  return Val ^ t(1ull << I);
}

// TODO: replace bsr with the faster intrinsic

#if defined(__clang__) || defined(__GNUC__)
mg_Inline i8
Msb(u32 V) {
  if (V == 0) return -1;
  return i8(mg_BitSizeOf(V) - 1 - __builtin_clz(V));
}
mg_Inline i8
Msb(u64 V) {
  if (V == 0) return -1;
  return i8(mg_BitSizeOf(V) - 1 -__builtin_clzll(V));
}
#elif defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanReverse64)
mg_Inline i8
Msb(u32 V) {
  if (V == 0) return -1;
  unsigned long Index = 0;
  _BitScanReverse(&Index, V);
  return (i8)Index;
}
mg_Inline i8
Msb(u64 V) {
  if (V == 0) return -1;
  unsigned long Index = 0;
  _BitScanReverse64(&Index, V);
  return (i8)Index;
}
#endif

/* Reverse the operation that inserts two 0 bits after every bit of x */
mg_Inline u32
CompactBy2(u32 X) {
  X &= 0x09249249;                  // X = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  X = (X ^ (X >>  2)) & 0x030c30c3; // X = ---- --98 ---- 76-- --54 ---- 32-- --10
  X = (X ^ (X >>  4)) & 0x0300f00f; // X = ---- --98 ---- ---- 7654 ---- ---- 3210
  X = (X ^ (X >>  8)) & 0x030000ff; // X = ---- --98 ---- ---- ---- ---- 7654 3210
  X = (X ^ (X >> 16)) & 0x000003ff; // X = ---- ---- ---- ---- ---- --98 7654 3210
  return X;
}

/* Morton decoding */
mg_Inline u32
DecodeMorton3X(u32 Code) { return CompactBy2(Code >> 0); }
mg_Inline u32
DecodeMorton3Y(u32 Code) { return CompactBy2(Code >> 1); }
mg_Inline u32
DecodeMorton3Z(u32 Code) { return CompactBy2(Code >> 2); }

mg_Inline u32
SplitBy2(u32 X) {
  X &= 0x000003ff;                  // X = ---- ---- ---- ---- ---- --98 7654 3210
  X = (X ^ (X << 16)) & 0x030000ff; // X = ---- --98 ---- ---- ---- ---- 7654 3210
  X = (X ^ (X <<  8)) & 0x0300f00f; // X = ---- --98 ---- ---- 7654 ---- ---- 3210
  X = (X ^ (X <<  4)) & 0x030c30c3; // X = ---- --98 ---- 76-- --54 ---- 32-- --10
  X = (X ^ (X <<  2)) & 0x09249249; // X = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return X;
}

mg_Inline u32
EncodeMorton3(u32 X, u32 Y, u32 Z) {
  return SplitBy2(X) | (SplitBy2(Y) << 1) | (SplitBy2(Z) << 2);
}

mg_Inline u32
Pack3Ints32(v3i V) { return u32(V.X) + (u32(V.Y) << 10) + (u32(V.Z) << 20); }
mg_Inline v3i
Unpack3Ints32(u32 V) {
  return v3i(V & 0x3FF, (V & 0xFFC00) >> 10, (V & 0x3FFFFC00) >> 20);
}

mg_Inline u64
Pack3i64(v3i V) { return u64(V.X) + (u64(V.Y) << 21) + (u64(V.Z) << 42); }
mg_Inline v3i
Unpack3i64(u64 V) {
  return v3i(V & 0x1FFFFF, (V & 0x3FFFFE00000) >> 21, (V & 0x7FFFFC0000000000ull) >> 42);
}

mg_Inline u32
LowBits64(u64 V) { return V & 0xFFFFFFFF; }
mg_Inline u32
HighBits64(u64 V) { return V >> 32; }

} // namespace mg

