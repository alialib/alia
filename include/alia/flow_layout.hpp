#ifndef ALIA_FLOW_LAYOUT_HPP
#define ALIA_FLOW_LAYOUT_HPP

#include <alia/layout_object.hpp>
#include <alia/layout_logic.hpp>
#include <alia/point.hpp>

namespace alia {

struct flow_layout_data;

class flow_layout_logic : public layout_logic
{
 public:
    void begin(context& ctx, flow_layout_data& data);
    void end();
    int get_remaining_width() const { return remaining_width_; }
    void start_new_row();
    // implementation of layout_logic interface...
    void request_horizontal_space_for_node(int minimum_width,
        float proportion);
    int get_width_for_node(int minimum_width,
        float proportion);
    void request_vertical_space_for_node(int minimum_height,
        int minimum_ascent, int minimum_descent, float proportion);
    void get_region_for_node(box2i* region, int* baseline_y,
        vector2i const& minimum_size, float proportion);
    int get_remaining_width();
 private:
    void start_row_assignment();
    context* ctx_;
    flow_layout_data* data_;
    int remaining_width_, extra_space_, row_n_;
    point2i position_;
    float remaining_proportion_;
};

class flow_layout : public layout_object
{
 public:
    flow_layout() {}
    flow_layout(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~flow_layout() { end(); }
    void begin(context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    flow_layout_logic logic_;
};

}

#endif
