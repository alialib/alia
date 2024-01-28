#ifndef ALIA_UI_SYSTEM_OS_INTERFACE_HPP
#define ALIA_UI_SYSTEM_OS_INTERFACE_HPP

#include <alia/ui/system/input_constants.hpp>

namespace alia {

struct os_interface
{
    virtual void
    set_clipboard_text(std::string text)
        = 0;

    virtual std::optional<std::string>
    get_clipboard_text() = 0;
};

} // namespace alia

#endif
