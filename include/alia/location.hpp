#ifndef ALIA_LOCATION_HPP
#define ALIA_LOCATION_HPP

#include <alia/id.hpp>
#include <alia/event.hpp>
#include <alia/point.hpp>

namespace alia {

void mark_location_impl(context& ctx, id_interface const& id);
template<class Id>
void mark_location(context& ctx, Id const& id)
{ mark_location_impl(ctx, typed_id<Id>(id)); }

struct location_event : event
{
    location_event(event_category category, event_type type,
      id_interface const& id)
      : event(category, type, NO_CULLING)
      , id(id.clone())
    {}
    owned_id id;
};

struct jump_event : location_event
{
    enum state_t
    {
        SEARCHING, // still searching for specified ID
        WAITING, // waiting for the first region after the ID
        JUMPING, // found the region, forcing it to be visible
    };
    jump_event(id_interface const& id)
      : location_event(LAYOUT_CATEGORY, JUMP_TO_LOCATION, id)
      , state(SEARCHING)
    {}
    state_t state;
    point2i point;
};

void jump_to_location(context& ctx, id_interface const& id);

}

#endif
