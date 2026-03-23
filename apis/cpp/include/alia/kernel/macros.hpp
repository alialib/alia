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

struct if_else_chain_scope
{
    alia_substrate_block* block;
    bool else_eligible;
    int branch_counter;
    int* active_branch;

    template<class Condition>
    bool
    eval(Condition&& condition)
    {
        ++branch_counter;
        bool current_condition_is_true
            = else_eligible
           && condition_is_strictly_true(std::forward<Condition>(condition));
        else_eligible
            = else_eligible
           && condition_is_strictly_false(std::forward<Condition>(condition));
        if (current_condition_is_true && *active_branch != branch_counter)
        {
            alia_substrate_reset_block(block);
            *active_branch = branch_counter;
        }
        return current_condition_is_true;
    }
};

static inline if_else_chain_scope
get_if_else_chain_scope(alia_context* ctx)
{
    // TODO: Merge block and branch storage into a single allocation.
    alia_substrate_block* block = alia_substrate_use_block(ctx);
    // TODO: Use C++ interface for this.
    auto branch_storage
        = alia_substrate_use_memory(ctx, sizeof(int), alignof(int));
    int* active_branch = static_cast<int*>(branch_storage.ptr);
    if (branch_storage.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
        *active_branch = -1;
    return {
        block,
        true,
        0,
        active_branch,
    };
}

// TODO: Support traversing the block twice when discovery is required.

struct inner_if_scope
{
    alia_context* ctx_;
    alia_struct_spec* spec_;
    int pass_number_;

    inner_if_scope(
        alia_context* ctx, alia_substrate_block* block, alia_struct_spec* spec)
    {
        ctx_ = ctx;
        spec_ = spec;
        pass_number_ = 0;
        alia_substrate_begin_block(ctx, block, spec);
    }

    void
    advance()
    {
        // TODO: Address thread-safety issues here.
        *spec_ = alia_substrate_end_block(ctx_);
        ++pass_number_;
    }

    bool
    done() const
    {
        return pass_number_ >= 1;
    }
};

// The following are macros used to annotate control flow.

// Note that all come in two forms. One form ends in an underscore and takes
// the context as its first argument. The other form has no trailing underscore
// and assumes that the context is a variable named 'ctx'.

#define ALIA_INVOKE_CONDITIONAL_BLOCK(ctx, chain)                             \
    for (auto _alia_inner_if = alia::inner_if_scope(                          \
             (ctx),                                                           \
             chain.block,                                                     \
             get_memoized_block_spec<struct ALIA_UNIQUE_ID(                   \
                 alia_if_block_spec_memo_tag_)>());                           \
         !_alia_inner_if.done();                                              \
         _alia_inner_if.advance())

#define ALIA_IF_(ctx, condition)                                              \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    if (auto _alia_chain_scope = alia::get_if_else_chain_scope(ctx);          \
        _alia_chain_scope.eval(condition))                                    \
        ALIA_INVOKE_CONDITIONAL_BLOCK((ctx), _alia_chain_scope)               \
    ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_IF(condition) ALIA_IF_(ctx, condition)

#define ALIA_ELSE_IF_(ctx, condition)                                         \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    if (auto _alia_chain_scope = alia::get_if_else_chain_scope(ctx);          \
        _alia_chain_scope.eval(condition))                                    \
        ALIA_INVOKE_CONDITIONAL_BLOCK((ctx), _alia_chain_scope)               \
    ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_ELSE_IF(condition) ALIA_ELSE_IF_(ctx, condition)

#define ALIA_ELSE_(ctx)                                                       \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    else if (_alia_chain_scope.eval(true)) ALIA_INVOKE_CONDITIONAL_BLOCK(     \
        (ctx), _alia_chain_scope) ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_ELSE ALIA_ELSE_(ctx)

#ifndef ALIA_STRICT_MACROS
#define alia_if_(ctx, condition) ALIA_IF_(ctx, condition)
#define alia_if(condition) ALIA_IF (condition)
#define alia_else_if_(ctx, condition) ALIA_ELSE_IF_(ctx, condition)
#define alia_else_if(condition) ALIA_ELSE_IF (condition)
#define alia_else_(ctx) ALIA_ELSE(ctx)
#define alia_else ALIA_ELSE
#endif

} // namespace alia
