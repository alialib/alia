#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <alia/indie/common.hpp>

#include <include/core/SkCanvas.h>

namespace alia { namespace indie {

struct render_node
{
    virtual void
    render(SkCanvas& canvas)
        = 0;
};

struct render_container : render_node
{
    std::vector<std::unique_ptr<render_node>> children;
};

struct system;

void
render(indie::system& system, SkCanvas& canvas);

}} // namespace alia::indie

#endif
