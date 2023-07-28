#ifndef ALIA_INDIE_SYSTEM_WINDOW_INTERFACE_HPP
#define ALIA_INDIE_SYSTEM_WINDOW_INTERFACE_HPP

#include <alia/indie/geometry.hpp>
#include <alia/indie/system/input_constants.hpp>

namespace alia { namespace indie {

struct window_interface
{
    virtual void
    set_mouse_cursor(mouse_cursor cursor)
        = 0;
};

}} // namespace alia::indie

#endif
