#ifndef ALIA_HTML_BOOTSTRAP_HPP
#define ALIA_HTML_BOOTSTRAP_HPP

#include <alia/html/widgets.hpp>

namespace alia {
namespace html {
namespace bootstrap {

template<class... Args>
element_handle<html::context>
button(Args&&... args)
{
    return html::button(std::forward<Args>(args)...).class_("btn");
}

template<class... Args>
element_handle<html::context>
primary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-primary");
}

namespace detail {
element_handle<html::context>
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label);
}
template<class Label>
element_handle<html::context>
checkbox(html::context ctx, duplex<bool> value, Label label)
{
    return detail::checkbox(ctx, value, signalize(label));
}

} // namespace bootstrap
} // namespace html
} // namespace alia

#endif
