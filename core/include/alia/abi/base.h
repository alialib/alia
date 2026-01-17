#ifndef ALIA_C_BASE_H
#define ALIA_C_BASE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

/* BRACED INITIALIZATION */
// clang-format off
#ifdef __cplusplus
#  define ALIA_BRACED_INIT(T, ...) T{ __VA_ARGS__ }
#else
#  define ALIA_BRACED_INIT(T, ...) (T){ __VA_ARGS__ }
#endif
// clang-format on

/* INLINE C++ */

#ifdef __cplusplus
#define ALIA_INLINE_CPP(ns, code)                                             \
    ALIA_EXTERN_C_END                                                         \
    namespace ns {                                                            \
    code                                                                      \
    }                                                                         \
    ALIA_EXTERN_C_BEGIN
#else
#define ALIA_INLINE_CPP(...)
#endif

#define ALIA_CONCAT_BLOCKS(a, b) a b

#ifdef __cplusplus
#define ALIA_DEFINE_EQUALITY_OPERATOR(type)                                   \
    ALIA_INLINE_CPP(                                                          \
        alia::operators,                                                      \
        ALIA_CONCAT_BLOCKS(                                                   \
            static inline bool operator==(type a, type b) {                   \
                return type##_equal(a, b);                                    \
            },                                                                \
            static inline bool operator!=(type a, type b) {                   \
                return !type##_equal(a, b);                                   \
            }))
#else
#define ALIA_DEFINE_EQUALITY_OPERATOR(type)
#endif

#ifdef __cplusplus
#define ALIA_DEFINE_PLUS_OPERATOR(type)                                       \
    ALIA_INLINE_CPP(                                                          \
        alia::operators,                                                      \
        ALIA_CONCAT_BLOCKS(                                                   \
            static inline type operator+(type a, type b) {                    \
                return type##_add(a, b);                                      \
            },                                                                \
            static inline type& operator+=(type& a, type b) {                 \
                type##_add_inplace(&a, b);                                    \
                return a;                                                     \
            }))
#else
#define ALIA_DEFINE_PLUS_OPERATOR(type)
#endif

#ifdef __cplusplus
#define ALIA_DEFINE_MINUS_OPERATOR(type)                                      \
    ALIA_INLINE_CPP(                                                          \
        alia::operators,                                                      \
        ALIA_CONCAT_BLOCKS(                                                   \
            static inline type operator-(type a, type b) {                    \
                return type##_sub(a, b);                                      \
            },                                                                \
            static inline type& operator-=(type& a, type b) {                 \
                type##_sub_inplace(&a, b);                                    \
                return a;                                                     \
            }))
#else
#define ALIA_DEFINE_MINUS_OPERATOR(type)
#endif

#ifdef __cplusplus
#define ALIA_DEFINE_SCALE_OPERATOR(type, scalar)                              \
    ALIA_INLINE_CPP(                                                          \
        alia::operators,                                                      \
        ALIA_CONCAT_BLOCKS(                                                   \
            ALIA_CONCAT_BLOCKS(                                               \
                static inline type operator*(type a, scalar b) {              \
                    return type##_scale(a, b);                                \
                },                                                            \
                static inline type operator*(scalar a, type b) {              \
                    return type##_scale(b, a);                                \
                }),                                                           \
            static inline type& operator*=(type& a, scalar b) {               \
                type##_scale_inplace(&a, b);                                  \
                return a;                                                     \
            }))
#else
#define ALIA_DEFINE_SCALE_OPERATOR(type, scalar)
#endif

#endif
