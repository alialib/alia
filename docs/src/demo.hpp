#ifndef DEMO_HPP
#define DEMO_HPP

#include <alia.hpp>

#include <alia/html/dom.hpp>
#include <alia/html/elements.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>

#include "color.hpp"

using namespace alia;

extern std::map<std::string, std::function<void(std::string dom_id)>>
    the_demos;

struct demo : noncopyable
{
    demo(std::string name, std::function<void(std::string dom_id)> f)
    {
        the_demos[name] = f;
    }
};

void
initialize(
    html::system& sys,
    std::string const& placeholder_id,
    std::function<void(html::context)> const& controller);

void
colored_box(html::context ctx, readable<rgb8> color);

inline void
colored_box(html::context ctx, rgb8 const& color)
{
    colored_box(ctx, value(color));
}

#endif
