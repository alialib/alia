#ifndef ALIA_ABI_PRELUDE_H
#define ALIA_ABI_PRELUDE_H

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

// TODO: Sort this out.
#define ALIA_API
// #if defined(_WIN32) || defined(__CYGWIN__)
// #if defined(ALIA_BUILD_SHARED)
// #define ALIA_API __declspec(dllexport)
// #else
// #define ALIA_API __declspec(dllimport)
// #endif
// #else
// #define ALIA_API __attribute__((visibility("default")))
// #endif

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

// Add C++ interface code inline in a C ABI file.
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

/* OPERATORS - These are used to define C++ operator overloads for C types. */

// Using this tricks clang-format into doing top-level alignment on the macro
// code below.
#define ALIA_CONCAT_BLOCKS(a, b) a b

// Define `==` and `!=` from the `<type>_equal` function.
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

// Define `+` and `+=` from `<type>_add` and `<type>_add_inplace`.
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

// Define `-` and `-=` from `<type>_sub` and `<type>_sub_inplace`.
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

// Define `*` and `*=` from `<type>_scale` and `<type>_scale_inplace`.
// Note that this defines `*` with the scalar on both the left and the right.
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

/* OPAQUE OBJECT INITIALIZATION */

// `alia_struct_spec` defines the size and alignment of an opaque object.
typedef struct alia_struct_spec
{
    size_t size;
    size_t align;
} alia_struct_spec;

/* ALIGNMENT */

// `ALIA_MIN_ALIGN` is the global minimum and *default* alignment for all
// allocations done by Alia. All allocations are aligned to at least this
// value, and in the allocation 'fast path', sizes must be a multiple of this
// value.
#define ALIA_MIN_ALIGN (16u)
// `ALIA_MAX_ALIGN` is the global maximum alignment for all allocations done by
// Alia. Allocations are aligned to at most this value.
#define ALIA_MAX_ALIGN (256u)

// Align a value up to a power-of-two.
static inline size_t
alia_align_up(size_t x, size_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

// Align a value up to the global minimum alignment.
// macro version
#define ALIA_MIN_ALIGNED_SIZE(n)                                              \
    (((n) + ALIA_MIN_ALIGN - 1) & ~(ALIA_MIN_ALIGN - 1))
// function version
static inline size_t
alia_min_aligned_size(size_t n)
{
    return ((n) + ALIA_MIN_ALIGN - 1) & ~(ALIA_MIN_ALIGN - 1);
}

#endif
