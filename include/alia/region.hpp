#ifndef ALIA_REGION_HPP
#define ALIA_REGION_HPP

#include <alia/typedefs.hpp>

namespace alia {

struct region_data
{
    region_data() : last_refresh(0) {}
    uint64 last_refresh;
    layout_data* container;
};

}

#endif
