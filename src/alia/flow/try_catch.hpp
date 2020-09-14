#ifndef ALIA_FLOW_TRY_CATCH_HPP
#define ALIA_FLOW_TRY_CATCH_HPP

#include <alia/flow/events.hpp>
#include <alia/flow/macros.hpp>

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

template<class Clause, class = void_t<>>
struct is_concrete_clause : std::false_type
{
};
template<class Clause>
struct is_concrete_clause<
    Clause,
    void_t<typename lambda_arg_type<Clause>::type>> : std::true_type
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
    try_block(context ctx);

    void
    operator<<(function_view<void()> body);

    ~try_block();

    context ctx_;
    try_block_data* data_;
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
        data_block* block;
        get_data_node(ctx, &block);
        if (try_block_->uncaught_)
        {
            if (invoke_catch_clause(
                    ctx, try_block_->data_->exception, *block, body))
            {
                try_block_->uncaught_ = false;
                return;
            }
        }
        if (get_data_traversal(ctx).cache_clearing_enabled)
        {
            block->clear_cache();
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
                _alia_try_block << [&]() ALIA_UNDISABLE_MACRO_WARNINGS

#define ALIA_TRY ALIA_TRY_(ctx)

#define ALIA_CATCH_(ctx, pattern)                                             \
    ALIA_DISABLE_MACRO_WARNINGS;                                              \
    }                                                                         \
    {                                                                         \
        alia::catch_block _alia_catch_block(_alia_try_block);                 \
        _alia_catch_block << [&](pattern) ALIA_UNDISABLE_MACRO_WARNINGS

#define ALIA_CATCH(pattern) ALIA_CATCH_(ctx, pattern)

#ifndef ALIA_STRICT_MACROS
#define alia_try_(ctx) ALIA_TRY_(ctx)
#define alia_try ALIA_TRY
#define alia_catch_(ctx, pattern) ALIA_CATCH_(ctx, pattern)
#define alia_catch(pattern) ALIA_CATCH(pattern)
#endif

} // namespace alia

#endif
