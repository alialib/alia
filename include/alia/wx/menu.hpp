#ifndef ALIA_WX_MENU_HPP
#define ALIA_WX_MENU_HPP

#include <alia/menu/specification.hpp>

class wxMenu;
class wxMenuBar;

namespace alia { namespace wx {

void build_wx_menu(wxMenu* wx_menu,
    std::vector<menu_spec::item> const& menu);

wxMenuBar* build_wx_menu_bar(menu_spec const& spec);

void fix_wx_menu_bar(wxMenuBar* bar, menu_spec const& spec);

}}

#endif
