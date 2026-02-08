#ifndef ALIA_UI_SYSTEM_WINDOW_INTERFACE_HPP
#define ALIA_UI_SYSTEM_WINDOW_INTERFACE_HPP

#include <alia/abi/base/geometry.h>
#include <alia/input/constants.hpp>

namespace alia {

struct window_interface
{
    virtual ~window_interface()
    {
    }

    virtual void
    set_cursor(cursor cursor)
        = 0;
};

} // namespace alia

#endif
