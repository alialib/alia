#ifndef ALIA_LINK_PANEL_HPP
#define ALIA_LINK_PANEL_HPP

#include <alia/panel.hpp>

namespace alia {

class link_panel
{
 public:
    link_panel() {}
    link_panel(context& ctx, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id)
    { begin(ctx, flags, layout_spec, id); }
    void begin(context& ctx, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout, region_id id = auto_id);
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
