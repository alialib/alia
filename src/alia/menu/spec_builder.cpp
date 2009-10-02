#include <alia/menu/spec_builder.hpp>
#include <alia/exception.hpp>

namespace alia {

menu_spec_builder::menu_spec_builder(menu_spec* spec)
{
    stack_.push_back(&spec->items);
    next_id_ = 0;
}

void menu_spec_builder::begin_menu(std::string const& text, bool enabled)
{
    menu_spec::item item;
    item.type = menu_spec::MENU;
    item.id = next_id_++;
    item.text = text;
    item.enabled = enabled;
    stack_.back()->push_back(item);
    stack_.push_back(&stack_.back()->back().children);
}
void menu_spec_builder::end_menu()
{
    stack_.pop_back();
    if (stack_.empty())
        throw exception("invalid menu specification");
}
bool menu_spec_builder::do_option(std::string const& text, bool enabled)
{
    menu_spec::item item;
    item.type = menu_spec::OPTION;
    item.id = next_id_++;
    item.text = text;
    item.enabled = enabled;
    stack_.back()->push_back(item);
    return false;
}
bool menu_spec_builder::do_checkable_option(accessor<bool> const& value,
    std::string const& text, bool enabled)
{
    menu_spec::item item;
    item.type = menu_spec::CHECKABLE_OPTION;
    item.id = next_id_++;
    item.text = text;
    item.enabled = enabled;
    item.checked = value.get();
    stack_.back()->push_back(item);
    return false;
}
void menu_spec_builder::do_separator()
{
    menu_spec::item item;
    item.type = menu_spec::SEPARATOR;
    item.id = next_id_++;
    stack_.back()->push_back(item);
}

}
