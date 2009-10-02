#ifndef ALIA_MENU_CONTEXT_HPP
#define ALIA_MENU_CONTEXT_HPP

#include <alia/accessor.hpp>
#include <string>

namespace alia {

class menu_context
{
 public:
    virtual void begin_menu(std::string const& text, bool enabled = true) = 0;
    virtual void end_menu() = 0;
    virtual bool do_option(std::string const& text, bool enabled = true) = 0;
    virtual bool do_checkable_option(accessor<bool> const& value,
        std::string const& text, bool enabled = true) = 0;
    virtual void do_separator() = 0;
};

}

#endif
