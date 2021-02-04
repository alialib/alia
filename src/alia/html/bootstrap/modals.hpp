#ifndef ALIA_HTML_BOOTSTRAP_MODALS_HPP
#define ALIA_HTML_BOOTSTRAP_MODALS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html { namespace bootstrap {

struct internal_modal_handle : regular_element_handle<internal_modal_handle>
{
    using regular_element_handle::regular_element_handle;

    // Do the 'x' in the top right corner of the modal that serves as a close
    // button.
    html::element_handle
    close_button();

    // Do a standard modal header with a title and a close button.
    template<class Title>
    html::element_handle
    title(Title title)
    {
        return div(this->context(), "modal-header", [&] {
            element(this->context(), "h5").class_("modal-title").text(title);
            this->close_button();
        });
    }

    // Do a header with custom content.
    // This wraps the content in a 'modal-header' div and supplies a close
    // button.
    template<class Content>
    html::element_handle
    header(Content&& content)
    {
        return div(this->context(), "modal-header", [&] {
            std::forward<Content>(content)();
            this->close_button();
        });
    }

    // Do a body with custom content.
    // This wraps the content in a 'modal-body' div.
    template<class Content>
    html::element_handle
    body(Content&& content)
    {
        return div(this->context(), "modal-body", [&] {
            std::forward<Content>(content)();
        });
    }

    // Do a modal footer with custom content.
    // This wraps the content in a 'modal-footer' div.
    template<class Content>
    html::element_handle
    footer(Content&& content)
    {
        return div(this->context(), "modal-footer", [&] {
            std::forward<Content>(content)();
        });
    }

    // Close this modal.
    void
    close();

    // Get an action that closes this modal.
    auto
    close_action()
    {
        return alia::callback([this] { this->close(); });
    }
};

struct modal_data
{
    tree_node<element_object> root;
    bool active = false;
};

struct modal_handle : regular_element_handle<modal_handle>
{
    modal_handle(element_handle& element, modal_data& data)
        : regular_element_handle(element), data(data)
    {
    }

    // Activate the modal.
    void
    activate();

    // Get an action that activates this modal.
    auto
    activate_action()
    {
        return alia::callback([this] { this->activate(); });
    }

    modal_data& data;
};

// Define a modal.
modal_handle
modal(
    html::context ctx,
    alia::function_view<void(internal_modal_handle&)> content);

}}} // namespace alia::html::bootstrap

#endif
