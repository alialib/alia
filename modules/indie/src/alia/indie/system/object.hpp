#ifndef ALIA_INDIE_SYSTEM_OBJECT_HPP
#define ALIA_INDIE_SYSTEM_OBJECT_HPP

#include <alia/core/system/internals.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/mouse.hpp>
#include <alia/indie/layout/internals.hpp>

namespace alia { namespace indie {

struct widget;

struct system : alia::typed_system<indie::vanilla_context>
{
    std::function<void(indie::context)> controller;
    void invoke_controller(indie::vanilla_context) override;

    layout_system layout;
    widget* root_widget;

    mouse_cursor current_cursor = mouse_cursor::DEFAULT;
};

void
initialize(
    indie::system& system, std::function<void(indie::context)> controller);

}} // namespace alia::indie

#endif
