#ifndef DEMO_HPP
#define DEMO_HPP

#include "alia.hpp"

#include "dom.hpp"

using namespace alia;

extern std::map<std::string, std::function<void(std::string dom_id)>> the_demos;

struct demo : noncopyable
{
    demo(std::string name, std::function<void(std::string dom_id)> f)
    {
        the_demos[name] = f;
    }
};

#endif
