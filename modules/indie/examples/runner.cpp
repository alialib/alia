#include <alia/indie/backends/glfw.hpp>
#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/rendering.hpp>
#include <alia/indie/system/object.hpp>

#include <GLFW/glfw3.h>

using namespace alia;

void
my_ui(indie::context ctx);

int
main(void)
{
    indie::glfw_window the_window(
        "alia test", make_vector<unsigned>(1200, 1600), my_ui);
    the_window.do_main_loop();
}
