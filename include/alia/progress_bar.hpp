#ifndef ALIA_PROGRESS_BAR_HPP
#define ALIA_PROGRESS_BAR_HPP

#include <alia/layout.hpp>

namespace alia {

// value ranges from 0 (no progress) to 1 (done)

void do_progress_bar(context& ctx, double value,
    layout const& layout_spec = default_layout);

}

#endif
