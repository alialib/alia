#include "demo.hpp"

void
initialize(
    html::system& sys,
    std::string const& placeholder_id,
    std::function<void(html::context)> const& controller)
{
    auto rootless_controller = [=](html::context ctx) {
        placeholder_root(
            ctx, placeholder_id.c_str(), [&]() { controller(ctx); });
    };
    initialize(sys, rootless_controller);
}

void
colored_box(html::context ctx, readable<rgb8> color)
{
    element(ctx, "div")
        .attr("class", "colored-box")
        .attr(
            "style",
            printf(
                ctx,
                "background-color: #%02x%02x%02x",
                alia_field(color, r),
                alia_field(color, g),
                alia_field(color, b)));
}
