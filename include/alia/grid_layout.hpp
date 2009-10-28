#ifndef ALIA_GRID_LAYOUT_HPP
#define ALIA_GRID_LAYOUT_HPP

#include <alia/linear_layout.hpp>

namespace alia {

class grid_layout : boost::noncopyable
{
 public:
    grid_layout() : active_(false) {}
    grid_layout(context& ctx, unsigned axis = 0, // TODO: axis
        layout const& layout_spec = default_layout)
    { begin(ctx, axis, layout_spec); }
    ~grid_layout() { end(); }
    void begin(context& ctx, unsigned axis = 0,
        layout const& layout_spec = default_layout);
    bool is_relevant() const { return layout_.is_relevant(); }
    bool is_dirty() const { return layout_.is_dirty(); }
    void end();
 private:
    friend class grid_row;
    void clear_columns();
    void calculate_column_sizes(int assigned_size);
    context* ctx_;
    struct data;
    data* data_;
    bool active_;
    linear_layout layout_;
    unsigned axis_;
    bool saw_first_;
};

class grid_row : public layout_object
{
 public:
    grid_row() : active_(false) {}
    grid_row(grid_layout& grid, layout const& layout_spec = default_layout)
    { begin(grid, layout_spec); }
    ~grid_row() { end(); }
    void begin(grid_layout& grid, layout const& layout_spec = default_layout);
    void end();
 private:
    struct data;
    class logic : public layout_logic
    {
     public:
        void begin(grid_layout& grid, data& data);
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
        grid_layout* grid_;
        data* data_;
        unsigned index_;
        int position_, minimum_length_;
    };
    grid_layout* grid_;
    logic logic_;
    bool active_;
};

}

#endif
