#ifdef WIN32

#include <alia/native/win32/artist.hpp>
#include <alia/native/win32/font.hpp>
#include <alia/native/win32/themed.hpp>
#include <alia/native/win32/classic.hpp>
#include <alia/native/win32/widget_image.hpp>
#include <alia/context.hpp>
#include <alia/surface.hpp>
#include <alia/standard_colors.hpp>
#include <alia/scoped_state.hpp>

#include <alia/native/win32/windows.hpp>
#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")

namespace alia { namespace native {

static inline uint8 channel_multiply(uint8 a, uint8 b)
{
    uint32 x = uint32(a) * uint32(b) + 128;
    return uint8((x + (x >> 8)) >> 8);
}

void overlay_images(image_view<rgba8> const& result,
    image_view<rgba8> const& bottom,
    image_view<rgba8> const& top)
{
    alia_foreach_pixel3(result, rgba8, r, bottom, rgba8, b, top, rgba8, t,
        {
            uint8 ba = b.a - channel_multiply(b.a, t.a);
            r.r = channel_multiply(t.r, t.a) + channel_multiply(b.r, ba);
            r.g = channel_multiply(t.g, t.a) + channel_multiply(b.g, ba);
            r.b = channel_multiply(t.b, t.a) + channel_multiply(b.b, ba);
            r.a = t.a + ba;
        })
}

// TODO: remove
struct stateful_widget
{
    theme_set* themes;
    HTHEME theme;
    int type, part;
    vector2i size;

    stateful_widget_image_data<int,image<rgba8> > image_data;

    void operator()(image<rgba8>* image, int state) const
    {
        if (themes)
        {
            create_themed_widget_image(image, themes, theme, part, state,
                size);
        }
        else
            create_classic_widget_image(image, size, type, part | state);
    }

    void draw(surface& surface, point2i const& position, int state)
    {
        draw_stateful_widget(image_data, surface, position, state, *this);
    }

