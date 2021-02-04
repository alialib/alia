#ifndef ALIA_HTML_BOOTSTRAP_BUTTONS_HPP
#define ALIA_HTML_BOOTSTRAP_BUTTONS_HPP

#include <alia/html/bootstrap/tooltips.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

struct button_handle : regular_element_handle<button_handle>
{
    using regular_element_handle::regular_element_handle;

    template<class Text>
    button_handle&
    tooltip(Text&& text)
    {
        bootstrap::tooltip(*this, std::forward<Text>(text));
        return *this;
    }

    template<class Text, class Placement>
    button_handle&
    tooltip(Text&& text, Placement&& placement)
    {
        bootstrap::tooltip(
            *this,
            std::forward<Text>(text),
            std::forward<Placement>(placement));
        return *this;
    }
};

template<class Style, class... Args>
button_handle
button(html::context ctx, Style&& style, Args&&... args)
{
    return button_handle(html::button(ctx, std::forward<Args>(args)...)
                             .class_("btn")
                             .class_(std::forward<Style>(style)));
}

template<class... Args>
button_handle
primary_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-primary", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
secondary_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-secondary", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
success_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-success", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
danger_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-danger", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
warning_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-warning", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
info_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-info", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
light_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-light", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
dark_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-dark", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
link_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-link", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_primary_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-primary", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_secondary_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-secondary", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_success_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-success", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_danger_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-danger", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_warning_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-warning", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_info_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-info", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_light_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-light", std::forward<Args>(args)...);
}

template<class... Args>
button_handle
outline_dark_button(html::context ctx, Args&&... args)
{
    return button(ctx, "btn-outline-dark", std::forward<Args>(args)...);
}

}}} // namespace alia::html::bootstrap

#endif
