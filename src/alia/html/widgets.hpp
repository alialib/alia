#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia {
namespace html {

// TEXT

// paragraphs
template<class Signal>
element_handle<html::context>
p(html::context ctx, Signal signal)
{
    return html::element(ctx, "p").text(signal);
}

// INPUTS

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

// BUTTONS

// basic button container - You're expected to specify the content.
element_handle<html::context>
button(html::context ctx, action<> on_click);

// button with text label (as a signal)
element_handle<html::context>
button(html::context ctx, readable<std::string> text, action<> on_click);

// button with text label (as a string literal)
element_handle<html::context>
button(html::context ctx, char const* text, action<> on_click);

// LINKS

namespace detail {
element_handle<html::context>
link(html::context ctx, readable<std::string> text, action<> on_click);
}

// link with a custom action
template<class Text>
element_handle<html::context>
link(html::context ctx, Text text, action<> on_click)
{
    return detail::link(ctx, signalize(text), on_click);
}

// DIVS

// div with a class - You supply the content (if any).
template<class Context>
element_handle<Context>
div(Context ctx, char const* class_name)
{
    return element(ctx, "div").attr("class", class_name);
}

// div with a class and content
template<class Context, class Children>
element_handle<Context>
div(Context ctx, char const* class_name, Children&& children)
{
    return div(ctx, class_name).children(std::forward<Children>(children));
}

// div as an RAII container
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
        this->scoped_element::end();
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