    void reset()
    {
        invalidate(image_data);
    }
};

static unsigned const NORMAL_STATE = 0;
static unsigned const HOT_STATE = 1;
static unsigned const PRESSED_STATE = 2;
static unsigned const DISABLED_STATE = 3;
static unsigned const N_STATES = 4;

int get_state_index(widget_state state)
{
    if ((state & widget_states::DISABLED) != 0)
    {
        return DISABLED_STATE;
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            return HOT_STATE;
         case widget_states::DEPRESSED:
            return PRESSED_STATE;
         default:
            return NORMAL_STATE;
        }
    }
}

struct fixed_widget_image
{
    image<rgba8> img;
    cached_image_ptr renderer;
};

struct fixed_widget
{
    vector2i size;
    fixed_widget_image imgs[N_STATES];
};

void cache_themed_widget_image(fixed_widget_image& fwi,
    surface& surface, theme_set* themes, HTHEME theme, int part,
    int state, vector2i const& size)
{
    create_themed_widget_image(&fwi.img, themes, theme, part, state, size);
    fwi.renderer.reset();
    surface.cache_image(fwi.renderer, make_interface(fwi.img.view, 0));
}

void cache_classic_widget_image(fixed_widget_image& fwi,
    surface& surface, int type, int part, vector2i const& size)
{
    create_classic_widget_image(&fwi.img, size, type, part);
    fwi.renderer.reset();
    surface.cache_image(fwi.renderer, make_interface(fwi.img.view, 0));
}

void cache_classic_thumb_image(fixed_widget_image& fwi,
    surface& surface, int edge, int flags, vector2i const& size)
{
    create_classic_thumb_image(&fwi.img, size, edge, flags);
    fwi.renderer.reset();
    surface.cache_image(fwi.renderer, make_interface(fwi.img.view, 0));
}

static unsigned const CHECK_BOX_INDEX = 0;
static unsigned const RADIO_BUTTON_INDEX = 2;
static unsigned const SCROLLBAR_BUTTON_INDEX = 4;
static unsigned const SLIDER_THUMB_INDEX = 8;
static unsigned const N_FIXED_WIDGETS = 10;

struct artist::impl_data
{
    // If using visual styles, this stores a pointer to the theme set.
    // Otherwise, if using classic style, this is NULL.
    theme_set* themes;
    unsigned version;
    // cached fonts
    font normal_font, heading_font, fixed_font;
    // images of widgets with fixed sizes are shared and cached here
    fixed_widget fixed_widgets[N_FIXED_WIDGETS];
};

// STRUCTORS, ETC

artist::artist()
{
    impl_ = new impl_data;
    impl_->themes = 0;
    impl_->version = 0;
}

artist::~artist()
{
    delete impl_->themes;
    delete impl_;
}

void artist::initialize()
{
    generic_artist::initialize();
    delete impl_->themes;
    try
    {
        impl_->themes = new theme_set;
    }
    catch (...)
    {
        impl_->themes = 0;
    }
    ++impl_->version;
    initialize_fixed_widgets();
    initialize_fonts();
}

void artist::initialize_fixed_widgets()
{
    for (unsigned i = 0; i < N_FIXED_WIDGETS; ++i)
    {
        for (unsigned j = 0; j < N_STATES; ++j)
            impl_->fixed_widgets[i].imgs[j].renderer.reset();
    }

    theme_set* themes = impl_->themes;
    if (themes)
    {
        HTHEME theme[N_FIXED_WIDGETS] = {
            themes->button, themes->button,
            themes->button, themes->button,
            themes->scrollbar, themes->scrollbar,
            themes->scrollbar, themes->scrollbar,
            themes->trackbar, themes->trackbar
        };
        int part[N_FIXED_WIDGETS] = {
            BP_CHECKBOX, BP_CHECKBOX,
            BP_RADIOBUTTON, BP_RADIOBUTTON,
            SBP_ARROWBTN, SBP_ARROWBTN,
            SBP_ARROWBTN, SBP_ARROWBTN,
            TKP_THUMB, TKP_THUMBVERT
        };
        int state[N_FIXED_WIDGETS] = {
            CBS_UNCHECKEDNORMAL, CBS_CHECKEDNORMAL,
            RBS_UNCHECKEDNORMAL, RBS_CHECKEDNORMAL,
            ABS_LEFTNORMAL, ABS_RIGHTNORMAL,
            ABS_UPNORMAL, ABS_DOWNNORMAL,
            TUS_NORMAL, TUS_NORMAL
        };
        for (unsigned i = 0; i < N_FIXED_WIDGETS; ++i)
        {
            impl_->fixed_widgets[i].size =
                get_themed_widget_size(themes, theme[i], part[i], state[i]);
        }
    }
    else
    {
        for (int i = 0; i < 4; ++i)
            impl_->fixed_widgets[i].size = vector2i(13, 13);

        vector2i hsize, vsize;
        hsize[0] = GetSystemMetrics(SM_CXHSCROLL);
        hsize[1] = GetSystemMetrics(SM_CYHSCROLL);
        vsize[0] = GetSystemMetrics(SM_CXVSCROLL);
        vsize[1] = GetSystemMetrics(SM_CYVSCROLL);
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 0].size = hsize;
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 1].size = hsize;
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 2].size = vsize;
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 3].size = vsize;

        impl_->fixed_widgets[SLIDER_THUMB_INDEX + 0].size = vector2i(12, 20);
        impl_->fixed_widgets[SLIDER_THUMB_INDEX + 1].size = vector2i(20, 12);
    }
}

