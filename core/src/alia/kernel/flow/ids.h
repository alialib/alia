#pragma once

#include <alia/context.h>
#include <alia/impl/events.hpp>

namespace alia {

typedef void const* widget_id;
constexpr widget_id auto_id = nullptr;

inline widget_id
offset_id(widget_id main_id, uint8_t index)
{
    return reinterpret_cast<uint8_t const*>(main_id) + index;
}

// routable_widget_id identifies a widget with enough information that an
// event can be routed to it.
struct routable_widget_id
{
    widget_id id = nullptr;

    // TODO: Implement component identities.
    component_identity component;

    bool
    matches(widget_id widget) const
    {
        return this->id == widget;
    }

    explicit
    operator bool() const
    {
        return id != 0;
    }
};
static routable_widget_id const null_widget_id{0, component_identity()};

// TODO: Remove widget IDs.
inline routable_widget_id
make_routable_widget_id(ephemeral_context& ctx, widget_id id)
{
    return routable_widget_id{id, get_active_component_container(ctx)};
}

inline alia_routable_element_id
make_routable_element_id(ephemeral_context& ctx, alia_element_id id)
{
    // TODO: Add real routing information.
    return {id, {}};
}

} // namespace alia
