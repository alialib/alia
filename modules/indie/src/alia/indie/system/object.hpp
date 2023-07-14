#ifndef ALIA_INDIE_SYSTEM_OBJECT_HPP
#define ALIA_INDIE_SYSTEM_OBJECT_HPP

#include <alia/core/system/internals.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/rendering.hpp>
#include <alia/indie/system/defines.hpp>

namespace alia { namespace indie {

struct system
{
    std::function<void(indie::context)> controller;

    void
    operator()(alia::context ctx);

    alia::system alia_system;

    std::unique_ptr<render_node> render_root;
};

void
initialize(
    indie::system& system, std::function<void(indie::context)> controller);

}} // namespace alia::indie

#endif
