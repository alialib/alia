#include <alia/backends/glfw.hpp>
#include <alia/ui/widget.hpp>

using namespace alia;

void
my_ui(ui_context ctx);

int
main(void)
{
    glfw_window the_window(
        "alia test", make_vector<unsigned>(1200, 1600), my_ui);
    the_window.do_main_loop();
}
