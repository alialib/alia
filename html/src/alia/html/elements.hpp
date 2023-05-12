#ifndef ALIA_HTML_ELEMENTS_HPP
#define ALIA_HTML_ELEMENTS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html {

// CONTAINERS

#define ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(name)                    \
    inline element_handle name(context ctx)                                   \
    {                                                                         \
        return element(ctx, #name);                                           \
    }                                                                         \
    template<class Classes>                                                   \
    inline std::enable_if_t<!std::is_invocable_v<Classes>, element_handle>    \
    name(context ctx, Classes classes)                                        \
    {                                                                         \
        return name(ctx).attr("class", classes);                              \
    }                                                                         \
    template<class Content>                                                   \
    std::enable_if_t<std::is_invocable_v<Content>, element_handle> name(      \
        context ctx, Content&& content)                                       \
    {                                                                         \
        return name(ctx).content(std::forward<Content>(content));             \
    }                                                                         \
    template<class Classes, class Content>                                    \
    element_handle name(context ctx, Classes classes, Content&& content)      \
    {                                                                         \
        return name(ctx, classes).content(std::forward<Content>(content));    \
    }

ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(header)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(footer)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(div)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(span)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(nav)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(article)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(section)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(aside)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(ul)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(ol)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(li)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(table)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(thead)
ALIA_HTML_DEFINE_CONTAINER_ELEMENT_INTERFACE(tr)

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

// LEAF ELEMENTS

#define ALIA_HTML_DEFINE_LEAF_ELEMENT_INTERFACE(name)                         \
    inline element_handle name(context ctx)                                   \
    {                                                                         \
        return element(ctx, #name);                                           \
    }                                                                         \
    template<class Classes>                                                   \
    inline element_handle name(context ctx, Classes classes)                  \
    {                                                                         \
        return element(ctx, #name).attr("class", classes);                    \
    }

ALIA_HTML_DEFINE_LEAF_ELEMENT_INTERFACE(a)
ALIA_HTML_DEFINE_LEAF_ELEMENT_INTERFACE(hr)

// TEXT ELEMENTS

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
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(label)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(th)
ALIA_HTML_DEFINE_TEXT_ELEMENT_INTERFACE(td)

// <p> (the paragraph element) gets special treatment because it commonly
// serves as both a container for elements and directly as a text element...
// <p> by itself
inline element_handle
p(html::context ctx)
{
    return element(ctx, "p");
}
// <p> w/text
template<class Text>
std::enable_if_t<!std::is_invocable_v<Text>, element_handle>
p(html::context ctx, Text text)
{
    return element(ctx, "p").text(text);
}
// <p> as a container
template<class Content>
std::enable_if_t<std::is_invocable_v<Content&&>, element_handle>
p(html::context ctx, Content&& content)
{
    return element(ctx, "p").content(std::forward<Content>(content));
}

}} // namespace alia::html

#endif
