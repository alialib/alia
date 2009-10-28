#ifndef ALIA_PANEL_HPP
#define ALIA_PANEL_HPP

#include <alia/scrollable_region.hpp>
#include <alia/artist.hpp>
#include <alia/scoped_state.hpp>

namespace alia {

class panel_border : boost::noncopyable
{
 public:
    panel_border() : active_(false) {}
    panel_border(context& ctx, unsigned style_code,
        layout const& layout_spec = default_layout)
    { begin(ctx, style_code, layout_spec); }
    ~panel_border() { end(); }
    void begin(context& ctx, unsigned style_code,
        layout const& layout_spec = default_layout);
    void end();
    box2i const& get_content_region() const;
 private:
    context* ctx_;
    struct data;
    data* data_;
    layout layout_spec_;
    bool active_;
    unsigned style_code_;
    overlay overlay_;
};

class panel_background : boost::noncopyable
{
 public:
    panel_background() : active_(false) {}
    panel_background(context& ctx, unsigned style_code, flag_set flags,
        layout const& layout_spec, region_id id)
    { begin(ctx, style_code, flags, layout_spec, id); }
    ~panel_background() { end(); }
    void begin(context& ctx, unsigned style_code, flag_set flags,
        layout const& layout_spec, region_id id);
    void end();
    bool is_relevant() const;
    box2i const& get_region() const { return region_; }
 private:
    context* ctx_;
    bool active_;
    scoped_style ss_;
    linear_layout layout_;
    box2i region_;
    vector2i outer_padding_size_;
};

class panel : boost::noncopyable
{
 public:
    panel() {}
    panel(context& ctx, style style, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id)
    { begin(ctx, style, flags, layout_spec, id); }
    panel(context& ctx, unsigned style_code, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id)
    { begin(ctx, style_code, flags, layout_spec, id); }
    ~panel() { end(); }
    void begin(context& ctx, style style, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
    void begin(context& ctx, unsigned style_code, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
    void end();
    bool is_relevant() const;
    box2i const& get_region() const;
 private:
    //panel_border border_;
    panel_background bg_;
};

class scrollable_panel : boost::noncopyable
{
 public:
    scrollable_panel() {}
    scrollable_panel(context& ctx, style style, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id)
    { begin(ctx, style, flags, layout_spec, id); }
    scrollable_panel(context& ctx, unsigned style_code,
        flag_set flags = NO_FLAGS, layout const& layout_spec = default_layout,
        region_id id = auto_id)
    { begin(ctx, style_code, flags, layout_spec, id); }
    ~scrollable_panel() { end(); }
    void begin(context& ctx, style style, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
    void begin(context& ctx, unsigned style_code, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
    void end();
    bool is_relevant() const;
    box2i const& get_region() const;
 private:
    //panel_border border_;
    scrollable_region sr_;
    panel_background bg_;
};

}

#endif
