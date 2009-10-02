#ifndef ALIA_MENU_SELECTION_DISPATCHER_HPP
#define ALIA_MENU_SELECTION_DISPATCHER_HPP

#include <alia/menu/context.hpp>

namespace alia {

class menu_selection_dispatcher : public menu_context
{
 public:
    menu_selection_dispatcher(unsigned selected_id);

    // menu_context implementation...
    void begin_menu(std::string const& text, bool enabled);
    void end_menu();
    bool do_option(std::string const& text, bool enabled);
    bool do_checkable_option(accessor<bool> const& value,
        std::string const& text, bool enabled);
    void do_separator();

 private:
    unsigned selected_id_, id_count_;
};

}

#endif
