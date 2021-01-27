#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html {

// TEXT

// paragraphs
template<class Signal>
element_handle
p(html::context ctx, Signal signal)
{
    return html::element(ctx, "p").text(signal);
}

// INPUTS

struct input_handle : regular_element_handle<input_handle>
{
    using regular_element_handle::regular_element_handle;

    // Define an action to be performed when the Enter key is pressed on the
    // input.
    void
    on_enter(action<> on_enter);
};

namespace detail {
input_handle
input(html::context ctx, duplex<std::string> value);
}
template<class Signal>
input_handle
input(html::context ctx, Signal signal)
{
    return detail::input(ctx, as_duplex_text(ctx, signal));
}

// BUTTONS

// basic button container - You're expected to specify the content.
element_handle
button(html::context ctx, action<> on_click);

// button with text label (as a signal)
element_handle
button(html::context ctx, readable<std::string> text, action<> on_click);

// button with text label (as a string literal)
element_handle
button(html::context ctx, char const* text, action<> on_click);

// LINKS

namespace detail {
element_handle
link(html::context ctx, readable<std::string> text, action<> on_click);
}

// link with a custom action
template<class Text>
element_handle
link(html::context ctx, Text text, action<> on_click)
{
    return detail::link(ctx, signalize(text), on_click);
}

// DIVS

// div with a class - You supply the content (if any).
element_handle
div(context ctx, char const* class_name);

// div with a class and content
template<class Children>
element_handle
div(context ctx, char const* class_name, Children&& children)
{
    return div(ctx, class_name).children(std::forward<Children>(children));
}

// div as an RAII container
struct scoped_div : scoped_element_base<scoped_div>
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
        this->scoped_element_base::end();
    }

    scoped_div&
    begin(html::context ctx, char const* class_name)
    {
        this->scoped_element_base::begin(ctx, "div").attr("class", class_name);
        return *this;
    }
};

}} // namespace alia::html

#endif
