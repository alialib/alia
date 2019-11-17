#include <alia/context.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

bool
is_refresh_pass(context ctx)
{
    return get_data_traversal(ctx).gc_enabled;
}

} // namespace alia
