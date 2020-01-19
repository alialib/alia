#include <alia/components/context.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

context
make_context(
    context_component_storage* storage,
    system* sys,
    event_traversal* event,
    data_traversal* data)
{
    return add_component<data_traversal_tag>(
        add_component<event_traversal_tag>(
            add_component<system_tag>(
                make_empty_component_collection(storage), sys),
            event),
        data);
}

bool
is_refresh_pass(context ctx)
{
    return get_data_traversal(ctx).gc_enabled;
}

} // namespace alia