void artist::initialize_fonts()
{
    NONCLIENTMETRICSW metrics = { 0 };
    metrics.cbSize = sizeof(NONCLIENTMETRICSW);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,
        sizeof(NONCLIENTMETRICSW), &metrics, 0))
    {
        metrics.cbSize -= 4;
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,
            sizeof(NONCLIENTMETRICSW), &metrics, 0))
        {
            // TODO: Should this just initialize some defaults instead?
            // (Or let the generic_artist do the fonts only?)
            // Would this ever actually fail anyway?
            throw exception(
                "SystemParametersInfo(SPI_GETNONCLIENTMETRICS) failed");
        }
    }
    impl_->normal_font = make_font(metrics.lfMessageFont);
    impl_->normal_font.set_size(impl_->normal_font.get_size() *
        get_context().font_scale_factor);
    impl_->heading_font = make_font(metrics.lfCaptionFont);
    impl_->heading_font.set_size(impl_->heading_font.get_size() *
        get_context().font_scale_factor);
    impl_->fixed_font = get_font(OEM_FIXED_FONT, font("courier", 10));
    impl_->fixed_font.set_size(impl_->fixed_font.get_size() *
        get_context().font_scale_factor);
}

font artist::translate_standard_font(standard_font font) const
{
    switch ((get_context().pass_state.style_code >> 4) & 0xf)
    {
     case TITLE_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return impl_->fixed_font;
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                17 * get_context().font_scale_factor, font::BOLD);
            //return impl_->heading_font;
        }
        break;
     case HEADING_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return impl_->fixed_font;
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                15 * get_context().font_scale_factor, font::BOLD);
            //return impl_->heading_font;
        }
        break;
     case SUBHEADING_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return impl_->fixed_font;
         case NORMAL_FONT:
         default:
            return alia::font("georgia",
                13 * get_context().font_scale_factor, font::BOLD);
            //return impl_->heading_font;
        }
        break;
     case HIGHLIGHTED_STYLE_CODE:
     case TEXT_CONTROL_STYLE_CODE:
     case LIST_STYLE_CODE:
        switch (font)
        {
         case FIXED_FONT:
            return impl_->fixed_font;
         case NORMAL_FONT:
         default:
            return alia::font("helvetica",
                13 * get_context().font_scale_factor, 0, 1.1f);
            //return impl_->heading_font;
        }
        break;
     default:
        switch (font)
        {
         case FIXED_FONT:
            return impl_->fixed_font;
         case NORMAL_FONT:
         default:
            return alia::font("helvetica",
                13 * get_context().font_scale_factor, 0, 1.1f);
            //return impl_->normal_font;
        }
        break;
    }
}

void artist::on_system_theme_change()
{
    initialize();
}

void artist::on_font_scale_factor_change()
{
    initialize();
}

// BUTTON

vector2i artist::get_button_size(artist_data_ptr& data,
    vector2i const& content_size) const
{
    return vector2i(
        (std::max)(content_size[0] + 12, 75),
        (std::max)(content_size[1] + 8, 23));
}

vector2i artist::get_button_content_offset(artist_data_ptr& data,
    vector2i const& content_size, widget_state state) const
{
    vector2i offset(6, 4);
    if ((state & widget_states::PRIMARY_STATE_MASK) ==
        widget_states::DEPRESSED)
    {
        offset += vector2i(1, 1);
    }
    vector2i minimum_content_size(63, 15);
    for (int i = 0; i < 2; ++i)
    {
        if (content_size[i] < minimum_content_size[i])
            offset[i] += (minimum_content_size[i] - content_size[i]) / 2;
    }
    return offset;
}

struct button_data : artist_data
{
    unsigned version;
    stateful_widget widget;
};

void artist::draw_button(artist_data_ptr& data_, box2i const& region,
    widget_state state) const
{
    assert(!data_ || dynamic_cast<button_data*>(data_.get()));
    button_data* data = static_cast<button_data*>(data_.get());
    if (!data || data->version != impl_->version ||
        data->widget.size != region.size)
    {
        data = new button_data;
        data_.reset(data);
        data->version = impl_->version;
        data->widget.size = region.size;
        data->widget.themes = impl_->themes;
        if (impl_->themes)
        {
            data->widget.theme = impl_->themes->button;
            data->widget.part = BP_PUSHBUTTON;
        }
        else
        {
            data->widget.type = DFC_BUTTON;
            data->widget.part = DFCS_BUTTONPUSH;
        }
    }

    int native_state;
    if (impl_->themes)
    {
        if ((state & widget_states::DISABLED) != 0)
        {
            native_state = PBS_DISABLED;
        }
        else
        {
            switch (state & widget_states::PRIMARY_STATE_MASK)
            {
             case widget_states::HOT:
                native_state = PBS_HOT;
                break;
             case widget_states::DEPRESSED:
                native_state = PBS_PRESSED;
                break;
             default:
                native_state = (state & widget_states::FOCUSED) ?
                    PBS_DEFAULTED : PBS_NORMAL;
            }
        }
    }
    else
        native_state = get_classic_state(state);

    data->widget.draw(get_surface(), region.corner, native_state);

    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(region, -3));
}

