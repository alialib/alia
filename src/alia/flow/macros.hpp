#ifndef ALIA_FLOW_MACROS_HPP
#define ALIA_FLOW_MACROS_HPP

#include <alia/flow/data_graph.hpp>

namespace alia {

// The following are utilities that are used to implement the control flow
// macros. They shouldn't be used directly by applications.

struct if_block : noncopyable
{
    if_block(data_traversal& traversal, bool condition);

 private:
    scoped_data_block scoped_data_block_;
};

struct switch_block : noncopyable
{
    template<class Context>
    switch_block(Context& ctx)
    {
        nc_.begin(ctx);
    }
    template<class Id>
    void
    activate_case(Id id)
    {
        active_case_.end();
        active_case_.begin(nc_, make_id(id), manual_delete(true));
    }

 private:
    naming_context nc_;
    named_block active_case_;
};

struct loop_block : noncopyable
{
    loop_block(data_traversal& traversal);
    ~loop_block();
    data_block&
    block() const
    {
        return *block_;
    }
    data_traversal&
    traversal() const
    {
        return *traversal_;
    }
    void
    next();

 private:
    data_traversal* traversal_;
    data_block* block_;
};

// The following are macros used to annotate control flow.
// They are used exactly like their C equivalents, but all require an alia_end
// after the end of their scope.
// Also note that all come in two forms. One form ends in an underscore and
// takes the context as its first argument. The other form has no trailing
// underscore and assumes that the context is a variable named 'ctx'.

// condition_is_true(x), where x is a signal whose value is testable in a
// boolean context, returns true iff x is readable and its value is true.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_true(Signal const& x)
{
    return signal_is_readable(x) && (read_signal(x) ? true : false);
}

// condition_is_false(x), where x is a signal whose value is testable in a
// boolean context, returns true iff x is readable and its value is false.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_false(Signal const& x)
{
    return signal_is_readable(x) && (read_signal(x) ? false : true);
}

// condition_is_readable(x), where x is a readable signal type, calls
// signal_is_readable.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, bool>
condition_is_readable(Signal const& x)
{
    return signal_is_readable(x);
}

// read_condition(x), where x is a readable signal type, calls read_signal.
template<class Signal>
std::enable_if_t<
    is_readable_signal_type<Signal>::value,
    typename Signal::value_type>
read_condition(Signal const& x)
{
    return read_signal(x);
}

// ALIA_STRICT_CONDITIONALS disables the definitions that allow non-signals to
// be used in if/else/switch macros.
#ifndef ALIA_STRICT_CONDITIONALS

// condition_is_true(x) evaluates x in a boolean context.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_true(T x)
{
    return x ? true : false;
}

// condition_is_false(x) evaluates x in a boolean context and inverts it.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_false(T x)
{
    return x ? false : true;
}

// condition_is_readable(x), where x is NOT a signal type, always returns true.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, bool>
condition_is_readable(T const& x)
{
    return true;
}

// read_condition(x), where x is NOT a signal type, simply returns x.
template<class T>
std::enable_if_t<!is_signal_type<T>::value, T const&>
read_condition(T const& x)
{
    return x;
}

#endif

// if, else_if, else

#define ALIA_IF_(ctx, condition)                                               \
    {                                                                          \
        bool _alia_else_condition ALIA_UNUSED;                                 \
        {                                                                      \
            auto const& _alia_condition = (condition);                         \
            bool _alia_if_condition                                            \
                = ::alia::condition_is_true(_alia_condition);                  \
            _alia_else_condition                                               \
                = ::alia::condition_is_false(_alia_condition);                 \
            ::alia::if_block _alia_if_block(                                   \
                get_data_traversal(ctx), _alia_if_condition);                  \
            if (_alia_if_condition)                                            \
            {

#define ALIA_IF(condition) ALIA_IF_(ctx, condition)

#define ALIA_ELSE_IF_(ctx, condition)                                          \
    }                                                                          \
    }                                                                          \
    {                                                                          \
        auto const& _alia_condition = (condition);                             \
        bool _alia_else_if_condition                                           \
            = _alia_else_condition                                             \
              && ::alia::condition_is_true(_alia_condition);                   \
        _alia_else_condition = _alia_else_condition                            \
                               && ::alia::condition_is_false(_alia_condition); \
        ::alia::if_block _alia_if_block(                                       \
            get_data_traversal(ctx), _alia_else_if_condition);                 \
        if (_alia_else_if_condition)                                           \
        {

#define ALIA_ELSE_IF(condition) ALIA_ELSE_IF_(ctx, condition)

#define ALIA_ELSE_(ctx)                                                        \
    }                                                                          \
    }                                                                          \
    {                                                                          \
        ::alia::if_block _alia_if_block(                                       \
            get_data_traversal(ctx), _alia_else_condition);                    \
        if (_alia_else_condition)                                              \
        {

#define ALIA_ELSE ALIA_ELSE_(ctx)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_if_(ctx, condition) ALIA_IF_(ctx, condition)
#define alia_if(condition) ALIA_IF(condition)
#define alia_else_if_(ctx, condition) ALIA_ELSE_IF_(ctx, condition)
#define alia_else_if(condition) ALIA_ELSE_IF(condition)
#define alia_else_(ctx) ALIA_ELSE(ctx)
#define alia_else ALIA_ELSE
#endif

// switch

#define ALIA_SWITCH_(ctx, x)                                                   \
    {                                                                          \
        ::alia::switch_block _alia_switch_block(ctx);                          \
        if (::alia::condition_is_readable(x))                                  \
        {                                                                      \
            switch (::alia::read_condition(x))                                 \
            {

#define ALIA_SWITCH(x) ALIA_SWITCH_(ctx, x)

#define ALIA_CONCATENATE_HELPER(a, b) a##b
#define ALIA_CONCATENATE(a, b) ALIA_CONCATENATE_HELPER(a, b)

#define ALIA_CASE_(ctx, c)                                                     \
    case c:                                                                    \
        _alia_switch_block.activate_case(c);                                   \
        goto ALIA_CONCATENATE(_alia_dummy_label_, __LINE__);                   \
        ALIA_CONCATENATE(_alia_dummy_label_, __LINE__)

#define ALIA_CASE(c) ALIA_CASE_(ctx, c)

#define ALIA_DEFAULT_(ctx)                                                     \
    default:                                                                   \
        _alia_switch_block.activate_case("_alia_default_case");                \
        goto ALIA_CONCATENATE(_alia_dummy_label_, __LINE__);                   \
        ALIA_CONCATENATE(_alia_dummy_label_, __LINE__)

#define ALIA_DEFAULT ALIA_DEFAULT_(ctx)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_switch_(ctx, x) ALIA_SWITCH_(ctx, x)
#define alia_switch(x) ALIA_SWITCH(x)
#define alia_case_(ctx, c) ALIA_CASE(ctx, c)
#define alia_case(c) ALIA_CASE(c)
#define alia_default_(ctx) ALIA_DEFAULT_(ctx)
#define alia_default ALIA_DEFAULT
#endif

// for

#define ALIA_FOR_(ctx, x)                                                      \
    {                                                                          \
        {                                                                      \
            ::alia::loop_block _alia_looper(get_data_traversal(ctx));          \
            for (x)                                                            \
            {                                                                  \
                ::alia::scoped_data_block _alia_scope;                         \
                _alia_scope.begin(                                             \
                    _alia_looper.traversal(), _alia_looper.block());           \
                _alia_looper.next();

#define ALIA_FOR(x) ALIA_FOR_(ctx, x)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_for_(ctx, x) ALIA_FOR_(ctx, x)
#define alia_for(x) ALIA_FOR(x)
#endif

// while

#define ALIA_WHILE_(ctx, x)                                                    \
    {                                                                          \
        {                                                                      \
            ::alia::loop_block _alia_looper(get_data_traversal(ctx));          \
            while (x)                                                          \
            {                                                                  \
                ::alia::scoped_data_block _alia_scope;                         \
                _alia_scope.begin(                                             \
                    _alia_looper.traversal(), _alia_looper.block());           \
                _alia_looper.next();

#define ALIA_WHILE(x) ALIA_WHILE_(ctx, x)

#ifdef ALIA_LOWERCASE_MACROS
#define alia_while_(ctx, x) ALIA_WHILE_(ctx, x)
#define alia_while(x) ALIA_WHILE(x)
#endif

// end

#define ALIA_END                                                               \
    }                                                                          \
    }                                                                          \
    }

#ifdef ALIA_LOWERCASE_MACROS
#define alia_end ALIA_END
#endif

// The following macros are substitutes for normal C++ control flow statements.
// Unlike alia_if and company, they do NOT track control flow. Instead, they
// convert your context variable to a dataless_context within the block.
// This means that any attempt to retrieve data within the block will result
// in an error (as it should).

#define ALIA_REMOVE_DATA_TRACKING(ctx)                                         \
    decltype(ctx)::storage_type _alia_storage;                                 \
    auto _alia_ctx                                                             \
        = alia::remove_component<data_traversal_tag>(&_alia_storage, ctx);     \
    auto ctx = _alia_ctx;

#define ALIA_UNTRACKED_IF_(ctx, condition)                                     \
    if (alia::condition_is_true(condition))                                    \
    {                                                                          \
        ALIA_REMOVE_DATA_TRACKING(ctx)                                         \
        {                                                                      \
            {

#define ALIA_UNTRACKED_IF(condition) ALIA_UNTRACKED_IF_(ctx, condition)

#define ALIA_UNTRACKED_ELSE_IF_(ctx, condition)                                \
    }                                                                          \
    }                                                                          \
    }                                                                          \
    else if (alia::condition_is_true(condition))                               \
    {                                                                          \
        ALIA_REMOVE_DATA_TRACKING(ctx)                                         \
        {                                                                      \
            {

#define ALIA_UNTRACKED_ELSE_IF(condition)                                      \
    ALIA_UNTRACKED_ELSE_IF_(ctx, condition)

#define ALIA_UNTRACKED_ELSE_(ctx)                                              \
    }                                                                          \
    }                                                                          \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        ALIA_REMOVE_DATA_TRACKING(ctx)                                         \
        {                                                                      \
            {

#define ALIA_UNTRACKED_ELSE ALIA_UNTRACKED_ELSE_(ctx)

#define ALIA_UNTRACKED_SWITCH_(ctx, expression)                                \
    switch (expression)                                                        \
    {                                                                          \
        {                                                                      \
            {

#define ALIA_UNTRACKED_SWITCH(expression)                                      \
    ALIA_UNTRACKED_SWITCH_(ctx, expression)

#define ALIA_UNTRACKED_CASE_(ctx, c)                                           \
    }                                                                          \
    }                                                                          \
    }                                                                          \
    case c:                                                                    \
    {                                                                          \
        {                                                                      \
            {                                                                  \
                ALIA_REMOVE_DATA_TRACKING(ctx)                                 \
                goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__);           \
                ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

#define ALIA_UNTRACKED_CASE(c) ALIA_UNTRACKED_CASE_(ctx, c)

#define ALIA_UNTRACKED_DEFAULT_(ctx)                                           \
    }                                                                          \
    }                                                                          \
    }                                                                          \
    default:                                                                   \
    {                                                                          \
        {                                                                      \
            {                                                                  \
                ALIA_REMOVE_DATA_TRACKING(ctx)                                 \
                goto ALIA_CONCATENATE(alia__dummy_label_, __LINE__);           \
                ALIA_CONCATENATE(alia__dummy_label_, __LINE__)

#define ALIA_UNTRACKED_DEFAULT ALIA_UNTRACKED_DEFAULT_(ctx)

#ifdef ALIA_LOWERCASE_MACROS

#define alia_untracked_if_(ctx, condition) ALIA_UNTRACKED_IF_(ctx, condition)
#define alia_untracked_if(condition) ALIA_UNTRACKED_IF(condition)
#define alia_untracked_else_if_(ctx, condition)                                \
    ALIA_UNTRACKED_ELSE_IF_(ctx, condition)
#define alia_untracked_else_if(condition) ALIA_UNTRACKED_ELSE_IF(condition)
#define alia_untracked_else_(ctx) ALIA_UNTRACKED_ELSE(ctx)
#define alia_untracked_else ALIA_UNTRACKED_ELSE

#define alia_untracked_switch_(ctx, x) ALIA_UNTRACKED_SWITCH_(ctx, x)
#define alia_untracked_switch(x) ALIA_UNTRACKED_SWITCH(x)
#define alia_untracked_case_(ctx, c) ALIA_UNTRACKED_CASE_(ctx, c)
#define alia_untracked_case(c) ALIA_UNTRACKED_CASE(c)
#define alia_untracked_default_(ctx) ALIA_UNTRACKED_DEFAULT_(ctx)
#define alia_untracked_default ALIA_UNTRACKED_DEFAULT

#endif

} // namespace alia

#endif
