#include <alia/wx/menu.hpp>
#include <alia/wx/wx.hpp>

namespace alia { namespace wx {

void build_wx_menu(wxMenu* wx_menu,
    std::vector<menu_spec::item> const& menu)
{
    for (std::vector<menu_spec::item>::const_iterator
        i = menu.begin(); i != menu.end(); ++i)
    {
        switch (i->type)
        {
         case menu_spec::MENU:
          {
            wxMenu* submenu = new wxMenu;
            build_wx_menu(submenu, i->children);

            wx_menu->Append(i->id, i->text, submenu);
            if (!i->enabled)
                wx_menu->Enable(i->id, false);

            break;
          }
         case menu_spec::OPTION:
          {
            wx_menu->Append(i->id, i->text.c_str());
            if (!i->enabled)
                wx_menu->Enable(i->id, false);
            break;
          }
         case menu_spec::CHECKABLE_OPTION:
          {
            wx_menu->AppendCheckItem(i->id, i->text.c_str());
            wx_menu->Check(i->id, i->checked);
            if (!i->enabled)
                wx_menu->Enable(i->id, false);
            break;
          }
         case menu_spec::SEPARATOR:
          {
            wx_menu->AppendSeparator();
            break;
          }
        }
    }
}

wxMenuBar* build_wx_menu_bar(menu_spec const& spec)
{
    wxMenuBar* bar = new wxMenuBar;

    for (std::vector<menu_spec::item>::const_iterator
        i = spec.items.begin(); i != spec.items.end(); ++i)
    {
        assert(i->type == menu_spec::MENU);
        wxMenu* wx_menu = new wxMenu;
        build_wx_menu(wx_menu, i->children);
        bar->Append(wx_menu, i->text.c_str());
    }

    return bar;
}

void fix_wx_menu_bar(wxMenuBar* bar, menu_spec const& spec)
{
    int n = 0;
    for (std::vector<menu_spec::item>::const_iterator
        i = spec.items.begin(); i != spec.items.end(); ++i, ++n)
    {
        if (!i->enabled)
            bar->EnableTop(n, false);
    }
}

}}
