#ifndef ALIA_SIGNAL_FIELD_MACROS_HPP
#define ALIA_SIGNAL_FIELD_MACROS_HPP

#include <alia/signals/operators.hpp>

// __declspec(property(...)) is non-standard.
#if defined(__clang__) || defined(_MSC_VER)

#define ALIA_PP_CONCAT(a, b) ALIA_PP_CONCAT1(a, b)
#define ALIA_PP_CONCAT1(a, b) ALIA_PP_CONCAT2(a, b)
#define ALIA_PP_CONCAT2(a, b) a##b

#define ALIA_PP_FE_2_0(F, a, b)
#define ALIA_PP_FE_2_1(F, a, b, x) F(a, b, x)
#define ALIA_PP_FE_2_2(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_1(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_3(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_2(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_4(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_3(F, a, b, __VA_ARGS__)
#define ALIA_PP_FE_2_5(F, a, b, x, ...)                                       \
    F(a, b, x) ALIA_PP_FE_2_4(F, a, b, __VA_ARGS__)

#define ALIA_PP_GET_MACRO(_0, _1, _2, _3, _4, _5, NAME, ...) NAME
#define ALIA_PP_FOR_EACH_2(F, a, b, ...)                                      \
    ALIA_PP_GET_MACRO(                                                        \
        _0,                                                                   \
        __VA_ARGS__,                                                          \
        ALIA_PP_FE_2_5,                                                       \
        ALIA_PP_FE_2_4,                                                       \
        ALIA_PP_FE_2_3,                                                       \
        ALIA_PP_FE_2_2,                                                       \
        ALIA_PP_FE_2_1,                                                       \
        ALIA_PP_FE_2_0)                                                       \
    (F, a, b, __VA_ARGS__)

#define ALIA_DEFINE_STRUCT_SIGNAL_FIELD(signal_type, struct_name, field_name) \
    auto ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)()         \
    {                                                                         \
        return (*this)->*&struct_name::field_name;                            \
    }                                                                         \
    __declspec(property(                                                      \
        get = ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)))    \
        alia::field_signal<                                                   \
            ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),      \
            decltype(struct_name::field_name)>                                \
            field_name;

#define ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(signal_type, struct_name, ...)       \
    ALIA_PP_FOR_EACH_2(                                                       \
        ALIA_DEFINE_STRUCT_SIGNAL_FIELD,                                      \
        signal_type,                                                          \
        struct_name,                                                          \
        __VA_ARGS__)

#define ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                     \
    signal_name, signal_type, struct_name, ...)                               \
    struct signal_name : alia::signal_type<struct_name>                       \
    {                                                                         \
        using signal_ref::signal_ref;                                         \
        ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(                                     \
            signal_type, struct_name, __VA_ARGS__)                            \
    };

#define ALIA_DEFINE_STRUCT_SIGNAL(signal_type, struct_name, ...)              \
    ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                         \
        ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),          \
        signal_type,                                                          \
        struct_name,                                                          \
        __VA_ARGS__)

#define ALIA_DEFINE_STRUCT_SIGNALS(struct_name, ...)                          \
    ALIA_DEFINE_STRUCT_SIGNAL(readable, struct_name, __VA_ARGS__)             \
    ALIA_DEFINE_STRUCT_SIGNAL(duplex, struct_name, __VA_ARGS__)

#endif

#endif
