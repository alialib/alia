#ifndef ALIA_OVERLAY_HPP
#define ALIA_OVERLAY_HPP

#include <alia/layout_logic.hpp>
#include <alia/box.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class overlay : boost::noncopyable
{
 public:
    overlay() : active_(false) {}
    overlay(context& ctx, point2i const& position) { begin(ctx, position); }
    overlay(context& ctx, box2i const& region) { begin(ctx, region); }
    ~overlay() { end(); }
    void begin(context& ctx, point2i const& position);
    void begin(context& ctx, box2i const& region);
    void end();
    vector2i const& get_minimum_size() const;
    int get_minimum_ascent() const;
    int get_minimum_descent() const;
 private:
    struct data;
    void begin(context& ctx, data& data, box2i const& region);
    class logic : public layout_logic
    {
     public:
        void begin(data& data, box2i const& region);
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
        data* data_;
        box2i region_;
    };
    context* ctx_;
    logic logic_;
    bool active_;
    layout_logic* old_logic_;
    layout_data* old_data_;
    layout_data** old_next_ptr_;
};

}

#endif