// CHECK BOX

vector2i artist::get_check_box_size(artist_data_ptr& data,
    bool checked) const
{
    return impl_->fixed_widgets[CHECK_BOX_INDEX + (checked ? 1 : 0)].size;
}

void artist::draw_check_box(artist_data_ptr& data, bool checked,
    point2i const& position, widget_state state) const
{
    fixed_widget& w =
        impl_->fixed_widgets[CHECK_BOX_INDEX + (checked ? 1 : 0)];
    int state_index = get_state_index(state);
    fixed_widget_image& img = w.imgs[state_index];
    if (!img.renderer)
    {
        if (impl_->themes)
        {
            cache_themed_widget_image(img, get_surface(), impl_->themes,
                impl_->themes->button, BP_CHECKBOX,
                (checked ? CBS_CHECKEDNORMAL : CBS_UNCHECKEDNORMAL) +
                state_index, w.size);
        }
        else
        {
            cache_classic_widget_image(img, get_surface(), DFC_BUTTON,
                DFCS_BUTTONCHECK |
                get_classic_state(state) | (checked ? DFCS_CHECKED : 0),
                w.size);
        }
    }
    img.renderer->draw(point2d(position)); 
    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(box2i(position, w.size), 1));
}

// RADIO BUTTON

vector2i artist::get_radio_button_size(artist_data_ptr& data,
    bool selected) const
{
    return impl_->fixed_widgets[RADIO_BUTTON_INDEX + (selected ? 1 : 0)].size;
}

void artist::draw_radio_button(artist_data_ptr& data, bool selected,
    point2i const& position, widget_state state) const
{
    fixed_widget& w =
        impl_->fixed_widgets[RADIO_BUTTON_INDEX + (selected ? 1 : 0)];
    int state_index = get_state_index(state);
    fixed_widget_image& img = w.imgs[state_index];
    if (!img.renderer)
    {
        if (impl_->themes)
        {
            cache_themed_widget_image(img, get_surface(), impl_->themes,
                impl_->themes->button, BP_RADIOBUTTON,
                (selected ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL) +
                state_index, w.size);
        }
        else
        {
            cache_classic_widget_image(img, get_surface(), DFC_BUTTON,
                DFCS_BUTTONRADIO | get_classic_state(state) |
                (selected ? DFCS_CHECKED : 0), w.size);
        }
    }
    img.renderer->draw(point2d(position)); 
    if ((state & widget_states::FOCUSED) != 0)
        draw_focus_rect(add_border(box2i(position, w.size), 1));
}

// SCROLLBAR

int artist::get_scrollbar_width() const
{
    return impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 2].size[0];
}
int artist::get_scrollbar_button_length() const
{
    return impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 2].size[1];
}
int artist::get_minimum_scrollbar_thumb_length() const
{
    return 10;
}

struct scrollbar_background_data : artist_data
{
    unsigned version;
    theme_set* themes;
    stateful_widget_image_data<int,image<rgb8> > image_data;
    vector2i size;
    int part;
    void operator()(image<rgb8>* image, int state) const
    {
        if (themes)
        {
            int native_part;
            switch (part)
            {
             case 0:
                native_part = SBP_LOWERTRACKHORZ;
                break;
             case 1:
                native_part = SBP_UPPERTRACKHORZ;
                break;
             case 2:
                native_part = SBP_LOWERTRACKVERT;
                break;
             case 3:
                native_part = SBP_UPPERTRACKVERT;
                break;
            }
            create_themed_widget_image(image, themes, themes->scrollbar,
                native_part, state, size);
        }
        else
            create_classic_scrollbar_background_image(image, size, state);
    }
    void draw(surface& surface, point2i const& position, int state)
    {
        draw_stateful_widget(image_data, surface, position, state, *this);
    }
};

