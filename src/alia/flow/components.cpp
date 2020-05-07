#include <alia/flow/components.hpp>
#include <alia/flow/events.hpp>

namespace alia {

void
mark_dirty_component(component_container_ptr const& container)
{
    component_container* c = container.get();
    while (c && !c->dirty)
    {
        c->dirty = true;
        c = c->parent.get();
    }
}

void
mark_dirty_component(dataless_context ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);
    mark_dirty_component(*traversal.active_container);
}

void
mark_animating_component(component_container_ptr const& container)
{
    component_container* r = container.get();
    while (r && !r->animating)
    {
        r->animating = true;
        r = r->parent.get();
    }
}

void
mark_animating_component(dataless_context ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);
    mark_animating_component(*traversal.active_container);
}

void
scoped_component_container::begin(
    dataless_context ctx, component_container_ptr* container)
{
    event_traversal& traversal = get_event_traversal(ctx);

    ctx_.reset(ctx);

    container_ = container;

    if (traversal.active_container)
    {
        if ((*container)->parent != *traversal.active_container)
            (*container)->parent = *traversal.active_container;
    }
    else
        (*container)->parent.reset();

    parent_ = traversal.active_container;
    traversal.active_container = container;

    is_dirty_ = (*container)->dirty;
    (*container)->dirty = false;

    is_animating_ = (*container)->animating;
    (*container)->animating = false;

    if (traversal.targeted)
    {
        if (traversal.path_to_target
            && traversal.path_to_target->node == container->get())
        {
            traversal.path_to_target = traversal.path_to_target->rest;
            is_on_route_ = true;
        }
        else
            is_on_route_ = false;
    }
    else
        is_on_route_ = true;
}

void
scoped_component_container::begin(context ctx)
{
    component_container_ptr* container;
    if (get_data(ctx, &container))
        container->reset(new component_container);

    this->begin(ctx, container);
}

void
scoped_component_container::end()
{
    if (ctx_)
    {
        auto ctx = *ctx_;
        get_event_traversal(ctx).active_container = parent_;
        ctx_.reset();
    }
}

} // namespace alia
