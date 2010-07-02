#ifndef ALIA_UNIFORM_GRID_LAYOUT_HPP
#define ALIA_UNIFORM_GRID_LAYOUT_HPP

#include <alia/layout_object.hpp>
#include <alia/layout_logic.hpp>

namespace alia {

class uniform_grid_layout : public layout_object
{
 public:
    uniform_grid_layout() {}
    uniform_grid_layout(context& ctx, unsigned n_columns,
        layout const& layout_spec = default_layout)
    { begin(ctx, n_columns, layout_spec); }
    ~uniform_grid_layout() { end(); }
    void begin(context& ctx, unsigned n_columns,
        layout const& layout_spec = default_layout);
    void end();

 private:
    friend class uniform_linear_layout;

    context* ctx_;

    struct data;

    class logic : public layout_logic
    {
     public:
        void begin(context& ctx, data& data, unsigned n_columns);
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

        void initialize_assignment(unsigned axis);
        void inc(vector2i& index);
        int get_assigned_size(unsigned axis);

        vector2i request_index_, assignment_index_;
        unsigned n_columns_;

        // used for assignment passes
        vector2i item_size_;
        point2i position_;
        int baseline_y_;

        // The assigned size might not be evenly divisible by the number of
        // items in the grid, so the first n_larger_items_ get one extra pixel
        // each.
        vector2i n_larger_items_;
    };
    logic logic_;
};

}

#endif
