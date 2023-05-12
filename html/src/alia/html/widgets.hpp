#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html {

// INPUTS

struct input_handle : regular_element_handle<input_handle>
{
    using regular_element_handle::regular_element_handle;

    // Add placeholder text.
    template<class Text>
    input_handle&
    placeholder(Text text)
    {
        attr("placeholder", text);
        return *this;
    }

    // Set the autofocus attribute.
    input_handle&
    autofocus()
    {
        attr("autofocus");
        return *this;
    }

    // Define an action to be performed when the Enter key is pressed on the
    // input.
    input_handle&
    on_enter(action<> on_enter);

    // Define an action to be performed when the Escape key is pressed on the
    // input.
    input_handle&
    on_escape(action<> on_escape);
};

inline input_handle
input(html::context ctx)
{
    return input_handle(element(ctx, "input"));
}

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
template<class Text, class Action>
std::enable_if_t<is_action_type<Action>::value, element_handle>
link(html::context ctx, Text text, Action on_click)
{
    return detail::link(ctx, signalize(text), on_click);
}

namespace detail {
element_handle
link(
    html::context ctx, readable<std::string> text, readable<std::string> href);
}

// link with an href
template<class Text, class Href>
std::enable_if_t<!is_action_type<Href>::value, element_handle>
link(html::context ctx, Text text, Href href)
{
    return detail::link(ctx, signalize(text), signalize(href));
}

// CHECKBOX

element_handle
checkbox(html::context ctx, duplex<bool> value);

}} // namespace alia::html

#endif
