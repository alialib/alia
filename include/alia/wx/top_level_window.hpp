#ifndef ALIA_WX_TOP_LEVEL_WINDOW_HPP
#define ALIA_WX_TOP_LEVEL_WINDOW_HPP

namespace alia { namespace wx {

class top_level_window
{
 public:
    virtual void* get_wx_window() const = 0;
};

}}

#endif
