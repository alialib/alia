#ifndef ALIA_WX_IMPL_HPP
#define ALIA_WX_IMPL_HPP

namespace alia { namespace wx {

namespace impl {

class top_level_window
{
 public:
    virtual void update() = 0;
    virtual void close() = 0;
    virtual void adjust_font_sizes() = 0;
    virtual void update_color_scheme() = 0;
};

}

}}

#endif
