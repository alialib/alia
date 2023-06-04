#include <alia/core/context/interface.hpp>

#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/system/interface.hpp>

namespace alia {

context
make_context(
    context_storage* storage,
    system& sys,
    event_traversal& event,
    data_traversal& data,
    timing_subsystem& timing)
{
    storage->content_id = &unit_id;
    return detail::add_context_object<data_traversal_tag>(
        detail::add_context_object<timing_tag>(
            detail::add_context_object<event_traversal_tag>(
                detail::add_context_object<system_tag>(
                    make_context(
                        detail::make_empty_structural_collection(storage)),
                    std::ref(sys)),
                std::ref(event)),
            std::ref(timing)),
        std::ref(data));
}

namespace detail {

struct context_content_id_folding_data
{
    captured_id id_from_above;
    captured_id id_from_here;
    simple_id<unsigned> id_to_present = {0};
};

void
fold_in_content_id(context ctx, id_interface const& id)
{
    id_interface const* id_from_above = ctx.contents_.storage->content_id;

    auto& data = get_cached_data<context_content_id_folding_data>(ctx);

    if (!data.id_from_above.matches(*id_from_above))
    {
        ++data.id_to_present.value_;
        data.id_from_above.capture(*id_from_above);
    }

    if (!data.id_from_here.matches(id))
    {
        ++data.id_to_present.value_;
        data.id_from_here.capture(id);
    }

    ctx.contents_.storage->content_id = &data.id_to_present;
}

} // namespace detail

} // namespace alia
