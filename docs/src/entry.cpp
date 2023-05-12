#include "demo.hpp"

using namespace alia;

std::map<std::string, std::function<void(std::string dom_id)>> the_demos;

extern "C" {

bool
init_demo(char const* name)
{
    auto demo = the_demos.find(name);
    if (demo == the_demos.end())
        return false;
    demo->second(name);
    return true;
}
}
