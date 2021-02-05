#ifndef ALIA_HTML_WIDGETS_HPP
#define ALIA_HTML_WIDGETS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html {

// hr
inline element_handle
hr(html::context ctx)
{
    return element(ctx, "hr");
}

// TEXT

// paragraph element by itself
inline element_handle
p(html::context ctx)
{
    return element(ctx, "p");
}
// paragraph w/text
template<class Text>
std::enable_if_t<!std::is_invocable_v<Text>, element_handle>
p(html::context ctx, Text text)
{
    return element(ctx, "p").text(text);
}
// paragraph as a container
template<class Content>
std::enable_if_t<std::is_invocable_v<Content&&>, element_handle>
p(html::context ctx, Content&& content)
{
    return element(ctx, "p").children(std::forward<Content>(content));
}

#define ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(name)                         \
    inline element_handle name(html::context ctx)                             \
    {                                                                         \
        return element(ctx, #name);                                           \
    }                                                                         \
    template<class Text>                                                      \
    element_handle name(html::context ctx, Text text)                         \
    {                                                                         \
        return element(ctx, #name).text(text);                                \
    }

ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(code)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(pre)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h1)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h2)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h3)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h4)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h5)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(h6)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(b)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(strong)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(i)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(em)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(mark)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(small)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(del)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(ins)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(sub)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(sup)

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
