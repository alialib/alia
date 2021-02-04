#ifndef ALIA_HTML_BOOTSTRAP_DROPDOWNS_HPP
#define ALIA_HTML_BOOTSTRAP_DROPDOWNS_HPP

#include <alia/html/bootstrap/buttons.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

struct internal_dropdown_handle
{
    html::context ctx;
    element_handle& menu;

    void
    align_right()
    {
        menu.class_("dropdown-menu-right");
    }

    template<class Label>
    element_handle
    option(Label label, action<> on_click)
    {
        return link(ctx, label, on_click).class_("dropdown-item");
    }

    element_handle
    divider()
    {
        return div(ctx, "dropdown-divider");
    }

    template<class Text>
    element_handle
    heading(Text text)
    {
        return h6(ctx).class_("dropdown-header").text(text);
    }
};

typedef element_handle dropdown_handle;

namespace detail {

dropdown_handle
dropdown_button(
    html::context ctx,
    readable<std::string> style,
    function_view<void()> const& label,
    function_view<void(internal_dropdown_handle&)> const& content);

}

template<class Style>
dropdown_handle
dropdown_button(
    html::context ctx,
    Style&& style,
    readable<std::string> label,
    function_view<void(internal_dropdown_handle&)> const& content)
{
    return detail::dropdown_button(
        ctx, signalize(style), [&] { text_node(ctx, label); }, content);
}

template<class Style>
dropdown_handle
dropdown_button(
    html::context ctx,
    Style&& style,
    char const* label,
    function_view<void(internal_dropdown_handle&)> const& content)
{
    return detail::dropdown_button(
        ctx, signalize(style), [&] { text_node(ctx, label); }, content);
}

template<class Style>
dropdown_handle
dropdown_button(
    html::context ctx,
    Style&& style,
    function_view<void()> const& label,
    function_view<void(internal_dropdown_handle&)> const& content)
{
    return detail::dropdown_button(ctx, signalize(style), label, content);
}

}}} // namespace alia::html::bootstrap

#endif
