#include <alia/wx/artist.hpp>
#include <alia/wx/wx.hpp>

namespace alia { namespace wx {

#if 0

artist::artist(context& context)
  : generic_artist(context)
{}

color get_wx_color(wxSystemColour index)
{
    wxColour c = wxSystemSettings::GetColour(index);
    return color(c.Red() / 255.f, c.Green() / 255.f, c.Blue() / 255.f);
}

inline color invert_color(color const& c)
{
    return color(1 - c[0], 1 - c[1], 1 - c[2]);
}

void artist::initialize_style_info()
{
    //color highlight = get_wx_color(wxSYS_COLOUR_HIGHLIGHT);
    //color window = get_wx_color(wxSYS_COLOUR_WINDOW);
    //color text = get_wx_color(wxSYS_COLOUR_WINDOWTEXT);
    //color highlight_text = get_wx_color(wxSYS_COLOUR_HIGHLIGHTTEXT);

    //{
    //generic_style_info* gs = get_generic_style_info(NORMAL_STYLE);
    //style_info* s = get_style_info(NORMAL_STYLE);
    //gs->panel_color = window;
    //gs->disabled_link_button_color = color(0.65, 0.65, 0.65);
    //gs->link_button_color = color(0.2, 0.3, 0.7);
    //gs->highlighted_link_button_color = color(0.2, 0.3, 0.7);
    //gs->depressed_link_button_color = color(color(0.2, 0.3, 0.7) * 0.7f);
    //gs->normal_button_bg_color = highlight;
    //gs->highlighted_button_bg_color =
    //    color(highlight * 0.80f + window * 0.20f);
    //gs->depressed_button_bg_color = color(highlight * 0.60f);
    //gs->disabled_button_bg_color = color(light_gray * .90f);
    //s->static_text_color = text;
    //s->option_bg_color = window;
    //s->option_fg_color = text;
    //s->focused_option_bg_color = text;
    //s->focused_option_fg_color = window;
    //s->selected_option_bg_color = highlight;
    //s->selected_option_fg_color = highlight_text;
    //autocomplete_style_info(s);
    //}
    //{
    //generic_style_info* gs = get_generic_style_info(HIGHLIGHTED_STYLE);
    //style_info* s = get_style_info(HIGHLIGHTED_STYLE);
    //// TODO
    //*gs = *get_generic_style_info(NORMAL_STYLE);
    //*s = *get_style_info(NORMAL_STYLE);
    //}
    //{
    //generic_style_info* gs = get_generic_style_info(INSET_STYLE);
    //style_info* s = get_style_info(INSET_STYLE);
    //*gs = *get_generic_style_info(NORMAL_STYLE);
    //*s = *get_style_info(NORMAL_STYLE);
    //}
    //{
    //generic_style_info* gs = get_generic_style_info(CONTENT_STYLE);
    //style_info* s = get_style_info(CONTENT_STYLE);
    //// TODO
    //*gs = *get_generic_style_info(NORMAL_STYLE);
    //*s = *get_style_info(NORMAL_STYLE);
    //}
    //{
    //generic_style_info* gs = get_generic_style_info(SECTION_HEADER_STYLE);
    //style_info* s = get_style_info(SECTION_HEADER_STYLE);
    //gs->panel_color = highlight;
    //gs->disabled_link_button_color = color(0.65, 0.65, 0.65);
    //gs->link_button_color = highlight_text;
    //gs->highlighted_link_button_color = highlight_text;
    //gs->depressed_link_button_color = color(highlight_text * 0.85f +
    //    highlight * 0.15f);
    //// TODO: button, bg
    //s->static_text_color = highlight_text;
    //s->option_bg_color = highlight;
    //s->option_fg_color = highlight_text;
    //s->focused_option_bg_color = highlight_text;
    //s->focused_option_fg_color = highlight;
    //s->selected_option_bg_color = highlight_text;
    //s->selected_option_fg_color = highlight;
    //autocomplete_style_info(s);
    //}
    //{
    //generic_style_info* gs = get_generic_style_info(SECTION_CONTENT_STYLE);
    //style_info* s = get_style_info(SECTION_CONTENT_STYLE);
    //// TODO
    //*gs = *get_generic_style_info(NORMAL_STYLE);
    //*s = *get_style_info(NORMAL_STYLE);
    //autocomplete_style_info(s);
    //}

    //set_inset_border_color(black);
}

#endif

}}