void artist::draw_scrollbar_background(artist_data_ptr& data_,
    box2i const& rect, int axis, int which, widget_state state) const
{
    assert(!data_ || dynamic_cast<scrollbar_background_data*>(data_.get()));
    scrollbar_background_data* data =
        static_cast<scrollbar_background_data*>(data_.get());
    if (!data || data->version != impl_->version || data->size != rect.size ||
        data->part != axis * 2 + which)
    {
        data = new scrollbar_background_data;
        data_.reset(data);
        data->version = impl_->version;
        data->themes = impl_->themes;
        data->part = axis * 2 + which;
        data->size = rect.size;
    }
    int native_state;
    if (impl_->themes)
    {
        if ((state & widget_states::DISABLED) != 0)
        {
            native_state = SCRBS_DISABLED;
        }
        else
        {
            switch (state & widget_states::PRIMARY_STATE_MASK)
            {
             case widget_states::HOT:
                native_state = SCRBS_HOT;
                break;
             case widget_states::DEPRESSED:
                native_state = SCRBS_PRESSED;
                break;
             default:
                native_state = SCRBS_NORMAL;
            }
        }
    }
    else
    {
        bool invert = ((state & widget_states::PRIMARY_STATE_MASK) ==
            widget_states::DEPRESSED);
        bool odd = ((rect.corner[0] ^ rect.corner[1]) & 1) != 0;
        native_state = (invert ? 1 : 0) | (odd ? 2 : 0);
    }
    data->draw(get_surface(), rect.corner, native_state);
}

struct scrollbar_thumb_data : artist_data
{
    unsigned version;
    theme_set* themes;
    stateful_widget_image_data<int,image<rgba8> > image_data;
    vector2i size;
    int axis;
    void operator()(image<rgba8>* img, int state) const
    {
        if (themes)
        {
            create_image(*img, size);
            image<rgba8> bottom(size), top(size);
            create_themed_widget_image(&bottom, themes, themes->scrollbar,
                axis ? SBP_THUMBBTNVERT : SBP_THUMBBTNHORZ, state, size);
            create_themed_widget_image(&top, themes, themes->scrollbar,
                axis ? SBP_GRIPPERVERT : SBP_GRIPPERHORZ, state, size);
            overlay_images(img->view, bottom.view, top.view);
        }
        else
        {
            create_classic_thumb_image(img, size, EDGE_RAISED,
                BF_RECT | BF_MIDDLE);
        }
    }
    void draw(surface& surface, point2i const& position, int state)
    {
        draw_stateful_widget(image_data, surface, position, state, *this);
    }
};

void artist::draw_scrollbar_thumb(artist_data_ptr& data_,
    box2i const& rect, int axis, widget_state state) const
{
    assert(!data_ || dynamic_cast<scrollbar_thumb_data*>(data_.get()));
    scrollbar_thumb_data* data =
        static_cast<scrollbar_thumb_data*>(data_.get());
    if (!data || data->version != impl_->version || data->size != rect.size ||
        data->axis != axis)
    {
        data = new scrollbar_thumb_data;
        data_.reset(data);
        data->version = impl_->version;
        data->themes = impl_->themes;
        data->axis = axis;
        data->size = rect.size;
    }
    int native_state;
    if (impl_->themes)
    {
        if ((state & widget_states::DISABLED) != 0)
        {
            native_state = SCRBS_DISABLED;
        }
        else
        {
            switch (state & widget_states::PRIMARY_STATE_MASK)
            {
             case widget_states::HOT:
                native_state = SCRBS_HOT;
                break;
             case widget_states::DEPRESSED:
                native_state = SCRBS_PRESSED;
                break;
             default:
                native_state = SCRBS_NORMAL;
            }
        }
    }
    else
        native_state = 0;
    data->draw(get_surface(), rect.corner, native_state);
}

