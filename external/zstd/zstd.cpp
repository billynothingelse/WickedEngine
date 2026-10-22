// Implementation of the zstd header-only library, compiled once for the whole engine.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

#ifndef __SCE__
// prevent linking problems when using ASAN
extern "C" {
#endif // __SCE__

#include "zstd.c"

#ifndef __SCE__
}
#endif // __SCE__
