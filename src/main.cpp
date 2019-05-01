#include "mg_array.h"
#include "mg_args.h"
#include "mg_assert.h"
#include "mg_bitops.h"
#include "mg_bitstream.h"
#include "mg_common_types.h"
#include "mg_dataset.h"
#include "mg_encode.h"
#include "mg_enum.h"
#include "mg_error.h"
#include "mg_expected.h"
#include "mg_file_format.h"
#include "mg_filesystem.h"
#include "mg_linked_list.h"
#include "mg_logger.h"
#include "mg_io.h"
#include "mg_math.h"
#include "mg_memory.h"
#include "mg_memory_map.h"
#include "mg_scopeguard.h"
#include "mg_signal_processing.h"
#include "mg_dataset.h"
#include "mg_timer.h"
#include "mg_types.h"
#include "mg_volume.h"
#include "mg_zfp.h"
#include "mg_wavelet.h"
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/default_executor.hpp>

using namespace mg;

// TODO: memory out-of-bound/leak detection

mg_Enum(action, int, Encode, Decode)

struct params {
  action Action = action::Encode;
  cstr DataFile = nullptr;
  cstr CompressedFile = nullptr;
  int NLevels = 0;
  int NBitPlanes = 0;
  v3i TileDims = v3i(0, 0, 0);
  int ChunkBytes = 0;
  f64 Tolerance = 0;
  metadata Meta;
};

// TODO: when decoding, construct the raw file name from info embedded inside
// the compressed file
// TODO: add --precision for decoding
params ParseParams(int Argc, const char** Argv) {
  params P;
  if (OptionExists(Argc, Argv, "--encode"))
    P.Action = action::Encode;
  else if (OptionExists(Argc, Argv, "--decode"))
    P.Action = action::Decode;
  else
    mg_Abort("Provide either --encode or --decode");
  mg_AbortIf(!GetOptionValue(Argc, Argv, "--compressed_file", &P.CompressedFile),
    "Provide --compressed_files");
  mg_AbortIf(!GetOptionValue(Argc, Argv, "--raw_file", &P.DataFile),
    "Provide --raw_file");
  error Err = ParseMeta(P.DataFile, &P.Meta);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  mg_AbortIf(Prod<i64>(P.Meta.Dims) > Traits<i32>::Max,
    "Data dimensions too big");
  mg_AbortIf(P.Meta.Type != data_type::float32 &&
             P.Meta.Type != data_type::float64, "Data type not supported");
  if (P.Action == action::Encode) {
    mg_AbortIf(!GetOptionValue(Argc, Argv, "--num_levels", &P.NLevels),
      "Provide --num_levels");
    mg_AbortIf(!GetOptionValue(Argc, Argv, "--tile_dims", &P.TileDims),
      "Provide --tile_dims");
    mg_AbortIf(!GetOptionValue(Argc, Argv, "--chunk_bytes", &P.ChunkBytes),
      "Provide --chunk_bytes");
    mg_AbortIf(!GetOptionValue(Argc, Argv, "--precision", &P.NBitPlanes),
      "Provide --precision");
    mg_AbortIf(P.NBitPlanes > BitSizeOf(P.Meta.Type),
      "precision too high");
    mg_AbortIf(!GetOptionValue(Argc, Argv, "--tolerance", &P.Tolerance),
      "Provide --tolerance");
  }
  return P;
}

void TestMemMap(const char* Input) {
  /* test write */
  mmap_file MMap;
  auto Err = open_file("test.raw", map_mode::Write, &MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = map_file(&MMap, 128);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  MMap.Buf[4] = 'h';
  MMap.Buf[5] = 'e';
  MMap.Buf[6] = 'l';
  MMap.Buf[7] = 'l';
  MMap.Buf[8] = 'o';
  Err = flush_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = sync_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = unmap_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = close_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  /* test read */
  Err = open_file(Input, map_mode::Read, &MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = map_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  printf("%x\n", MMap.Buf[4096]);
  printf("%x\n", MMap.Buf[4096 * 2]);
  printf("%x\n", MMap.Buf[4096 * 3]);
  printf("%x\n", MMap.Buf[500000000]);
  Err = unmap_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
  Err = close_file(&MMap);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));
}

#define Array { 56, 40, 8, 24, 48, 48, 40, 16, 30,\
                40, 8, 24, 48, 48, 40, 16, 30, 56,\
                8, 24, 48, 48, 40, 16, 30, 56, 40,\
                24, 48, 48, 40, 16, 30, 56, 40, 8,\
                48, 48, 40, 16, 30, 56, 40, 8, 24,\
                48, 40, 16, 30, 56, 40, 8, 24, 48,\
                40, 16, 30, 56, 40, 8, 24, 48, 48,\
                16, 30, 56, 40, 8, 24, 48, 48, 40,\
                30, 56, 40, 8, 24, 48, 48, 40, 16 }


