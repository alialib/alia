#ifndef ALIA_UI_SYSTEM_WINDOW_INTERFACE_HPP
#define ALIA_UI_SYSTEM_WINDOW_INTERFACE_HPP

#include <alia/ui/geometry.hpp>
#include <alia/ui/system/input_constants.hpp>

namespace alia {

struct window_interface
{
    virtual ~window_interface()
    {
    }

    virtual void
    set_mouse_cursor(mouse_cursor cursor)
        = 0;
};

} // namespace alia

#endif
