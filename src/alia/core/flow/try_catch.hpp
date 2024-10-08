#ifndef ALIA_CORE_FLOW_TRY_CATCH_HPP
#define ALIA_CORE_FLOW_TRY_CATCH_HPP

#include <alia/core/flow/events.hpp>
#include <alia/core/flow/macros.hpp>

namespace alia {

template<class CallOperator>
struct call_operator_arg_type
{
};
template<class Class, class Return, class Arg>
struct call_operator_arg_type<Return (Class::*)(Arg) const>
{
    typedef Arg type;
};

template<class Lambda>
struct lambda_arg_type : call_operator_arg_type<decltype(&Lambda::operator())>
{
};

template<class Clause, class = std::void_t<>>
struct is_concrete_clause : std::false_type
{
};
template<class Clause>
struct is_concrete_clause<
    Clause,
    std::void_t<typename lambda_arg_type<Clause>::type>> : std::true_type
{
};

template<class Context, class Lambda>
std::enable_if_t<is_concrete_clause<std::decay_t<Lambda>>::value, bool>
invoke_catch_clause(
    Context ctx,
    std::exception_ptr exception,
    data_block& block,
    Lambda&& concrete)
{
    try
    {
        std::rethrow_exception(exception);
    }
    catch (typename lambda_arg_type<std::decay_t<Lambda>>::type arg)
    {
        scoped_data_block scoped_block(ctx, block);
        concrete(arg);
        return true;
    }
    catch (...)
    {
        return false;
    }
};

template<class Context, class Lambda>
std::enable_if_t<!is_concrete_clause<std::decay_t<Lambda>>::value, bool>
invoke_catch_clause(
    Context ctx,
    std::exception_ptr exception,
    data_block& block,
    Lambda&& lambda)
{
    scoped_data_block scoped_block(ctx, block);
    lambda();
    return true;
}

struct try_block_data
{
    counter_type last_refresh = 0;
    std::exception_ptr exception;
    bool uncaught = false;
};

struct try_block
{
    try_block(core_context ctx);

    void
    operator<<(function_view<void()> body);

    ~try_block();

    core_context ctx_;
    try_block_data* data_;
    uncaught_exception_detector exception_detector_;
    bool uncaught_;
};

struct catch_block
{
    catch_block(try_block& tb) : try_block_(&tb)
    {
    }

    template<class Body>
    void
    operator<<(Body&& body)
    {
        auto ctx = try_block_->ctx_;
        data_block_node* node;
        get_data_node(ctx, &node);

        // If we're trying to handle an uncaught exception, try invoking this
        // clause.
        if (try_block_->uncaught_)
        {
            if (invoke_catch_clause(
                    ctx, try_block_->data_->exception, node->block, body))
            {
                // This clause handled it, so mark it as caught and return.
                try_block_->uncaught_ = false;
                return;
            }
        }

        // We'll get here for one of three reasons:
        //
        // 1 - There was no exception.
        // 2 - There was an exception but this clause couldn't handle it.
        // 3 - We just tried to refresh the TRY clause and it threw an
        //     exception, so we marked the component as dirty and we're going
        //     to revisit this whole block.
        //
        // In cases 1 and 2, we want to clear out the cache of this block, but
        // we have to make sure not to do so in case 3.
        //
        if (!(*get_event_traversal(ctx).active_container)->dirty
            && get_data_traversal(ctx).cache_clearing_enabled)
        {
            clear_data_block_cache(node->block);
        }
    }

    try_block* try_block_;
};

#define ALIA_TRY_(ctx)                                                        \
    ALIA_DISABLE_MACRO_WARNINGS                                               \
    {                                                                         \
        {                                                                     \
            alia::try_block _alia_try_block(ctx);                             \
            {                                                                 \
                _alia_try_block << [&]() ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_TRY ALIA_TRY_(ctx)

#define ALIA_CATCH_(ctx, pattern)                                             \
    ALIA_DISABLE_MACRO_WARNINGS;                                              \
    }                                                                         \
    {                                                                         \
        alia::catch_block _alia_catch_block(_alia_try_block);                 \
        _alia_catch_block << [&](pattern) ALIA_REENABLE_MACRO_WARNINGS

#define ALIA_CATCH(pattern) ALIA_CATCH_(ctx, pattern)

#ifndef ALIA_STRICT_MACROS
#define alia_try_(ctx) ALIA_TRY_(ctx)
#define alia_try ALIA_TRY
#define alia_catch_(ctx, pattern) ALIA_CATCH_(ctx, pattern)
#define alia_catch(pattern) ALIA_CATCH(pattern)
#endif

} // namespace alia

#endif
