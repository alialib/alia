#include <alia/event_routing.hpp>

namespace alia {

void scoped_event_routing_traversal::begin(
    event_routing_traversal& traversal,
    data_traversal& data,
    bool targeted,
    routing_region_ptr const& target)
{
    traversal.targeted = targeted;
    traversal.path_to_target.clear();
    if (targeted)
    {
        routing_region* region = target.get();
        while (region)
        {
            traversal.path_to_target.push_front(region);
            region = region->parent.get();
        }
    }
    traversal.data = &data;
    traversal.active_region = 0;
}

void scoped_routing_region::begin(event_routing_traversal& traversal)
{
    routing_region_ptr* region;
    if (get_data(*traversal.data, &region))
        region->reset(new routing_region);

    if (traversal.active_region)
    {
        if ((*region)->parent != *traversal.active_region)
            (*region)->parent = *traversal.active_region;
    }
    else
        (*region)->parent.reset();

    parent_ = traversal.active_region;
    traversal.active_region = region;

    if (traversal.targeted)
    {
        if (!traversal.path_to_target.empty() &&
            traversal.path_to_target.front() == region->get())
        {
            traversal.path_to_target.pop_front();
            is_relevant_ = true;
        }
        else
            is_relevant_ = false;
    }
    else
        is_relevant_ = true;

    traversal_ = &traversal;
}
void scoped_routing_region::end()
{
    if (traversal_)
    {
        traversal_->active_region = parent_;
        traversal_ = 0;
    }
}

}
