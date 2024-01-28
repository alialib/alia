#include <alia/core/context/interface.hpp>

#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/system/interface.hpp>

namespace alia { namespace detail {

struct context_content_id_folding_data
{
    captured_id id_from_above;
    captured_id id_from_here;
    simple_id<unsigned> id_to_present = {0};
};

void
fold_in_content_id(core_context ctx, id_interface const& id)
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

}} // namespace alia::detail
