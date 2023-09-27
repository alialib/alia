#include <alia/indie/backends/glfw.hpp>
#include <alia/indie/widget.hpp>

using namespace alia;

void
my_ui(indie::context ctx);

int
main(void)
{
    indie::glfw_window the_window(
        "alia test", indie::make_vector<unsigned>(1200, 1600), my_ui);
    the_window.do_main_loop();
}
