#pragma once

#include "mg_common.h"
#include "mg_volume.h"

namespace mg {

struct wavelet_block {
  v3i Levels;
  grid Grid;
  volume Volume;
  bool IsPacked = false; // if the Volume stores data just for this block
};

struct wav_basis_norms {
  array<f64> ScalNorms; // scaling function norms
  array<f64> WaveNorms; // wavelet function norms
};
wav_basis_norms GetCdf53Norms(int NLevels);

/* Normal lifting which uses mirroring at the boundary */
mg_T(t) void FLiftCdf53OldX(t* F, const v3i& N, const v3i& L);
mg_T(t) void FLiftCdf53OldY(t* F, const v3i& N, const v3i& L);
mg_T(t) void FLiftCdf53OldZ(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53OldX(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53OldY(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53OldZ(t* F, const v3i& N, const v3i& L);
/* Do constant extrapolation (the last value is extrapolated if the signal is odd-length) */
mg_T(t) void FLiftCdf53ConstX(t* F, const v3i& N, const v3i& L);
mg_T(t) void FLiftCdf53ConstY(t* F, const v3i& N, const v3i& L);
mg_T(t) void FLiftCdf53ConstZ(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53ConstX(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53ConstY(t* F, const v3i& N, const v3i& L);
mg_T(t) void ILiftCdf53ConstZ(t* F, const v3i& N, const v3i& L);

/* New set of lifting functions. We assume the volume where we want to transform
to happen (M) is contained within a bigger volume (Vol). When Dims(Grid) is even,
extrapolation will happen, in a way that the last (odd) wavelet coefficient is 0.
We assume the storage at index M.(x/y/z) is available to store the extrapolated
values if necessary. */
enum lift_option { Normal, PartialUpdateLast, NoUpdateLast, NoUpdate };
mg_T(t) void FLiftCdf53X(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);
mg_T(t) void FLiftCdf53Y(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);
mg_T(t) void FLiftCdf53Z(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);
mg_T(t) void ILiftCdf53X(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);
mg_T(t) void ILiftCdf53Y(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);
mg_T(t) void ILiftCdf53Z(const grid& Grid, const v3i& M, lift_option Opt, volume* Vol);

/* Lifting with extrapolation */
mg_T(t) void FLiftExtCdf53X(t* F, const v3i& N, const v3i& NBig, const v3i& L);
mg_T(t) void FLiftExtCdf53Y(t* F, const v3i& N, const v3i& NBig, const v3i& L);
mg_T(t) void FLiftExtCdf53Z(t* F, const v3i& N, const v3i& NBig, const v3i& L);
mg_T(t) void ILiftExtCdf53X(t* F, const v3i& N, const v3i& NBig, const v3i& L);
mg_T(t) void ILiftExtCdf53Y(t* F, const v3i& N, const v3i& NBig, const v3i& L);
mg_T(t) void ILiftExtCdf53Z(t* F, const v3i& N, const v3i& NBig, const v3i& L);

// #define mg_Cdf53TileDebug

// void ForwardCdf53Tile(int NLvls, const v3i& TDims3, const volume& Vol
// #if defined(mg_Cdf53TileDebug)
//   , volume* OutVol
// #endif
// );
void ForwardCdf53(const extent& Ext, int NLevels, volume* Vol);
void InverseCdf53(const extent& Ext, int NLevels, volume* Vol);
void ForwardCdf53Old(volume* Vol, int NLevels);
void InverseCdf53Old(volume* Vol, int NLevels);
void ForwardCdf53Ext(const extent& Ext, volume* Vol);
void InverseCdf53Ext(const extent& Ext, volume* Vol);

mg_T(t) struct array;
void BuildSubbands(const v3i& N, int NLevels, array<extent>* Subbands);
void BuildSubbands(const v3i& N, int NLevels, array<grid>* Subbands);
/* Copy samples from Src so that in Dst, samples are organized into subbands */
void FormSubbands(int NLevels, const grid& SrcGrid, const volume& SrcVol,
                  const grid& DstGrid, volume* DstVol);
/* Assume the wavelet transform is done in X, then Y, then Z */
int LevelToSubband(const v3i& Lvl3);
v3i ExpandDomain(const v3i& N, int NLevels);

/* If Norm(alize), the return levels are either 0 or 1 */
v3i SubbandToLevel(int NDims, int Sb, bool Norm = false);

struct wav_grids {
  grid WavGrid; // grid of wavelet coefficients to copy
  grid ValGrid; // the output grid of values
  grid WrkGrid; // determined using the WavGrid
};
wav_grids ComputeWavGrids(int NDims, int Sb, const extent& ValExt, const grid& WavGrid, const v3i& ValStrd);

/* Return the footprint (influence range) of a block of wavelet coefficients */
extent WavFootprint(int NDims, int Sb, const grid& WavGrid);

} // namespace mg

#include "mg_wavelet.inl"
