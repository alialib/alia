#ifndef ALIA_COMPONENTS_CONTEXT_HPP
#define ALIA_COMPONENTS_CONTEXT_HPP

#include <alia/components/privileged.hpp>
#include <alia/components/typing.hpp>

namespace alia {

struct data_traversal;
struct event_traversal;

typedef component_storage<data_traversal, event_traversal>
    context_component_storage;

typedef add_component_type_t<
    empty_component_collection<context_component_storage>,
    event_component>
    dataless_context;

typedef add_component_type_t<dataless_context, data_component> context;

// And some convenience functions...

template<class Context>
data_traversal&
get_data_traversal(Context ctx)
{
    return *get_component<data_component>(ctx);
}

bool
is_refresh_pass(context ctx);

template<class Context>
event_traversal&
get_event_traversal(Context ctx)
{
    return *get_component<event_component>(ctx);
}

} // namespace alia

#endif
