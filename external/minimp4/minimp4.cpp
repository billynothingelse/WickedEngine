// Implementation of the minimp4 header-only library, compiled once for the whole engine.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

#ifndef __SCE__
// prevent linking problems when using ASAN
extern "C" {
#endif // __SCE__

#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
#undef RETURN_ERROR

#ifndef __SCE__
}
#endif // __SCE__
