#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/utilities.hpp>

namespace alia { namespace indie {

bool
operator==(absolute_length const& a, absolute_length const& b)
{
    return a.length == b.length && a.units == b.units;
}
bool
operator!=(absolute_length const& a, absolute_length const& b)
{
    return !(a == b);
}

bool
operator==(relative_length const& a, relative_length const& b)
{
    return a.length == b.length && a.is_relative == b.is_relative
           && (!a.is_relative || a.units == b.units);
}
bool
operator!=(relative_length const& a, relative_length const& b)
{
    return !(a == b);
}

bool
operator==(layout const& a, layout const& b)
{
    return a.size == b.size && a.flags == b.flags
           && a.growth_factor == b.growth_factor;
}
bool
operator!=(layout const& a, layout const& b)
{
    return !(a == b);
}

}} // namespace alia::indie
