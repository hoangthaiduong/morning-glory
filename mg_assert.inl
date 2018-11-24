#pragma once

#include <stdio.h>
#include "mg_debugbreak.h"
#include "mg_macros.h"

#undef mg_Assert
#undef mg_AssertMsg
#undef mg_AssertFmt

#if defined(mg_Slow)
  #define mg_Assert(Cond) \
    do { \
      if (!(Cond)) { \
        fprintf(stderr, "Condition %s failed, ", #Cond); \
        fprintf(stderr, "in file %s, line %d\n", __FILE__, __LINE__); \
        debug_break(); \
      } \
    } while (0)
  #define mg_AssertMsg(Cond, Msg) \
    do { \
      if (!(Cond)) { \
        fprintf(stderr, "Condition %s failed, ", #Cond); \
        fprintf(stderr, "in file %s, line %d: ", __FILE__, __LINE__); \
        fprintf(stderr, Msg);\
        debug_break(); \
      } \
    } while (0)
  #define mg_AssertFmt(Cond, Fmt, ...) \
    do { \
      if (!(Cond)) { \
        fprintf(stderr, "Condition %s failed, ", #Cond); \
        fprintf(stderr, "in file %s, line %d: ", __FILE__, __LINE__); \
        fprintf(stderr, Fmt, __VA_ARGS__);\
        debug_break(); \
      } \
    } while (0)
#else
  #define mg_Assert(Cond) ;
  #define mg_AssertMsg(Cond, Msg) ;
  #define mg_AssertFmt(Cond, Fmt, ...) ;
#endif