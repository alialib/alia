#ifndef ALIA_LAYOUT_HPP
#define ALIA_LAYOUT_HPP

#include <alia/forward.hpp>
#include <alia/box.hpp>
#include <alia/layout_interface.hpp>
#include <alia/event.hpp>

namespace alia {

bool do_layout(context& ctx);

struct layout_event : event
{
    layout_event(event_type type, event_culling_type culling_type)
      : event(LAYOUT_CATEGORY, type, culling_type)
      , active_logic(0), active_data(0), next_ptr(0), active_grids(0) {}
    layout_logic* active_logic;
    layout_data* active_data;
    layout_data** next_ptr;
    // When a grid's elements are updated, they all have to be updated
    // together, so conditional layout has to be disabled between the grid
    // and its elements.
    int active_grids;
};

struct refresh_event : layout_event
{
    refresh_event()
      : layout_event(REFRESH_EVENT, NO_CULLING)
      , layout_needed(false)
      , future_refresh_needed(false)
    {}
    bool layout_needed, future_refresh_needed;
};

// pass 0: calculate width requirements
struct layout_pass0_event : layout_event
{
    layout_pass0_event() : layout_event(LAYOUT_PASS_0, LAYOUT_CULLING) {}
};
// pass 1: assign widths, calculate height requirements
struct layout_pass1_event : layout_event
{
    layout_pass1_event() : layout_event(LAYOUT_PASS_1, LAYOUT_CULLING) {}
};
// pass 2: assign heights and positions
struct layout_pass2_event : layout_event
{
    layout_pass2_event() : layout_event(LAYOUT_PASS_2, LAYOUT_CULLING) {}
};

// Resolve a user size specification into an actual size, in pixels.
vector2i resolve_size(context& ctx, size const& s);

// This structure is filled in by the widget to inform the layout engine about
// the relevant properties of the widget itself.
struct widget_layout_info
{
    widget_layout_info(vector2i const& minimum_size, int minimum_ascent,
        int minimum_descent, vector2i const& default_size,
        layout_flag_set default_alignment, bool padded_by_default)
      : minimum_size(minimum_size)
      , minimum_ascent(minimum_ascent)
      , minimum_descent(minimum_descent)
      , default_size(default_size)
      , default_alignment(default_alignment)
      , padded_by_default(padded_by_default)
    {}
    vector2i minimum_size;
    int minimum_ascent, minimum_descent;
    vector2i default_size;
    layout_flag_set default_alignment;
    bool padded_by_default;
};

struct resolved_layout_spec
{
    vector2i size;
    int ascent, descent;
    layout_flag_set alignment;
    float proportion;
    vector2i padding_size;
};

bool operator==(resolved_layout_spec const& a, resolved_layout_spec const& b);
bool operator!=(resolved_layout_spec const& a, resolved_layout_spec const& b);

struct layout_data
{
    layout_data() : dirty(false), parent(0), next(0) {}
    // if this is set, the layout for this node needs to be recalculated
    bool dirty;
    // this is the result of layout and is used during non-layout passes
    box2i assigned_region;
    // these make it possible to determine if nodes have moved around
    layout_data* parent;
    layout_data* next;
    // if this changes, something has changed in the layout of the node itself
    resolved_layout_spec spec;
};

struct layout_object_data
{
    layout_object_data() : children(0), minimum_size(0, 0),
        minimum_ascent(0), minimum_descent(0) {}
    alia::layout_data layout_data;
    alia::layout_data* children;
    vector2i minimum_size;
    int minimum_ascent, minimum_descent;
};

void layout_widget(context& ctx, layout_data& data,
    layout const& spec, vector2i const& requested_size,
    widget_layout_info const& info);

// layout_widget() is implemented in terms of the following.  If you need more
// control over how your widget is laid out, you can call them separately.

void do_prechild_layout(context& ctx, layout_data& data,
    layout const& spec, vector2i const& requested_size,
    widget_layout_info const& info);

void do_postchild_layout(context& ctx, layout_data& data,
    layout const& spec, vector2i const& requested_size,
    widget_layout_info const& info);

void record_layout_change(context& ctx, layout_data& data);

void diff_widget_layout(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved);
void diff_widget_location(context& ctx, layout_data& data);
void diff_layout_spec(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved);

void resolve_layout_spec(context& ctx, resolved_layout_spec* resolved,
    layout const& spec, vector2i const& resolved_size,
    widget_layout_info const& info);

void record_layout(context& ctx, layout_data& data,
    resolved_layout_spec const& resolved);

void request_horizontal_space(context& ctx,
    resolved_layout_spec const& resolved);

int get_assigned_width(context& ctx, resolved_layout_spec const& resolved);

void request_vertical_space(context& ctx,
    resolved_layout_spec const& resolved);

void get_assigned_region(context& ctx, box2i* region,
    resolved_layout_spec const& resolved);

}

#endif
