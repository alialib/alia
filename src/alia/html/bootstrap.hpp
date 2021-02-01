#ifndef ALIA_HTML_BOOTSTRAP_HPP
#define ALIA_HTML_BOOTSTRAP_HPP

#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

// BREADCRUMB

struct breadcrumb
{
    scoped_element nav;
    scoped_element ol;

    breadcrumb()
    {
    }
    breadcrumb(html::context ctx)
    {
        this->begin(ctx);
    }
    ~breadcrumb()
    {
        this->end();
    }

    breadcrumb&
    begin(html::context ctx)
    {
        this->nav.begin(ctx, "nav").attr("aria-label", "breadcrumb");
        this->ol.begin(ctx, "ol").class_("breadcrumb");
        return *this;
    }
    void
    end()
    {
        this->ol.end();
        this->nav.end();
    }

    template<class Link>
    element_handle
    item(Link&& link)
    {
        return element(this->nav.context(), "li")
            .class_("breadcrumb-item")
            .children([&] { std::forward<Link>(link)(); });
    }

    element_handle
    active_item()
    {
        return element(this->nav.context(), "li")
            .classes("breadcrumb-item active")
            .attr("aria-current", "page");
    }

    template<class Label>
    element_handle
    active_item(Label label)
    {
        return active_item().text(label);
    }
};

// BUTTONS

// button - Note that for this overload you MUST also add a style class (e.g.,
// "btn-primary"). Alternatively, you can use one of the overloads below.
template<class... Args>
element_handle
button(Args&&... args)
{
    return html::button(std::forward<Args>(args)...).class_("btn");
}

template<class... Args>
element_handle
primary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-primary");
}

template<class... Args>
element_handle
secondary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-secondary");
}

template<class... Args>
element_handle
success_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-success");
}

template<class... Args>
element_handle
danger_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-danger");
}

template<class... Args>
element_handle
warning_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-warning");
}

template<class... Args>
element_handle
info_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-info");
}

template<class... Args>
element_handle
light_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-light");
}

template<class... Args>
element_handle
dark_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-dark");
}

template<class... Args>
element_handle
link_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-link");
}

template<class... Args>
element_handle
outline_primary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-primary");
}

template<class... Args>
element_handle
outline_secondary_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-secondary");
}

template<class... Args>
element_handle
outline_success_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-success");
}

template<class... Args>
element_handle
outline_danger_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-danger");
}

template<class... Args>
element_handle
outline_warning_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-warning");
}

template<class... Args>
element_handle
outline_info_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-info");
}

template<class... Args>
element_handle
outline_light_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-light");
}

template<class... Args>
element_handle
outline_dark_button(Args&&... args)
{
    return button(std::forward<Args>(args)...).class_("btn-outline-dark");
}

// CHECKBOXES

namespace detail {
element_handle
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label);
}
template<class Label>
element_handle
checkbox(html::context ctx, duplex<bool> value, Label label)
{
    return detail::checkbox(ctx, value, signalize(label));
}

// MISCELLANEOUS

// Do the 'x' that serves as a close button.
html::element_handle
close_button(html::context ctx);

// MODALS

struct internal_modal_handle : regular_element_handle<input_handle>
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

struct modal_handle : regular_element_handle<input_handle>
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
    alia::function_view<void(internal_modal_handle)> content);

}}} // namespace alia::html::bootstrap

#endif
