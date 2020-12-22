#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia {
namespace html {

template<class Signal>
element_handle<html::context>
text(html::context ctx, Signal signal)
{
    return html::element(ctx, "p").text(signal);
}

namespace detail {
element_handle<html::context>
input(html::context ctx, duplex<std::string> value);
}
template<class Signal>
element_handle<html::context>
input(html::context ctx, Signal signal)
{
    return detail::input(ctx, as_duplex_text(ctx, signal));
}

element_handle<html::context>
button(html::context ctx, action<> on_click);

element_handle<html::context>
button(html::context ctx, readable<std::string> text, action<> on_click);

element_handle<html::context>
button(html::context ctx, char const* text, action<> on_click);

namespace detail {
element_handle<html::context>
link(html::context ctx, readable<std::string> text, action<> on_click);
}
template<class Text>
element_handle<html::context>
link(html::context ctx, Text text, action<> on_click)
{
    return detail::link(ctx, signalize(text), on_click);
}

template<class Context>
element_handle<Context>
div(Context ctx, char const* class_name)
{
    return element(ctx, "div").attr("class", class_name);
}

template<class Context, class Children>
element_handle<Context>
div(Context ctx, char const* class_name, Children&& children)
{
    return div(ctx, class_name).children(std::forward<Children>(children));
}

struct scoped_div : scoped_element
{
    scoped_div()
    {
    }
    scoped_div(html::context ctx, char const* class_name)
    {
        begin(ctx, class_name);
    }
    ~scoped_div()
    {
        end();
    }

    scoped_div&
    begin(html::context ctx, char const* class_name)
    {
        this->scoped_element::begin(ctx, "div").attr("class", class_name);
        return *this;
    }
};

} // namespace html
} // namespace alia

#endif
