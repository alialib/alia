#ifndef ALIA_ROTATED_LAYOUT_HPP
#define ALIA_ROTATED_LAYOUT_HPP

#include <alia/layout_interface.hpp>
#include <alia/overlay.hpp>
#include <alia/scoped_state.hpp>

namespace alia {

class rotated_layout : boost::noncopyable
{
 public:
    rotated_layout() : active_(false) {}
    rotated_layout(context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~rotated_layout() { end(); }
    void begin(context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    context* ctx_;
    struct data;
    data* data_;
    layout layout_spec_;
    bool active_;
    overlay overlay_;
    scoped_transformation st_;
};

}

#endif
