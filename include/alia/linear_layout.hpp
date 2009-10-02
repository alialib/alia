#ifndef ALIA_LINEAR_LAYOUT_HPP
#define ALIA_LINEAR_LAYOUT_HPP

#include <alia/layout_object.hpp>
#include <alia/layout_logic.hpp>

namespace alia {

// A linear_layout object arranges its children in a line, either horizontally
// or vertically, depending on the axis specified.
class linear_layout : public layout_object
{
 public:
    linear_layout() {}
    linear_layout(context& ctx, unsigned axis,
        layout const& layout_spec = default_layout)
    { begin(ctx, axis, layout_spec); }
    ~linear_layout() { end(); }
    void begin(context& ctx, unsigned axis,
        layout const& layout_spec = default_layout);
    void end();
 private:
    struct data;
    class logic : public layout_logic
    {
     public:
        void begin(context& ctx, data& data, unsigned axis);
        void end();
        // implementation of layout_logic interface...
        void request_horizontal_space_for_node(int minimum_width,
            float proportion);
        int get_width_for_node(int minimum_width,
            float proportion);
        void request_vertical_space_for_node(int minimum_height,
            int minimum_ascent, int minimum_descent, float proportion);
        void get_region_for_node(box2i* region, int* baseline_y,
            vector2i const& minimum_size, float proportion);
     private:
        context* ctx_;
        data* data_;
        int axis_;
        // counts the number of items seen so far
        int item_n_;
        // used during assignment passes to track how much proportion is left
        float remaining_proportion_;
        // used by rows during assignment passes
        int baseline_y_;
        // used during assignment passes to track the current posiion along the
        // major axis
        int major_position_;
        // used during assignment passes to track how much extra space is left
        int extra_space_;
    };
    logic logic_;
};

class row_layout : public linear_layout
{
 public:
    row_layout() {}
    row_layout(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    void begin(context& ctx, layout const& layout_spec = default_layout)
    { linear_layout::begin(ctx, 0, layout_spec); }
};

class column_layout : public linear_layout
{
 public:
    column_layout() {}
    column_layout(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    void begin(context& ctx, layout const& layout_spec = default_layout)
    { linear_layout::begin(ctx, 1, layout_spec); }
};

}

#endif
