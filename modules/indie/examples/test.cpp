#include <alia/indie/backends/glfw.hpp>
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
        paint.setColor(SK_ColorBLACK);
        canvas.drawPaint(paint);
    }
};

int
main(void)
{
    indie::glfw_window the_window(
        "alia test",
        make_vector<unsigned>(1200, 1600),
        [](indie::context ctx) {
            get<indie::system_tag>(ctx).render_root.reset(new my_render_node);
        });
    the_window.do_main_loop();
}
