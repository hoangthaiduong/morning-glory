#include "mg_assert.h"
#include "mg_bitops.h"
#include "mg_common_types.h"
#include "mg_expected.h"
#include "mg_io.h"
#include "mg_math.h"
#include "mg_memory.h"
#include "mg_types.h"
#include "mg_volume.h"

namespace mg {

error ReadVolume(cstr FileName, v3i Dims, data_type Type, volume* Volume) {
  error Ok = ReadFile(FileName, &Volume->Buffer);
  if (!Ok)
    return Ok;
  Volume->Dims = Stuff3Ints(Dims);
  Volume->Type = Type;
  return mg_Error(NoError);
}

void Copy(sub_volume* Dst, const sub_volume& Src) {
#define Body(type)\
  mg_Assert(Src.Extent.Dims == Dst->Extent.Dims);\
  mg_Assert(Dst->Buffer.Data && Src.Buffer.Data);\
  mg_Assert(Dst->Type == Src.Type);\
  type* DstBuf = (type*)Dst->Buffer.Data;\
  const type* SrcBuf = (const type*)Src.Buffer.Data;\
  v3i StartSrc = Extract3Ints(Src.Extent.Pos);\
  v3i StartDst = Extract3Ints(Dst->Extent.Pos);\
  v3i Dims = Extract3Ints(Src.Extent.Dims);\
  v3i BigDimsSrc = Extract3Ints(Src.Dims);\
  v3i BigDimsDst = Extract3Ints(Dst->Dims);\
  v3i PosSrc = StartSrc;\
  v3i PosDst = StartDst;\
  for (PosSrc.Z = StartSrc.Z, PosDst.Z = StartDst.Z;\
       PosSrc.Z < StartSrc.Z + Dims.Z; ++PosSrc.Z, ++PosDst.Z) {\
  for (PosSrc.Y = StartSrc.Y, PosDst.Y = StartDst.Y;\
       PosSrc.Y < StartSrc.Y + Dims.Y; ++PosSrc.Y, ++PosDst.Y) {\
  for (PosSrc.X = StartSrc.X, PosDst.X = StartDst.X;\
       PosSrc.X < StartSrc.X + Dims.X; ++PosSrc.X, ++PosDst.X) {\
    i64 I = XyzToI(BigDimsSrc, PosSrc);\
    i64 J = XyzToI(BigDimsDst, PosDst);\
    DstBuf[J] = SrcBuf[I];\
  }}}\

  TypeChooser(Src.Type);
#undef Body
}

volume Clone(const volume& Vol, allocator* Alloc) {
  volume VolCopy;
  VolCopy.Buffer = Clone(Vol.Buffer, Alloc);
  VolCopy.Type = Vol.Type;
  return VolCopy;
}

array<extent, 8> Split3D(v3i Dims) {
  mg_Assert(Dims.X > 1 && Dims.Y > 1 && Dims.Z > 1);
  array<extent, 8> Vols;
  Vols[0] = extent(v3i(0, 0, 0), v3i(1, 1, 1));
  Vols[1] = extent(v3i(1, 0, 0), v3i(Dims.X - 1, 0, 0));
  Vols[2] = extent(v3i(0, 1, 0), v3i(0, Dims.Y - 1, 0));
  Vols[3] = extent(v3i(0, 0, 1), v3i(0, 0, Dims.Z - 1));
  Vols[4] = extent(v3i(1, 1, 0), v3i(Dims.X - 1, Dims.Y - 1, 0));
  Vols[5] = extent(v3i(0, 1, 1), v3i(0, Dims.Y - 1, Dims.Z - 1));
  Vols[6] = extent(v3i(1, 0, 1), v3i(Dims.X - 1, 0, Dims.Z - 1));
  Vols[7] = extent(v3i(1, 1, 1), v3i(Dims.X - 1, Dims.Y - 1, Dims.Z - 1));
  return Vols;
}

} // namespace mg

