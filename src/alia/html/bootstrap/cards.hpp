#ifndef ALIA_HTML_BOOTSTRAP_CARDS_HPP
#define ALIA_HTML_BOOTSTRAP_CARDS_HPP

#include <alia/html/elements.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

struct internal_card_handle : regular_element_handle<internal_card_handle>
{
    using regular_element_handle::regular_element_handle;

    // Do a header with custom content.
    // This wraps the content in a 'card-header' div.
    template<class Content>
    html::element_handle
    header(Content&& content)
    {
        return div(this->context(), "card-header", [&] {
            std::forward<Content>(content)();
        });
    }

    // Do a body with custom content.
    // This wraps the content in a 'card-body' div.
    template<class Content>
    html::element_handle
    body(Content&& content)
    {
        return div(this->context(), "card-body", [&] {
            std::forward<Content>(content)();
        });
    }

    // Do a footer with custom content.
    // This wraps the content in a 'card-footer' div.
    template<class Content>
    html::element_handle
    footer(Content&& content)
    {
        return div(this->context(), "card-footer", [&] {
            std::forward<Content>(content)();
        });
    }

    // Do a card title.
    // This applies the 'card-title' class to the title text.
    // :element_type should be a heading element tag (e.g, "h5").
    template<class Text>
    html::element_handle
    title(char const* element_type, Text&& text)
    {
        return element(this->context(), element_type)
            .class_("card-title")
            .text(std::forward<Text>(text));
    }

    // Do card text.
    // This wraps the text in a <p> element with the 'card-text' class applied
    // to it.
    template<class Text>
    html::element_handle
    text(Text&& text)
    {
        return p(this->context())
            .class_("card-text")
            .text(std::forward<Text>(text));
    }

    // Do a card link.
    // This applies 'card-link' class to the link.
    template<class... Args>
    auto
    link(Args&&... args)
    {
        return html::link(this->context(), std::forward<Args>(args)...)
            .class_("card-link");
    }
};

// Define a card.
html::element_handle
card(
    html::context ctx,
    alia::function_view<void(internal_card_handle&)> content);

}}} // namespace alia::html::bootstrap

#endif
