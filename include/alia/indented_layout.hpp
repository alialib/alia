#ifndef ALIA_INDENTED_LAYOUT_HPP
#define ALIA_INDENTED_LAYOUT_HPP

#include <alia/linear_layout.hpp>
#include <alia/grid_layout.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

void do_indent(context& ctx);

class indented_layout : boost::noncopyable
{
 public:
    indented_layout() {}
    indented_layout(context& ctx,layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS)
    { begin(ctx, layout_spec, flags); }
    ~indented_layout() { end(); }
    void begin(context& ctx, layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS);
    void end();
 private:
    // The order of these is significant. Children must be destructed before
    // their containers.
    row_layout full_region_;
    // TODO: Make this a configurable linear_layout.
    column_layout child_region_;
};

}

#endif
