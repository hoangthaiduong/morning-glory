#pragma once

#include "mg_common.h"
#include "mg_data_types.h"
#include "mg_error.h"
#include "mg_macros.h"
#include "mg_memory.h"

namespace mg {

enum dimension { X, Y, Z };

struct volume;

struct extent {
  u64 From = 0, Dims = 0;
  extent();
  explicit extent(const v3i& Dims3);
  explicit extent(const volume& Vol);
  extent(const v3i& From3, const v3i& Dims3);
  operator bool() const;
  static extent Invalid();
};

struct grid : public extent {
  u64 Strd = 0;
  grid();
  explicit grid(const v3i& Dims3);
  explicit grid(const volume& Vol);
  grid(const v3i& From3, const v3i& Dims3);
  grid(const v3i& From3, const v3i& Dims3, const v3i& Strd3);
  explicit grid(const extent& Ext);
  operator bool() const;
  static grid Invalid();
};

// TODO: do not allocate in the constructor, use Alloc() function
struct volume {
  buffer Buffer = {};
  u64 Dims = 0;
  dtype Type = dtype::__Invalid__;
  volume();
  volume(const buffer& Buf, const v3i& Dims3, dtype TypeIn);
  volume(const v3i& Dims3, dtype TypeIn, allocator* Alloc = &Mallocator());
  mg_T(t) volume(const t* Ptr, i64 Size);
  mg_T(t) volume(const t* Ptr, const v3i& Dims3);
  mg_T(t) explicit volume(const buffer_t<t>& Buf);
  mg_T(t) t& At(const v3i& P) const;
  mg_T(t) t& At(i64 Idx) const;
};

/* Represent a volume storing samples of a sub-grid to a larger grid */
struct subvol_grid {
  volume Vol;
  grid Grid;
  explicit subvol_grid(const volume& VolIn);
  subvol_grid(const grid& GridIn, const volume& VolIn);
};

#define mg_PrStrGrid "[" mg_PrStrV3i mg_PrStrV3i mg_PrStrV3i "]"
#define mg_PrGrid(G) mg_PrV3i(From(G)), mg_PrV3i(Dims(G)), mg_PrV3i(Strd(G))

bool operator==(const volume& V1, const volume& V2);

v3i Dims(const v3i& First, const v3i& Last);
v3i Dims(const v3i& First, const v3i& Last, const v3i& Strd);

v3i From(const extent& Ext);
v3i To(const extent& Ext);
v3i Frst(const extent& Ext);
v3i Last(const extent& Ext);
v3i Dims(const extent& Ext);
v3i Strd(const extent& Ext);
i64 Size(const extent& Ext);
void SetFrom(extent* Ext, const v3i& From3);
void SetDims(extent* Ext, const v3i& Dims3);

v3i From(const grid& Grid);
v3i To(const grid& Grid);
v3i Frst(const grid& Grid);
v3i Last(const grid& Grid);
v3i Dims(const grid& Grid);
v3i Strd(const grid& Grid);
i64 Size(const grid& Grid);
void SetFrom(grid* Grid, const v3i& From3);
void SetDims(grid* Grid, const v3i& Dims3);
void SetStrd(grid* Grid, const v3i& Strd3);

v3i From(const volume& Vol);
v3i To(const volume& Vol);
v3i Frst(const volume& Vol);
v3i Last(const volume& Vol);
v3i Dims(const volume& Vol);
v3i Strd(const volume& Vol);
i64 Size(const volume& Vol);
void SetDims(volume* Vol, const v3i& Dims3);

i64 Row(const v3i& N, const v3i& P);
v3i InvRow(i64 I, const v3i& N);

//grid Intersect();

mg_T(t)
struct volume_iterator {
  t* Ptr = nullptr;
  v3i P = {}, N = {};
  volume_iterator& operator++();
  t& operator*();
  bool operator!=(const volume_iterator& Other) const;
  bool operator==(const volume_iterator& Other) const;
};
mg_T(t) volume_iterator<t> Begin(const volume& Vol);
mg_T(t) volume_iterator<t> End(const volume& Vol);

mg_T(t)
struct extent_iterator {
  t* Ptr = nullptr;
  v3i P = {}, D = {}, N = {};
  extent_iterator& operator++();
  t& operator*();
  bool operator!=(const extent_iterator& Other) const;
  bool operator==(const extent_iterator& Other) const;
};
mg_T(t) extent_iterator<t> Begin(const extent& Ext, const volume& Vol);
mg_T(t) extent_iterator<t> End(const extent& Ext, const volume& Vol);
// TODO: merge grid_iterator and grid_indexer?
// TODO: add extent_iterator and dimension_iterator?

mg_T(t)
struct grid_iterator {
  t* Ptr = nullptr;
  v3i P = {}, D = {}, S = {}, N = {};
  grid_iterator& operator++();
  t& operator*();
  bool operator!=(const grid_iterator& Other) const;
  bool operator==(const grid_iterator& Other) const;
};
mg_T(t) grid_iterator<t> Begin(const grid& Grid, const volume& Vol);
mg_T(t) grid_iterator<t> End(const grid& Grid, const volume& Vol);

/* Read a volume from a file. */
error<> ReadVolume(cstr FileName, const v3i& Dims3, dtype Type, volume* Vol);

void Resize(volume* Vol, const v3i& Dims3);
void Resize(volume* Vol, const v3i& Dims3, dtype Type);
void Dealloc(volume* Vol);

mg_T(t) error<> WriteVolume(FILE* Fp, const volume& Vol, const t& Grid);

/* Copy a region of the first volume to a region of the second volume */
mg_T(t) void Copy(const t& SGrid, const volume& SVol, volume* DVol);
mg_TT(t1, t2) void Copy(const t1& SGrid, const volume& SVol, const t2& DGrid, volume* DVol);
/* Copy the part of SVol that overlaps with Grid to DVol. All grids are defined
on some "global" coordinate system. */
mg_T(t) void Copy(const t& Grid, const subvol_grid& SVol, subvol_grid* DVol);
/* Similar to copy, but add the source to the destination instead */
mg_TT(t1, t2) void Add(const t1& SGrid, const volume& SVol, const t2& DGrid, volume* DVol);
/* Returns whether Grid1 is a sub-grid of Grid2 */
mg_TT(t1, t2) bool IsSubGrid(const t1& Grid1, const t2& Grid2);
mg_TT(t1, t2) t1 SubGrid(const t1& Grid1, const t2& Grid2);
/* Compute the position of Grid1 relative to Grid2 (Grid1 is a sub-grid of Grid2) */
mg_TT(t1, t2) t1 Relative(const t1& Grid1, const t2& Grid2);
/* "Crop" Grid1 against Grid2 */
mg_TT(t1, t2) t1 Crop(const t1& Grid1, const t2& Grid2);
mg_T(t) bool IsInGrid(const t& Grid, const v3i& Point);

/* Return a slab (from Grid) of size N in the direction of D. If N is positive,
take from the lower end, otherwise take from the higher end. Useful for e.g.,
taking a boundary face/column/corner of a block. */
mg_T(t) t Slab(const t& Grid, dimension D, int N);
mg_T(t) t Translate(const t& Grid, dimension D, int N);

void Clone(const volume& Src, volume* Dst, allocator* Alloc = &Mallocator());

/* Return the number of dimensions, given a volume size */
int NumDims(const v3i& N);

#define mg_BeginGridLoop(G, V) // G is a grid and V is a volume
#define mg_EndGridLoop
#define mg_BeginGridLoop2(GI, VI, GJ, VJ) // loop through two grids in lockstep
#define mg_EndGridLoop2

} // namespace mg

#include "mg_volume.inl"
