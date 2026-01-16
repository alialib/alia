#pragma once

#include <alia/abi/events.h>
#include <alia/context.hpp>

namespace alia {

struct component_container;

typedef std::shared_ptr<component_container> component_container_ptr;

struct component_container
{
    component_container_ptr parent;
    // The component is dirty and needs to be refreshed immediately.
    bool dirty = false;
    // The component is animating and would like to be refreshed soon.
    bool animating = false;
};

void
mark_dirty_component(component_container_ptr const& container);

void
mark_dirty_component(ephemeral_context& ctx);

void
mark_animating_component(component_container_ptr const& container);

void
mark_animating_component(ephemeral_context& ctx);

// TODO
// struct scoped_component_container
// {
//     scoped_component_container()
//     {
//     }
//     scoped_component_container(core_context ctx)
//     {
//         begin(ctx);
//     }
//     scoped_component_container(
//         core_context ctx, component_container_ptr* container)
//     {
//         begin(ctx, container);
//     }
//     ~scoped_component_container()
//     {
//         end();
//     }

//     void
//     begin(dataless_core_context ctx, component_container_ptr* container);

//     void
//     begin(core_context ctx);

//     void
//     end();

//     bool
//     is_on_route() const
//     {
//         return is_on_route_;
//     }

//     bool
//     is_dirty() const
//     {
//         return is_dirty_;
//     }

//     bool
//     is_animating() const
//     {
//         return is_animating_;
//     }

//  private:
//     optional_context<ephemeral_context> ctx_;
//     component_container_ptr* container_;
//     component_container_ptr* parent_;
//     bool is_on_route_;
//     bool is_dirty_;
//     bool is_animating_;
// };

} // namespace alia
