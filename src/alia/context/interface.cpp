#include <alia/context/interface.hpp>

#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/system.hpp>

namespace alia {

context
make_context(
    context_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data,
    timing_component& timing)
{
    return make_context(impl::make_empty_structural_collection(storage))
        .extend<system_tag>(sys)
        .extend<event_traversal_tag>(event)
        .extend<timing_tag>(timing)
        .extend<data_traversal_tag>(data);
}

} // namespace alia
