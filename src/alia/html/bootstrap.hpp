#ifndef ALIA_HTML_BOOTSTRAP_HPP
#define ALIA_HTML_BOOTSTRAP_HPP

#include <alia/html/widgets.hpp>

namespace alia {
namespace html {
namespace bootstrap {

// BUTTONS

// button - Note that for this overload you MUST also add a style class (e.g.,
// "btn-primary"). Alternatively, you can use one of the overloads below.
template<class... Args>
element_handle<html::context>
button(Args&&... args)
{
    return html::button(std::forward<Args>(args)...).class_("btn");
}

// primary button
template<class... Args>
element_handle<html::context>
primary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-primary");
}

// secondary button
template<class... Args>
element_handle<html::context>
secondary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-primary");
}

// CHECKBOXES

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

// COMMON

// Do the 'x' that serves as a close button.
html::element_handle<html::context>
close_button(html::context ctx);

// MODALS

// Do the 'x' in the top right corner of a modal that serves as a close button.
html::element_handle<html::context>
modal_close_button(html::context ctx);

// Do a standard modal header with a title and a close button.
template<class Title>
html::element_handle<html::context>
standard_modal_header(html::context ctx, Title const& title)
{
    return div(ctx, "modal-header", [&] {
        element(ctx, "h5").class_("modal-title").text(title);
        modal_close_button(ctx);
    });
}

// Do a modal header with custom content.
// This wraps the content in a 'modal-header' div and supplies a close button.
template<class Content>
html::element_handle<html::context>
modal_header(html::context ctx, Content&& content)
{
    return div(ctx, "modal-header", [&] {
        invoke_component_function(ctx, std::forward<Content>(content));
        modal_close_button(ctx);
    });
}

// Do a modal body with custom content.
// This wraps the content in a 'modal-body' div.
template<class Content>
html::element_handle<html::context>
modal_body(html::context ctx, Content&& content)
{
    return div(ctx, "modal-body", [&] {
        invoke_component_function(ctx, std::forward<Content>(content));
    });
}

// Do a modal footer with custom content.
// This wraps the content in a 'modal-footer' div.
template<class Content>
html::element_handle<html::context>
modal_footer(html::context ctx, Content&& content)
{
    return div(ctx, "modal-footer", [&] {
        invoke_component_function(ctx, std::forward<Content>(content));
    });
}

// Close the active modal.
void
close_modal();

struct modal_data
{
    tree_node<element_object> root;
    bool active = false;
};

struct modal_handle
{
    void
    activate()
    {
        data.active = true;
        EM_ASM({ jQuery(Module['nodes'][$0]).modal('show'); }, asmdom_id);
    }

    modal_data& data;
    int asmdom_id;
};

// Define a modal.
modal_handle
modal(html::context ctx, alia::function_view<void()> content);

} // namespace bootstrap
} // namespace html
} // namespace alia

#endif
