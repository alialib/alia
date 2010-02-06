#ifndef ALIA_COLLAPSIBLE_CONTENT_HPP
#define ALIA_COLLAPSIBLE_CONTENT_HPP

#include <alia/overlay.hpp>
#include <alia/linear_layout.hpp>
#include <alia/scoped_state.hpp>

namespace alia {

class collapsible_content : boost::noncopyable
{
 public:
    collapsible_content() {}
    ~collapsible_content() { end(); }

    collapsible_content(context& ctx, bool expanded,
        layout const& layout_spec = default_layout)
    { begin(ctx, expanded, layout_spec); }

    void begin(context& ctx, bool expanded,
        layout const& layout_spec = default_layout);

    void end();

    bool do_children() const { return do_children_; }

 private:
    context* ctx_;
    struct data;
    data* data_;
    layout layout_spec_;
    bool active_;
    scoped_clip_region scr_;
    scoped_transformation st_;
    linear_layout layout_;
    overlay overlay_;
    bool do_children_;
};

}

#endif
