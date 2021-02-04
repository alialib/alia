#include <alia/html/bootstrap/dropdowns.hpp>

namespace alia { namespace html { namespace bootstrap { namespace detail {

dropdown_handle
dropdown_button(
    html::context ctx,
    readable<std::string> style,
    function_view<void()> const& label,
    function_view<void(internal_dropdown_handle&)> const& content)
{
    return element(ctx, "btn-group").children([&] {
        element(ctx, "button")
            .attr("type", "button")
            .attr("class", "btn dropdown-toggle")
            .attr("data-toggle", "dropdown")
            .attr("aria-haspopup", "true")
            .attr("aria-expanded", "false")
            .class_(style)
            .children(label);

        auto menu = div(ctx, "dropdown-menu");
        menu.children([&] {
            internal_dropdown_handle handle{ctx, menu};
            content(handle);
        });
    });
}

}}}} // namespace alia::html::bootstrap::detail