// button

void artist::draw_scrollbar_button(artist_data_ptr& data,
    point2i const& position, int axis, int which, widget_state state) const
{
    fixed_widget& w =
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + (axis * 2 + which)];
    int state_index = get_state_index(state);
    fixed_widget_image& img = w.imgs[state_index];
    if (!img.renderer)
    {
        if (impl_->themes)
        {
            int base_state[4] = { ABS_LEFTNORMAL, ABS_RIGHTNORMAL,
                ABS_UPNORMAL, ABS_DOWNNORMAL };
            cache_themed_widget_image(img, get_surface(), impl_->themes,
                impl_->themes->scrollbar, SBP_ARROWBTN,
                base_state[axis * 2 + which] + state_index, w.size);
        }
        else
        {
            int base_state[4] = { DFCS_SCROLLLEFT, DFCS_SCROLLRIGHT,
                DFCS_SCROLLUP, DFCS_SCROLLDOWN };
            cache_classic_widget_image(img, get_surface(), DFC_SCROLL,
                base_state[axis * 2 + which] | get_classic_state(state),
                w.size);
        }
    }
    img.renderer->draw(point2d(position)); 
}

// junction

struct scrollbar_junction_data : artist_data
{
    unsigned version;
    rgb8 color;
};

void artist::draw_scrollbar_junction(artist_data_ptr& data_,
    point2i const& position) const
{
    assert(!data_ || dynamic_cast<scrollbar_junction_data*>(data_.get()));
    scrollbar_junction_data* data =
        static_cast<scrollbar_junction_data*>(data_.get());
    if (!data || data->version != impl_->version)
    {
        data = new scrollbar_junction_data;
        data_.reset(data);
        data->version = impl_->version;
        data->color = get_sys_color(COLOR_BTNFACE);
    }
    box2i rect(position, vector2i(
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 2].size[0],
        impl_->fixed_widgets[SCROLLBAR_BUTTON_INDEX + 2].size[0]));
    point2i poly[4];
    make_polygon(poly, rect);
    get_surface().draw_filled_polygon(data->color, poly, 4);
}

// DROP DOWN LIST

//struct drop_down_button_data : artist_data
//{
//    unsigned version;
//    stateful_widget widget;
//};
//vector2i artist::get_minimum_drop_down_button_size() const
//{
//    return vector2i(16, 16);
//}
//void artist::draw_drop_down_button(artist_data_ptr& data_,
//    box2i const& rect, widget_state state) const
//{
//    assert(!data_ || dynamic_cast<drop_down_button_data*>(data_.get()));
//    drop_down_button_data* data =
//        static_cast<drop_down_button_data*>(data_.get());
//    if (!data || data->version != impl_->version ||
//        data->widget.size != rect.size)
//    {
//        data = new drop_down_button_data;
//        data_.reset(data);
//        data->version = impl_->version;
//        data->widget.size = rect.size;
//        data->widget.themes = impl_->themes;
//        if (impl_->themes)
//        {
//            data->widget.theme = impl_->themes->combo_box;
//            data->widget.part = CP_DROPDOWNBUTTON;
//        }
//        else
//        {
//            data->widget.type = DFC_SCROLL;
//            data->widget.part = DFCS_SCROLLCOMBOBOX;
//        }
//    }
//
//    int native_state;
//    if (impl_->themes)
//    {
//        if ((state & widget_states::DISABLED) != 0)
//        {
//            native_state = CBXS_DISABLED;
//        }
//        else
//        {
//            switch (state & widget_states::PRIMARY_STATE_MASK)
//            {
//             case widget_states::HOT:
//                native_state = CBXS_HOT;
//                break;
//             case widget_states::DEPRESSED:
//                native_state = CBXS_PRESSED;
//                break;
//             default:
//                native_state = CBXS_NORMAL;
//            }
//        }
//    }
//    else
//        native_state = get_classic_state(state);
//
//    data->widget.draw(get_surface(), rect.corner, native_state);
//}

