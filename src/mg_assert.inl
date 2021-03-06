#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "mg_debugbreak.h"
#include "mg_io.h"
#include "mg_macros.h"
#include "mg_stacktrace.h"

#define mg_AssertHelper(Debug, Cond, ...)\
  do {\
    if (!(Cond)) {\
      fprintf(stderr, "Condition \"%s\" failed, ", #Cond);\
      fprintf(stderr, "in file %s, line %d\n", __FILE__, __LINE__);\
      if (mg_NumArgs(__VA_ARGS__) > 0) {\
        mg_FPrintHelper(stderr, __VA_ARGS__);\
        fprintf(stderr, "\n");\
      }\
      mg::printer Pr(stderr);\
      mg::PrintStacktrace(&Pr);\
      if (Debug)\
        debug_break();\
      else\
        exit(EXIT_FAILURE);\
    }\
  } while (0);

#undef mg_Assert
#if defined(mg_Slow)
  #define mg_Assert(Cond, ...) mg_AssertHelper(true, (Cond), __VA_ARGS__)
#else
  #define mg_Assert(Cond, ...) do {} while (0)
#endif

#undef mg_AbortIf
#define mg_AbortIf(Cond, ...)\
  mg_AssertHelper(false, !(Cond) && "Fatal error!", __VA_ARGS__)
#undef mg_Abort
#define mg_Abort(...) mg_AbortIf(true, __VA_ARGS__)
