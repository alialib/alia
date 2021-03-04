#include <alia/html/bootstrap/forms.hpp>
#include <alia/html/elements.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap { namespace detail {

element_handle
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label)
{
    auto checkbox = div(ctx, "form-check");
    auto id = printf(ctx, "alia-id-%i", checkbox.asmdom_id());
    checkbox.content([&] {
        html::checkbox(ctx, value)
            .attr("class", "form-check-input")
            .attr("id", id);
        element(ctx, "label")
            .class_("form-check-label")
            .attr("for", id)
            .text(label);
    });
    return checkbox;
}

}}}} // namespace alia::html::bootstrap::detail
