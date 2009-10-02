#ifndef ALIA_MENU_SPEC_DIFFER_HPP
#define ALIA_MENU_SPEC_DIFFER_HPP

#include <alia/menu/context.hpp>
#include <alia/menu/specification.hpp>
#include <list>

namespace alia {

class menu_spec_differ : public menu_context
{
 public:
    menu_spec_differ(menu_spec const& spec);

    void end();

    bool is_different() const { return is_different_; }

    // menu_context implementation...
    void begin_menu(std::string const& text, bool enabled);
    void end_menu();
    bool do_option(std::string const& text, bool enabled);
    bool do_checkable_option(accessor<bool> const& value,
        std::string const& text, bool enabled);
    void do_separator();

 private:
    bool is_different_;
    struct iterator
    {
        std::vector<menu_spec::item> const* menu;
        unsigned index;
    };
    std::list<iterator> stack_;
};

}

#endif
