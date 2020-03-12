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
    timing_subsystem& timing)
{
    return make_context(impl::make_empty_structural_collection(storage))
        .add<system_tag>(sys)
        .add<event_traversal_tag>(event)
        .add<timing_tag>(timing)
        .add<data_traversal_tag>(data);
}

} // namespace alia
