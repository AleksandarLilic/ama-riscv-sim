// single compilation unit for nanoprintf; feature flags can be overridden
// per-test via CFLAGS or USER_DEFINES in the test's Makefile

// field width (%8d, %-4s, etc.)
#ifndef NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#endif

// precision (%.4s, used implicitly by float if enabled)
#ifndef NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 0
#endif

// %f / %e / %g; requires precision above; pulls in soft-float on ilp32
#ifndef NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#endif

// %ll / %z / %t length modifiers
#ifndef NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#endif

// %hh / %h length modifiers
#ifndef NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#endif

// %b binary output
#ifndef NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#endif

// %n writeback (almost never needed)
#ifndef NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#endif

#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf/nanoprintf.h"
