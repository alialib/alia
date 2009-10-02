#include <alia/menu/selection_dispatcher.hpp>

namespace alia {

menu_selection_dispatcher::menu_selection_dispatcher(unsigned selected_id)
{
    selected_id_ = selected_id;
    id_count_ = 0;
}

void menu_selection_dispatcher::begin_menu(
    std::string const& text, bool enabled)
{
    ++id_count_;
}
void menu_selection_dispatcher::end_menu()
{
}
bool menu_selection_dispatcher::do_option(
    std::string const& text, bool enabled)
{
    if (id_count_++ == selected_id_)
        return true;
    return false;
}
bool menu_selection_dispatcher::do_checkable_option(
    accessor<bool> const& value,
    std::string const& text, bool enabled)
{
    if (id_count_++ == selected_id_)
    {
        value.set(!value.get());
        return true;
    }
    return false;
}
void menu_selection_dispatcher::do_separator()
{
    ++id_count_;
}

}