// SLIDER

int artist::get_slider_left_border() const
{
    return 5;
}
int artist::get_slider_right_border() const
{
    return 5;
}
int artist::get_slider_height() const
{
    return impl_->fixed_widgets[SLIDER_THUMB_INDEX].size[1];
}
int artist::get_default_slider_width() const
{
    return 130;
}
//box1i artist::get_slider_track_region() const
//{
//    return box1i(point1i(10), vector1i(4));
//}
//box1i artist::get_slider_track_hot_region() const
//{
//    return box1i(point1i(6), vector1i(10));
//}
box2i artist::get_slider_thumb_region() const
{
    return box2i(
        point2i(-impl_->fixed_widgets[SLIDER_THUMB_INDEX].size[0] / 2, 0),
        impl_->fixed_widgets[SLIDER_THUMB_INDEX].size);
}

//struct slider_track_data : artist_data
//{
//    unsigned version;
//    static_widget_image_data<image<rgba8> > image_data;
//    unsigned axis;
//    int width;
//};
//
//void artist::draw_slider_track(artist_data_ptr& data_, unsigned axis,
//    int width, point2i const& position) const
//{
//    assert(!data_ || dynamic_cast<slider_track_data*>(data_.get()));
//    slider_track_data* data =
//        static_cast<slider_track_data*>(data_.get());
//    if (!data || data->version != impl_->version || data->axis != axis ||
//        data->width != width)
//    {
//        data = new slider_track_data;
//        data_.reset(data);
//        data->version = impl_->version;
//        vector2i size;
//        size[axis] = width;
//        size[1 - axis] = get_slider_track_region().size[0];
//        if (impl_->themes)
//        {
//            create_themed_widget_image(&data->image_data.image, impl_->themes,
//                impl_->themes->trackbar,
//                (axis == 0) ? TKP_TRACK : TKP_TRACKVERT, TRS_NORMAL, size);
//        }
//        else
//            create_classic_slider_track_image(&data->image_data.image, size);
//        invalidate(data->image_data);
//        data->axis = axis;
//        data->width = width;
//    }
//    draw_widget(get_surface(), position, data->image_data);
//}

void artist::draw_slider_thumb(artist_data_ptr& data_, unsigned axis,
    point2i const& position, widget_state state) const
{
    point2i p = position;
    p[axis] += get_slider_thumb_region().corner[0];
    p[1 - axis] += get_slider_thumb_region().corner[1];

    fixed_widget& w = impl_->fixed_widgets[SLIDER_THUMB_INDEX + axis];
    int state_index = get_state_index(state);
    fixed_widget_image& img = w.imgs[state_index];
    if (!img.renderer)
    {
        if (impl_->themes)
        {
            int native_state;
            if ((state & widget_states::DISABLED) != 0)
            {
                native_state = TUS_DISABLED;
            }
            else
            {
                switch (state & widget_states::PRIMARY_STATE_MASK)
                {
                 case widget_states::HOT:
                    native_state = TUS_HOT;
                    break;
                 case widget_states::DEPRESSED:
                    native_state = TUS_PRESSED;
                    break;
                 default:
                    native_state = (state & widget_states::FOCUSED) ?
                        TUS_FOCUSED : TUS_NORMAL;
                }
            }
            cache_themed_widget_image(img, get_surface(), impl_->themes,
                impl_->themes->trackbar, axis == 0 ? TKP_THUMB : TKP_THUMBVERT,
                native_state, w.size);
        }
        else
        {
            cache_classic_thumb_image(img, get_surface(), EDGE_RAISED,
                BF_RECT | BF_SOFT | BF_MIDDLE, w.size);
        }
    }
    img.renderer->draw(point2d(p)); 
}

}}

#endif