void TestNewWaveletCode() {
  double A[] = Array;
  double C[] = Array;
  double B[] = Array;
  volume Vol;
  Vol.Buffer.Data = (byte*)A;
  Vol.Buffer.Bytes = sizeof(A);
  Vol.DimsCompact = Pack3Ints64(v3i(9, 9, 1));
  Vol.Type = data_type::float64;
  Vol.Extent.PosCompact = 0;
  Vol.Extent.DimsCompact = Pack3Ints64(v3i(9, 9, 1));
  Vol.Extent.StridesCompact = Pack3Ints64(v3i(1, 1, 1));
  extent Ext = Vol.Extent;
  int NLevels = 3;
  volume Vol2 = Vol;
  Vol2.Buffer.Data = (byte*)C;
  Vol2.Buffer.Bytes = sizeof(C);
  for (int I = 0; I < NLevels; ++I) {
    ForwardLiftCdf53X(B, v3i(9, 9, 1), v3i(I, I, I));
    ForwardLiftCdf53Y(B, v3i(9, 9, 1), v3i(I, I, I));
    FLiftCdf53X<double>(Vol, Ext);
    FLiftCdf53Y<double>(Vol, Ext);
    Ext.StridesCompact = Pack3Ints64(Strides(Ext) * 2);
    Ext.DimsCompact = Pack3Ints64((Dims(Ext) + 1) / 2);
  }
  FormSubbands(&Vol2, Vol, NLevels);
  printf("hello3\n");
  printf("\n--------A:\n");
  for (int I = 0; I < sizeof(A) / sizeof(double); ++I) {
    printf("%f ", A[I]);
  }
  printf("\n--------B:\n");
  for (int I = 0; I < sizeof(B) / sizeof(double); ++I) {
    printf("%f ", B[I]);
  }
  printf("\n--------C:\n");
  for (int I = 0; I < sizeof(C) / sizeof(double); ++I) {
    printf("%f ", C[I]);
  }
}

#include <iostream>
#include <new>

// TODO: handle float/int/int64/etc
int main(int Argc, const char** Argv) {
  TestNewWaveletCode();
  dynamic_array<extent> Subbands;
  BuildSubbandsInPlace(v3i(16, 16, 16), 3, &Subbands);
  //for (int I = 0; I < Size(Subbands); ++I) {
  //  printf("----------- Subband %d\n", I);
  //  v3i Pos = Unpack3Ints64(Subbands[I].PosCompact);
  //  v3i Dims = Unpack3Ints64(Subbands[I].DimsCompact);
  //  v3i Strides = Unpack3Ints64(Subbands[I].StridesCompact);
  //  printf("Pos %d %d %d\n", Pos.X, Pos.Y, Pos.Z);
  //  printf("Dims %d %d %d\n", Dims.X, Dims.Y, Dims.Z);
  //  printf("Strides %d %d %d\n", Strides.X, Strides.Y, Strides.Z);
  //}
  return 0;
  //int ChunkSize, BlockBegin, BlockEnd;
  //ToInt(Argv[1], &ChunkSize);
  //ToInt(Argv[2], &BlockBegin);
  //ToInt(Argv[3], &BlockEnd);
  //OldEncode(BlockBegin, BlockEnd);
  //TestEncoder(ChunkSize, BlockBegin, BlockEnd);
  //return 0;

  /* Read the parameters */
  SetHandleAbortSignals();
  params P = ParseParams(Argc, Argv);
  file_format Ff;
  SetWaveletTransform(&Ff, P.NLevels);
  SetTileDims(&Ff, P.TileDims);
  SetChunkBytes(&Ff, P.ChunkBytes);
  SetPrecision(&Ff, P.NBitPlanes);
  SetTolerance(&Ff, P.Tolerance);
  SetFileName(&Ff, P.CompressedFile);

  /* Read the raw file */
  volume F;
  AllocBuf(&F.Buffer, SizeOf(P.Meta.Type) * Prod(P.Meta.Dims));
  mg_CleanUp(0, DeallocBuf(&F.Buffer));
  error Err = ReadVolume(P.Meta.File, P.Meta.Dims, P.Meta.Type, &F);
  mg_AbortIf(ErrorExists(Err), "%s", ToString(Err));

  /* Perform the action */
  if (P.Action == action::Encode) {
    SetVolume(&Ff, F.Buffer.Data, BigDims(F), F.Type);
    ff_err FfErr = Encode(&Ff, P.Meta);
    mg_AbortIf(ErrorExists(FfErr), "%s", ToString(FfErr));
#if defined(mg_CollectStats)
    Log("stats_encode.txt");
#endif
  } else { // Decode
    ff_err FfErr = Decode(&Ff, &P.Meta);
    mg_AbortIf(ErrorExists(FfErr), "%s", ToString(FfErr));
    f64 Error = RMSError(F, Ff.Volume);
    f64 Psnr = PSNR(F, Ff.Volume);
    printf("RMSE = %e PSNR = %f\n", Error, Psnr);

#if defined(mg_CollectStats)
    Log("stats_decode.txt");
#endif
  }
  CleanUp(&Ff);
  //mg_Assert(_CrtCheckMemory());
  return 0;
}
