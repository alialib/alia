#ifndef ALIA_LINK_PANEL_HPP
#define ALIA_LINK_PANEL_HPP

#include <alia/panel.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class link_panel : boost::noncopyable
{
 public:
    link_panel() {}
    link_panel(context& ctx, layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS, region_id id = auto_id)
    { begin(ctx, layout_spec, flags, id); }
    void begin(context& ctx, layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS, region_id id = auto_id);
    void end() { panel_.end(); }
    bool is_relevant() const { return panel_.is_relevant(); }
    box2i const& get_region() const { panel_.get_region(); }
    bool clicked() const { return clicked_; }
 private:
    panel panel_;
    bool clicked_;
};

}

#endif
