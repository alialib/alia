#pragma once

#include <alia/abi/kernel/substrate.h>

// TODO: Consider defining our own `forward` and `move` functions.
#include <utility>

namespace alia {

template<typename Tag>
inline alia_struct_spec*
get_memoized_block_spec()
{
    static alia_struct_spec value = {0, 0};
    return &value;
}

#define ALIA_CONCAT_(a, b) a##b
#define ALIA_CONCAT(a, b) ALIA_CONCAT_(a, b)

#define ALIA_UNIQUE_ID(base) ALIA_CONCAT(base, __COUNTER__)

#ifndef ALIA_UNUSED
#if defined(__GNUC__) || defined(__clang__)
#define ALIA_UNUSED __attribute__((unused))
#else
#define ALIA_UNUSED
#endif
#endif

// `condition_is_strictly_true(x)` and `condition_is_strictly_false(x)`
// evaluate `x` in a boolean context. Note that due to signal mechanics, these
// are not necessarily opposites of one another.
// TODO: Extend to handle signals.

template<class T>
bool
condition_is_strictly_true(T&& x)
{
    return x ? true : false;
}

template<class T>
bool
condition_is_strictly_false(T&& x)
{
    return x ? false : true;
}

// MSVC likes to warn about shadowed local variables and function parameters,
// but there's really no way to implement these macros without potentially
// shadowing things, so we need to disable those warning within the macros.
// Similarly for Clang warning about unused variables.
#if defined(__clang__)
#define ALIA_DISABLE_MACRO_WARNINGS                                           \
    _Pragma("clang diagnostic push")                                          \
        _Pragma("clang diagnostic ignored \"-Wunknown-warning-option\"")      \
            _Pragma("clang diagnostic ignored \"-Wunused-but-set-variable\"")
#define ALIA_REENABLE_MACRO_WARNINGS _Pragma("clang diagnostic pop")
#elif defined(_MSC_VER)
#define ALIA_DISABLE_MACRO_WARNINGS                                           \
    __pragma(warning(push)) __pragma(warning(disable : 4456))                 \
        __pragma(warning(disable : 4457))
#define ALIA_REENABLE_MACRO_WARNINGS __pragma(warning(pop))
#else
#define ALIA_DISABLE_MACRO_WARNINGS
#define ALIA_REENABLE_MACRO_WARNINGS
#endif

// `if_block` owns/manages the substrate anchor for a single conditional
// branch.
struct if_block
{
    alia_context* ctx = nullptr;
    // substrate anchor for this branch
    alia_substrate_anchor* anchor = nullptr;

    if_block(alia_context* ctx_arg, bool condition)
        : ctx(ctx_arg), anchor(alia_substrate_use_anchor(ctx_arg))
    {
        if (!condition)
        {
            alia_substrate_deactivate_anchor(
                alia_ctx_substrate_system(ctx), anchor);
        }
    }

    if_block(if_block const&) = delete;
    if_block&
    operator=(if_block const&) = delete;
};

// TODO: Address thread-safety of memoized specs.

struct scoped_conditional_block
{
    alia_context* ctx_ = nullptr;
    alia_struct_spec* spec_ = nullptr;

    scoped_conditional_block(
        alia_context* ctx,
        alia_substrate_anchor* anchor,
        alia_struct_spec* spec)
        : ctx_(ctx), spec_(spec)
    {
        alia_substrate_begin_block(ctx_, anchor, spec_);
    }

    ~scoped_conditional_block()
    {
        *spec_ = alia_substrate_end_block(ctx_);
    }

    scoped_conditional_block(scoped_conditional_block const&) = delete;
    scoped_conditional_block&
    operator=(scoped_conditional_block const&) = delete;
};

// The following are macros used to annotate control flow.
// They are used like their C equivalents, but require an `alia_end` after the
// end of their scope.
//
// Note that all come in two forms. One form ends in an underscore and takes
// the context as its first argument. The other form has no trailing underscore
// and assumes that the context is a variable named 'ctx'.

#define ALIA_IF_(ctx, condition)                                              \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    {                                                                         \
        bool _alia_else_condition ALIA_UNUSED;                                \
        {                                                                     \
            auto const& _alia_condition = (condition);                        \
            bool _alia_if_condition                                           \
                = ::alia::condition_is_strictly_true(_alia_condition);        \
            _alia_else_condition                                              \
                = ::alia::condition_is_strictly_false(_alia_condition);       \
            ::alia::if_block _alia_if_block((ctx), _alia_if_condition);       \
            if (_alia_if_condition)                                           \
            {                                                                 \
                ::alia::scoped_conditional_block ALIA_UNUSED _alia_scope(     \
                    (ctx),                                                    \
                    _alia_if_block.anchor,                                    \
                    ::alia::get_memoized_block_spec<struct ALIA_UNIQUE_ID(    \
                        alia_if_block_spec_memo_tag_)>());                    \
                ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_IF(condition) ALIA_IF_ (ctx, condition)

#define ALIA_ELSE_IF_(ctx, condition)                                         \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    }                                                                         \
    }                                                                         \
    {                                                                         \
        auto const& _alia_condition = (condition);                            \
        bool _alia_else_if_condition                                          \
            = _alia_else_condition                                            \
           && ::alia::condition_is_strictly_true(_alia_condition);            \
        _alia_else_condition                                                  \
            = _alia_else_condition                                            \
           && ::alia::condition_is_strictly_false(_alia_condition);           \
        ::alia::if_block _alia_if_block((ctx), _alia_else_if_condition);      \
        if (_alia_else_if_condition)                                          \
        {                                                                     \
            ::alia::scoped_conditional_block ALIA_UNUSED _alia_scope(         \
                (ctx),                                                        \
                _alia_if_block.anchor,                                        \
                ::alia::get_memoized_block_spec<struct ALIA_UNIQUE_ID(        \
                    alia_if_block_spec_memo_tag_)>());                        \
            ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_ELSE_IF(condition) ALIA_ELSE_IF_ (ctx, condition)

#define ALIA_ELSE_(ctx)                                                       \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    }                                                                         \
    }                                                                         \
    {                                                                         \
        ::alia::if_block _alia_if_block((ctx), _alia_else_condition);         \
        if (_alia_else_condition)                                             \
        {                                                                     \
            ::alia::scoped_conditional_block ALIA_UNUSED _alia_scope(         \
                (ctx),                                                        \
                _alia_if_block.anchor,                                        \
                ::alia::get_memoized_block_spec<struct ALIA_UNIQUE_ID(        \
                    alia_if_block_spec_memo_tag_)>());                        \
            ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_ELSE ALIA_ELSE_ (ctx)

#define ALIA_END                                                              \
    }                                                                         \
    }                                                                         \
    }

#ifndef ALIA_STRICT_MACROS
#define alia_if_(ctx, condition) ALIA_IF_ (ctx, condition)
#define alia_if(condition) ALIA_IF (condition)
#define alia_else_if_(ctx, condition) ALIA_ELSE_IF_ (ctx, condition)
#define alia_else_if(condition) ALIA_ELSE_IF (condition)
#define alia_else_(ctx) ALIA_ELSE_ (ctx)
#define alia_else ALIA_ELSE
#define alia_end ALIA_END
#endif

} // namespace alia
