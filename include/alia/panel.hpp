#ifndef ALIA_PANEL_HPP
#define ALIA_PANEL_HPP

#include <alia/scrollable_region.hpp>
#include <alia/artist.hpp>
#include <alia/scoped_state.hpp>
#include <alia/style_utils.hpp>

namespace alia {

//class panel_border : boost::noncopyable
//{
// public:
//    panel_border() : active_(false) {}
//    panel_border(context& ctx, unsigned style_code,
//        layout const& layout_spec = default_layout)
//    { begin(ctx, style_code, layout_spec); }
//    ~panel_border() { end(); }
//    void begin(context& ctx, unsigned style_code,
//        layout const& layout_spec = default_layout);
//    void end();
//    box2i const& get_content_region() const;
// private:
//    context* ctx_;
//    struct data;
//    data* data_;
//    layout layout_spec_;
//    bool active_;
//    unsigned style_code_;
//    overlay overlay_;
//};

class panel_border : boost::noncopyable
{
 public:
    panel_border() {}
    panel_border(context& ctx, layout const& layout_spec)
    { begin(ctx, layout_spec); }
    ~panel_border() { end(); }
    void begin(context& ctx, layout const& layout_spec);
    void end();
    box2i const& get_region() const { return region_; }
    bool is_relevant() const;
 private:
    linear_layout outer_;
    box2i region_;
};

class panel_background : boost::noncopyable
{
 public:
    panel_background() {}
    panel_background(context& ctx, char const* style,
        layout const& layout_spec, flag_set flags, region_id id,
        widget_state state)
    { begin(ctx, style, layout_spec, flags, id, state); }
    ~panel_background() { end(); }
    void begin(context& ctx, char const* style,
        layout const& layout_spec, flag_set flags, region_id id,
        widget_state state);
    void end();
    box2i const& get_region() const { return region_; }
    bool is_relevant() const;
 private:
    linear_layout outer_;
    scoped_substyle substyle_;
    linear_layout inner_;
    box2i region_;
};

class panel : boost::noncopyable
{
 public:
    panel() {}
    panel(context& ctx, char const* style,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS,
        region_id id = auto_id, widget_state state = widget_states::NORMAL)
    { begin(ctx, style, layout_spec, flags, id, state); }
    ~panel() { end(); }
    void begin(context& ctx, char const* style,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS,
        region_id id = auto_id, widget_state state = widget_states::NORMAL);
    void end();
    bool is_relevant() const;
    box2i const& get_region() const;
 private:
    panel_border border_;
    panel_background bg_;
};

class scrollable_panel : boost::noncopyable
{
 public:
    scrollable_panel() {}
    scrollable_panel(context& ctx, char const* style,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS,
        region_id id = auto_id)
    { begin(ctx, style, layout_spec, flags, id); }
    ~scrollable_panel() { end(); }
    void begin(context& ctx, char const* style,
        layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS,
        region_id id = auto_id);
    void end();
    bool is_relevant() const;
    box2i const& get_region() const;
 private:
    panel_border border_;
    scrollable_region sr_;
    panel_background bg_;
};

}

#endif
