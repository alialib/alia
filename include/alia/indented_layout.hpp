#ifndef ALIA_INDENTED_LAYOUT_HPP
#define ALIA_INDENTED_LAYOUT_HPP

#include <alia/linear_layout.hpp>
#include <alia/grid_layout.hpp>

namespace alia {

class indented_layout
{
 public:
    indented_layout() : active_(false) {}
    indented_layout(context& ctx, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout)
    { begin(ctx, flags, layout_spec); }
    void begin(context& ctx, flag_set flags = NO_FLAGS,
        layout const& layout_spec = default_layout);
    void end();
 private:
    context* ctx_;
    // The order of these is significant. Children must be destructed before
    // their containers.
    row_layout full_region_;
    // TODO: Make this a configurable linear_layout.
    column_layout child_region_;
};

}

#endif
