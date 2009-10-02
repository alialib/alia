#ifndef ALIA_MENU_SPEC_BUILDER_HPP
#define ALIA_MENU_SPEC_BUILDER_HPP

#include <alia/menu/context.hpp>
#include <alia/menu/specification.hpp>
#include <list>
#include <vector>

namespace alia {

class menu_spec_builder : public menu_context
{
 public:
    menu_spec_builder(menu_spec* spec);

    // menu_context implementation...
    void begin_menu(std::string const& text, bool enabled);
    void end_menu();
    bool do_option(std::string const& text, bool enabled);
    bool do_checkable_option(accessor<bool> const& value,
        std::string const& text, bool enabled);
    void do_separator();

 private:
    std::list<std::vector<menu_spec::item>*> stack_;
    unsigned next_id_;
};

}

#endif
