#ifndef ALIA_FLOW_CONDITIONALS_HPP
#define ALIA_FLOW_CONDITIONALS_HPP

#include <alia/flow/macros.hpp>
#include <type_traits>

namespace alia {

template<class Context, class ComponentCallback>
std::enable_if_t<is_invocable<ComponentCallback>::value>
invoke_component_callback(Context, ComponentCallback&& callback)
{
    callback();
}

template<class Context, class ComponentCallback>
std::enable_if_t<is_invocable<ComponentCallback, Context>::value>
invoke_component_callback(Context ctx, ComponentCallback&& callback)
{
    callback(ctx);
}

namespace detail {

template<class Context>
struct if_stub
{
    template<class Condition, class Body>
    if_stub
    else_if_(Condition condition, Body&& body);

    template<class Body>
    void
    else_(Body&& body);

    Context ctx_;
    bool else_condition_;
};

template<class Context, class Condition, class Body>
if_stub<Context>
if_(Context ctx, bool inherited_condition, Condition condition, Body&& body)
{
    bool else_condition;

    ALIA_IF(inherited_condition && condition)
    {
        invoke_component_callback(ctx, body);
    }
    // Hacking the macros...
    // clang-format off
    }
    else_condition = inherited_condition && _alia_else_condition;
    {
    // clang-format on
    ALIA_END

    return if_stub<Context>{ctx, else_condition};
}

template<class Context>
template<class Condition, class Body>
if_stub<Context>
if_stub<Context>::else_if_(Condition condition, Body&& body)
{
    return if_(this->ctx_, this->else_condition_, condition, body);
}

template<class Context>
template<class Body>
void
if_stub<Context>::else_(Body&& body)
{
    if_(this->ctx_, this->else_condition_, true, body);
}

} // namespace detail

template<class Context, class Condition, class Body>
detail::if_stub<Context>
if_(Context ctx, Condition condition, Body&& body)
{
    return detail::if_(ctx, true, condition, body);
}

} // namespace alia

#endif
