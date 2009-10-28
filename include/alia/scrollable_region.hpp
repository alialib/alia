#ifndef ALIA_SCROLABLE_REGION_HPP
#define ALIA_SCROLABLE_REGION_HPP

#include <alia/linear_layout.hpp>
#include <alia/overlay.hpp>
#include <alia/box.hpp>
#include <alia/scoped_state.hpp>
#include <alia/flags.hpp>

namespace alia {

class scrollable_region
{
 public:
    scrollable_region() : active_(false) {}
    scrollable_region(context& ctx, flag_set flags = NO_AXIS,
        layout const& layout_spec = default_layout, region_id id = auto_id)
    { begin(ctx, flags, layout_spec, id); }
    ~scrollable_region() { end(); }

    void begin(context& ctx, flag_set flags = NO_AXIS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
    void end();

    bool is_relevant() const;

    vector2i get_content_size() const;

 private:
    context* ctx_;
    struct data;
    data* data_;
    region_id id_;
    int axis_;
    layout layout_spec_;
    bool active_;
    scoped_clip_region scr_;
    scoped_transformation st_;
    linear_layout layout_;
    overlay overlay_;
    box2i window_region_;
    flag_set flags_;
};

}

#endif
