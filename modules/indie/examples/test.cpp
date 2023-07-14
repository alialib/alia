#include <alia/indie/backends/glfw.hpp>
#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/rendering.hpp>
#include <alia/indie/system/object.hpp>

#include <GLFW/glfw3.h>

using namespace alia;

struct my_render_node : indie::render_node
{
    virtual void
    render(SkCanvas& canvas)
    {
        SkPaint paint;
        paint.setColor(SK_ColorMAGENTA);
        auto const& region = this->layout.assignment().region;
        SkRect rect;
        rect.fLeft = SkScalar(region.corner[0]);
        rect.fTop = SkScalar(region.corner[1]);
        rect.fRight = SkScalar(region.corner[0] + region.size[0]);
        rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
        canvas.drawRect(rect, paint);
    }

    alia::layout_leaf layout;
};

int
main(void)
{
    indie::glfw_window the_window(
        "alia test",
        make_vector<unsigned>(1200, 1600),
        [](indie::context ctx) {
            if (is_refresh_event(ctx))
            {
                static my_render_node my_node;
                add_render_node(
                    get<indie::render_traversal_tag>(ctx), &my_node);
                my_node.layout.refresh_layout(
                    get<indie::layout_traversal_tag>(ctx),
                    alia::layout(TOP | LEFT),
                    alia::leaf_layout_requirements(
                        alia::make_layout_vector(100, 100), 0, 0));
                add_layout_node(
                    get<indie::layout_traversal_tag>(ctx), &my_node.layout);
            }
        });
    the_window.do_main_loop();
}
