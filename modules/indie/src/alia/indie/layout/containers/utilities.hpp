#ifndef ALIA_INDIE_LAYOUT_CONTAINERS_UTILITIES_HPP
#define ALIA_INDIE_LAYOUT_CONTAINERS_UTILITIES_HPP

namespace alia { namespace indie {

// layout_cacher is a utility used by layout containers to cache the results
// of their layout calculations.
struct layout_cacher
{
    // the resolved layout spec supplied by the user
    resolved_layout_spec resolved_spec;

    // the last frame in which there was a horizontal requirements query
    counter_type last_horizontal_query = 0;
    // the result of that query
    layout_requirements horizontal_requirements;

    // the last frame in which there was a vertical requirements query
    counter_type last_vertical_query = 0;
    // the assigned_width associated with that query
    layout_scalar assigned_width;
    // the result of that query
    layout_requirements vertical_requirements;

    // last time set_relative_assignment was called
    counter_type last_relative_assignment = 0;
    // the last value that was passed to set_relative_assignment
    relative_layout_assignment relative_assignment;
    // the actual assignment that that value resolved to
    relative_layout_assignment resolved_relative_assignment;
};
bool
update_layout_cacher(
    layout_traversal<widget_container, widget>& traversal,
    layout_cacher& cacher,
    layout const& layout_spec,
    layout_flag_set default_flags);
struct horizontal_layout_query
{
    horizontal_layout_query(
        layout_cacher& cacher, counter_type last_content_change);
    bool
    update_required() const
    {
        return cacher_->last_horizontal_query != last_content_change_;
    }
    void
    update(calculated_layout_requirements const& calculated);
    layout_requirements const&
    result() const
    {
        return cacher_->horizontal_requirements;
    }

 private:
    layout_cacher* cacher_;
    counter_type last_content_change_;
};

struct vertical_layout_query
{
    vertical_layout_query(
        layout_cacher& cacher,
        counter_type last_content_change,
        layout_scalar assigned_width);
    bool
    update_required() const
    {
        return cacher_->assigned_width != assigned_width_
               || cacher_->last_vertical_query != last_content_change_;
    }
    void
    update(calculated_layout_requirements const& calculated);
    layout_requirements const&
    result() const
    {
        return cacher_->vertical_requirements;
    }

 private:
    layout_cacher* cacher_;
    counter_type last_content_change_;
    layout_scalar assigned_width_;
};
struct relative_region_assignment
{
    relative_region_assignment(
        layout_node_interface& node,
        layout_cacher& cacher,
        counter_type last_content_change,
        relative_layout_assignment const& assignment);
    bool
    update_required() const
    {
        return update_required_;
    }
    relative_layout_assignment const&
    resolved_assignment() const
    {
        return cacher_->resolved_relative_assignment;
    }
    void
    update();

 private:
    layout_cacher* cacher_;
    counter_type last_content_change_;
    bool update_required_;
};
// Get the resolved relative assignment for a layout cacher.
inline relative_layout_assignment const&
get_assignment(layout_cacher const& cacher)
{
    return cacher.resolved_relative_assignment;
}

// The vast majority of layout containers behave identically except for the
// logic they use to calculate their requirements and divide their space
// amongst their children.
// All the shared behavior is refactored into simple_layout_container.
template<class Logic>
struct simple_layout_container : widget_container
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        horizontal_layout_query query(cacher, last_content_change);
        if (query.update_required())
        {
            query.update(logic->get_horizontal_requirements(children));
        }
        return query.result();
    }

    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        vertical_layout_query query(
            cacher, last_content_change, assigned_width);
        if (query.update_required())
        {
            query.update(logic->get_vertical_requirements(
                children,
                resolve_assigned_width(
                    this->cacher.resolved_spec,
                    assigned_width,
                    this->get_horizontal_requirements())));
        }
        return query.result();
    }

    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        relative_region_assignment rra(
            *this, cacher, last_content_change, assignment);
        if (rra.update_required())
        {
            this->assigned_size = rra.resolved_assignment().region.size;
            logic->set_relative_assignment(
                children,
                rra.resolved_assignment().region.size,
                rra.resolved_assignment().baseline_y);
            rra.update();
        }
    }

    Logic* logic;

    layout_cacher cacher;

    layout_vector assigned_size;
};

// get_simple_layout_container is a utility function for retrieving a
// simple_layout_container with a specific type of logic from a UI context's
// data graph and refreshing it.
// template<class Logic>
// struct simple_layout_container_storage
// {
//     simple_layout_container<Logic> container;
//     Logic logic;
// };
// template<class Logic>
// void
// get_simple_layout_container(
//     layout_traversal& traversal,
//     data_traversal& data,
//     simple_layout_container<Logic>** container,
//     Logic** logic,
//     layout const& layout_spec)
// {
//     simple_layout_container_storage<Logic>* storage;
//     if (get_cached_data(data, &storage))
//         storage->container.logic = &storage->logic;

//     *container = &storage->container;

//     if (is_refresh_pass(traversal))
//     {
//         if (update_layout_cacher(
//                 traversal, (*container)->cacher, layout_spec, FILL |
//                 UNPADDED))
//         {
//             // Since this container isn't active yet, it didn't get marked
//             as
//             // needing recalculation, so we need to do that manually here.
//             (*container)->last_content_change = traversal.refresh_counter;
//         }
//     }

//     *logic = &storage->logic;
// }

#define ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(logic_type)                        \
    logic_type* logic;                                                        \
    get_simple_layout_container(                                              \
        traversal, data, &container_, &logic, layout_spec);                   \
    slc_.begin(traversal, container_);

}} // namespace alia::indie

#endif
