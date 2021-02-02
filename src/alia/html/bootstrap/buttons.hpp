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

// button - Note that for this overload you MUST also add a style class (e.g.,
// "btn-primary"). Alternatively, you can use one of the overloads below.
template<class... Args>
button_handle
button(Args&&... args)
{
    return button_handle(
        html::button(std::forward<Args>(args)...).class_("btn"));
}

template<class... Args>
button_handle
primary_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-primary"));
}

template<class... Args>
button_handle
secondary_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-secondary"));
}

template<class... Args>
button_handle
success_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-success"));
}

template<class... Args>
button_handle
danger_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-danger"));
}

template<class... Args>
button_handle
warning_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-warning"));
}

template<class... Args>
button_handle
info_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-info"));
}

template<class... Args>
button_handle
light_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-light"));
}

template<class... Args>
button_handle
dark_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-dark"));
}

template<class... Args>
button_handle
link_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-link"));
}

template<class... Args>
button_handle
outline_primary_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-primary"));
}

template<class... Args>
button_handle
outline_secondary_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-secondary"));
}

template<class... Args>
button_handle
outline_success_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-success"));
}

template<class... Args>
button_handle
outline_danger_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-danger"));
}

template<class... Args>
button_handle
outline_warning_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-warning"));
}

template<class... Args>
button_handle
outline_info_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-info"));
}

template<class... Args>
button_handle
outline_light_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-light"));
}

template<class... Args>
button_handle
outline_dark_button(Args&&... args)
{
    return button_handle(
        button(std::forward<Args>(args)...).class_("btn-outline-dark"));
}

}}} // namespace alia::html::bootstrap

#endif
