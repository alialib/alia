#include <alia/menu/spec_differ.hpp>

namespace alia {

menu_spec_differ::menu_spec_differ(menu_spec const& spec)
{
    iterator root;
    root.menu = &spec.items;
    root.index = 0;
    stack_.push_back(root);
    is_different_ = false;
}

void menu_spec_differ::end()
{
    if (is_different_)
        return;
    if (stack_.size() != 1 ||
        stack_.back().index != stack_.back().menu->size())
    {
        is_different_ = true;
    }
}

void menu_spec_differ::begin_menu(std::string const& text, bool enabled)
{
    if (is_different_)
        return;
    if (stack_.back().index == stack_.back().menu->size())
    {
        is_different_ = true;
        return;
    }
    menu_spec::item const& item = (*stack_.back().menu)[stack_.back().index++];
    if (item.type != menu_spec::MENU ||
        item.text != text ||
        item.enabled != enabled)
    {
        is_different_ = true;
        return;
    }
    iterator i;
    i.menu = &item.children;
    i.index = 0;
    stack_.push_back(i);
}
void menu_spec_differ::end_menu()
{
    if (is_different_)
        return;
    if (stack_.empty())
    {
        is_different_ = true;
        return;
    }
    if (stack_.back().index != stack_.back().menu->size())
        is_different_ = true;
    stack_.pop_back();
}
bool menu_spec_differ::do_option(std::string const& text, bool enabled)
{
    if (is_different_)
        return false;
    if (stack_.back().index == stack_.back().menu->size())
    {
        is_different_ = true;
        return false;
    }
    menu_spec::item const& item = (*stack_.back().menu)[stack_.back().index++];
    if (item.type != menu_spec::OPTION ||
        item.text != text ||
        item.enabled != enabled)
    {
        is_different_ = true;
        return false;
    }
    return false;
}
bool menu_spec_differ::do_checkable_option(accessor<bool> const& value,
    std::string const& text, bool enabled)
{
    if (is_different_)
        return false;
    if (stack_.back().index == stack_.back().menu->size())
    {
        is_different_ = true;
        return false;
    }
    menu_spec::item const& item = (*stack_.back().menu)[stack_.back().index++];
    if (item.type != menu_spec::CHECKABLE_OPTION ||
        item.text != text ||
        item.enabled != enabled ||
        item.checked != value.get())
    {
        is_different_ = true;
        return false;
    }
    return false;
}
void menu_spec_differ::do_separator()
{
    if (is_different_)
        return;
    if (stack_.back().index == stack_.back().menu->size())
    {
        is_different_ = true;
        return;
    }
    menu_spec::item const& item = (*stack_.back().menu)[stack_.back().index++];
    if (item.type != menu_spec::SEPARATOR)
    {
        is_different_ = true;
        return;
    }
}

}
