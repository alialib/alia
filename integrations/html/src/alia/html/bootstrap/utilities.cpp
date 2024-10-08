#include <alia/html/bootstrap/utilities.hpp>

namespace alia { namespace html { namespace bootstrap {

html::element_handle
close_button(html::context ctx)
{
    return element(ctx, "button")
        .attr("type", "button")
        .class_("close")
        .attr("aria-label", "Close")
        .content([&] {
            element(ctx, "span").attr("aria-hidden", "true").text("\u00D7");
        });
}

}}} // namespace alia::html::bootstrap
