#include "demo.hpp"

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
