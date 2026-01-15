#ifndef ALIA_C_BASE_CONFIG_H
#define ALIA_C_BASE_CONFIG_H

/* C / C++ LINKAGE */

#ifdef __cplusplus
#define ALIA_EXTERN_C_BEGIN extern "C" {
#define ALIA_EXTERN_C_END }
#else
#define ALIA_EXTERN_C_BEGIN
#define ALIA_EXTERN_C_END
#endif

/* EXPORT / IMPORT */

/* Define ALIA_BUILD_SHARED when building the Alia shared library.
   Consumers should NOT define this. */

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(ALIA_BUILD_SHARED)
#define ALIA_API __declspec(dllexport)
#else
#define ALIA_API __declspec(dllimport)
#endif
#else
#define ALIA_API __attribute__((visibility("default")))
#endif

/* DEBUG / ASSERT */

#include <assert.h>

#if defined(NDEBUG)
#define ALIA_ASSERT(expr) ((void) 0)
#else
#define ALIA_ASSERT(expr) assert(expr)
#endif

#endif /* ALIA_C_BASE_CONFIG_H */
