#include <alia/context/interface.hpp>

#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/system/interface.hpp>

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
    return make_context(detail::make_empty_structural_collection(storage))
        .add<system_tag>(sys)
        .add<event_traversal_tag>(event)
        .add<timing_tag>(timing)
        .add<data_traversal_tag>(data);
}

struct scoped_context_content_id_data
{
    captured_id id_from_above;
    captured_id id_from_here;
    unsigned id_to_present = 0;
};

void
scoped_context_content_id::begin(context ctx, id_interface const& id)
{
    ctx_.reset(ctx);
    parent_id_ = ctx.contents_.storage->content_id;

    auto& data = get_cached_data<scoped_context_content_id_data>(ctx);

    if (!data.id_from_above.matches(*parent_id_))
    {
        ++data.id_to_present;
        data.id_from_above.capture(*parent_id_);
    }

    if (!data.id_from_here.matches(id))
    {
        ++data.id_to_present;
        data.id_from_here.capture(id);
    }

    this_id_ = make_id(data.id_to_present);
    ctx.contents_.storage->content_id = &this_id_;
}

void
scoped_context_content_id::end()
{
    if (ctx_)
    {
        ctx_->contents_.storage->content_id = parent_id_;
        ctx_.reset();
    }
}

} // namespace alia
