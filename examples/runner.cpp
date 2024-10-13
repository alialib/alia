#include <alia/ui/backends/glfw.hpp>
// #include <alia/ui/backends/sdl.hpp>
// #include <alia/ui/backends/emscripten.hpp>

#include <alia/ui/context.hpp>

using namespace alia;

void
my_ui(ui_context ctx);

int
main(void)
{
    // sdl_window the_window(
    //     "alia test", make_vector<unsigned>(900, 1200), my_ui);
    // the_window.do_main_loop();

    glfw_window the_window(
        "alia test", make_vector<unsigned>(1200, 1600), my_ui);
    the_window.do_main_loop();

    // emscripten_canvas the_canvas(my_ui);
    // the_canvas.do_main_loop();
}
