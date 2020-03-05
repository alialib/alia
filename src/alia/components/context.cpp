#include <alia/components/context.hpp>

#include <alia/components/system.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>

namespace alia {

context
make_context(
    context_component_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data,
    timing_component& timing)
{
    return add_component<data_traversal_tag>(
        add_component<timing_tag>(
            add_component<event_traversal_tag>(
                add_component<system_tag>(
                    make_empty_component_collection(storage), sys),
                event),
            timing),
        data);
}

} // namespace alia
