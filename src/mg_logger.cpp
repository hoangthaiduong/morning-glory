#include <stdio.h>
#include <string.h>
#include "mg_assert.h"
#include "mg_logger.h"
#include "mg_io.h"
#include "mg_utils.h"

namespace mg {

void
SetBufferMode(logger* Logger, buffer_mode Mode) { Logger->Mode = Mode; }

void 
SetBufferMode(buffer_mode Mode) { SetBufferMode(&GlobalLogger, Mode); }

FILE* 
GetFileHandle(logger* Logger, cstr FileName) {
  int MaxSlots = Size(Logger->FHandles);
  u32 FullHash = Murmur3_32((u8*)FileName, strlen(FileName), 37);
  int Idx = FullHash % MaxSlots;
  FILE** Fp = &Logger->FHandles[Idx];
  bool Collision = *Fp && (Logger->FNameHashes[Idx] != FullHash ||
                           strncmp(FileName, Logger->FNames[Idx], 64) != 0);
  if (Collision) { // collision, find the next empty slot
    for (int I = 0; I < MaxSlots; ++I) {
      int J = (Idx + I) % MaxSlots;
      Fp = &Logger->FHandles[J];
      if (!Logger->FNames[J] || strncmp(FileName, Logger->FNames[J], 64) == 0) {
        Idx = J;
        break;
      }
    }
  }
  if (!*Fp) { // empty slot, open a new file for logging
    mg_Assert(!Logger->FNames[Idx]);
    mg_Assert(!Logger->FHandles[Idx]);
    *Fp = fopen(FileName, "w");
    mg_AbortIf(!*Fp, "File %s cannot be created", FileName);
    if (Logger->Mode == buffer_mode::Full) // full buffering
      setvbuf(*Fp, nullptr, _IOFBF, BUFSIZ);
    else if (Logger->Mode == buffer_mode::Line) // line buffering (same as full buffering on Windows)
      setvbuf(*Fp, nullptr, _IOLBF, BUFSIZ);
    else // no buffering
      setvbuf(*Fp, nullptr, _IONBF, 0);
    Logger->FNames[Idx] = FileName;
    Logger->FNameHashes[Idx] = FullHash;
  } else if (Collision) { // no more open slots
    mg_Abort("No more logger slots");
  }
  return *Fp;
}

} // namespace mg

