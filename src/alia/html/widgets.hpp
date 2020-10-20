#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia {
namespace html {

template<class Signal>
void
text(html::context ctx, Signal signal)
{
    html::element(ctx, "p").text(signal);
}

namespace detail {
element_handle<html::context>
input(html::context ctx, duplex<string> value);
}
template<class Signal>
element_handle<html::context>
input(html::context ctx, Signal signal)
{
    return detail::input(ctx, as_duplex_text(ctx, signal));
}

namespace detail {
void
button(html::context ctx, readable<std::string> text, action<> on_click);
}
template<class Text>
void
button(html::context ctx, Text text, action<> on_click)
{
    detail::button(ctx, signalize(text), on_click);
}

namespace detail {
void
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label);
}
template<class Label>
void
checkbox(html::context ctx, duplex<bool> value, Label label)
{
    detail::checkbox(ctx, value, signalize(label));
}

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

template<class Context, class ClassName>
element_handle<Context>
div(Context ctx, ClassName class_name)
{
    return element(ctx, "div").attr("class", signalize(class_name));
}

template<class Context, class ClassName, class Children>
element_handle<Context>
div(Context ctx, ClassName class_name, Children&& children)
{
    return div(ctx, class_name).children(std::forward<Children>(children));
}

struct scoped_div : scoped_element
{
    scoped_div()
    {
    }
    scoped_div(html::context ctx, readable<std::string> class_name)
    {
        begin(ctx, class_name);
    }
    ~scoped_div()
    {
        end();
    }

    scoped_div&
    begin(html::context ctx, readable<std::string> class_name)
    {
        this->scoped_element::begin(ctx, "div").attr("class", class_name);
        return *this;
    }
};

} // namespace html
} // namespace alia

#endif
