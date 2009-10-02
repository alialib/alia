#include <alia/location.hpp>
#include <alia/context.hpp>

namespace alia {

void mark_location_impl(context& ctx, id_interface const& id)
{
    if (ctx.event->type == JUMP_TO_LOCATION)
    {
        jump_event& e = get_event<jump_event>(ctx);
        if (e.state == jump_event::SEARCHING && id_ref(e.id.id) == id_ref(&id))
            e.state = jump_event::WAITING;
    }
}

void jump_to_location(context& ctx, id_interface const& id)
{
    jump_event e(id);
    issue_event(ctx, e);
}

}
