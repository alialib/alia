#include <alia/html/routing.hpp>

namespace alia { namespace html {

direct_const_signal<std::string>
get_location_hash(html::context ctx)
{
    return direct(const_cast<std::string const&>(get<system_tag>(ctx).hash));
}

}} // namespace alia::html