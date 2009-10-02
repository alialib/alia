#ifndef ALIA_LAYERED_LAYOUT_HPP
#define ALIA_LAYERED_LAYOUT_HPP

#include <alia/layout_object.hpp>
#include <alia/layout_logic.hpp>

namespace alia {

// A layered layout object assigns the same region to all of its children, so
// later children cover earlier children (generally only partially).
class layered_layout : public layout_object
{
 public:
    layered_layout() : active_(false) {}
    layered_layout(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~layered_layout() { end(); }
    void begin(context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    struct data;
    class logic : public layout_logic
    {
     public:
        void begin(context& ctx, data& data);
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
        data* data_;
    };
    logic logic_;
    bool active_;
};

}

#endif
