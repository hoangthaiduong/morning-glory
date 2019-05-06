#pragma once

#include "mg_common.h"
#include "mg_data_types.h"
#include "mg_error.h"

namespace mg {

/* 
In string form:
file = C:/My Data/my file.raw
name = combustion
field = o2
dimensions = 512 512 256
data type = float32 */
struct metadata {
  char File[256] = "";
  char Name[32] = "";
  char Field[32] = "";
  v3i Dims = v3i(0, 0, 0);
  data_type Type = data_type(data_type::__Invalid__);
  inline thread_local static char String[384];
}; // struct metadata

cstr ToString(const metadata& Meta);
error<> ReadMeta(cstr FileName, metadata* Meta);
error<> ParseMeta(stref FilePath, metadata* Meta);

} // namespace mg
