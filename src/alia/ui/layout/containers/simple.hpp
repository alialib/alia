#ifndef ALIA_UI_LAYOUT_CONTAINERS_SIMPLE_HPP
#define ALIA_UI_LAYOUT_CONTAINERS_SIMPLE_HPP

#include <alia/ui/layout/containers/utilities.hpp>
#include <alia/ui/layout/library.hpp>
#include <alia/ui/layout/logic/linear.hpp>

namespace alia {

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
        return cache_horizontal_layout_requirements(
            cacher, last_content_change, [&] {
                return logic->get_horizontal_requirements(children);
            });
    }

    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(
            cacher, last_content_change, assigned_width, [&] {
                return logic->get_vertical_requirements(
                    children,
                    resolve_assigned_width(
                        this->cacher.resolved_spec,
                        assigned_width,
                        this->get_horizontal_requirements()));
            });
    }

    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        // std::cout << "(simple) sra: " << assignment.region << std::endl;
        update_relative_assignment(
            *this,
            cacher,
            last_content_change,
            assignment,
            [&](auto const& resolved_assignment) {
                // std::cout << "(simple) ura: " << resolved_assignment.region
                //           << std::endl;
                this->assigned_size = resolved_assignment.region.size;
                logic->set_relative_assignment(
                    children,
                    resolved_assignment.region.size,
                    resolved_assignment.baseline_y);
            });
    }

    Logic* logic;

    layout_cacher cacher;

    layout_vector assigned_size;
};

template<class Logic>
layout_box
get_container_region(simple_layout_container<Logic> const& container)
{
    return layout_box(make_layout_vector(0, 0), container.assigned_size);
}

template<class Logic>
layout_box
get_padded_container_region(simple_layout_container<Logic> const& container)
{
    return layout_box(
        container.cacher.relative_assignment.region.corner
            - container.cacher.resolved_relative_assignment.region.corner,
        container.cacher.relative_assignment.region.size);
}

template<class Logic>
layout_vector
get_container_offset(simple_layout_container<Logic> const& container)
{
    return get_assignment(container.cacher).region.corner;
}

// get_simple_layout_container is a utility function for retrieving a
// simple_layout_container with a specific type of logic from a UI context's
// data graph and refreshing it.
template<class Logic>
struct layout_widget_container_storage
{
    layout_container_widget<simple_layout_container<Logic>> container;
    Logic logic;
};
template<class Logic>
void
get_simple_layout_container(
    layout_traversal<widget_container, widget>& traversal,
    data_traversal& data,
    layout_container_widget<simple_layout_container<Logic>>** container,
    Logic** logic,
    layout const& layout_spec)
{
    layout_widget_container_storage<Logic>* storage;
    if (get_cached_data(data, &storage))
        storage->container.logic = &storage->logic;

    *container = &storage->container;

    if (traversal.is_refresh_pass)
    {
        if (update_layout_cacher(
                traversal, (*container)->cacher, layout_spec, FILL | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked as
            // needing recalculation, so we need to do that manually here.
            (*container)->last_content_change = traversal.refresh_counter;
        }
    }

    *logic = &storage->logic;
}

template<class Logic>
struct simple_scoped_layout
{
    simple_scoped_layout()
    {
    }

    simple_scoped_layout(
        ui_context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }

    ~simple_scoped_layout()
    {
        end();
    }

    void
    begin(ui_context ctx, layout const& layout_spec = default_layout)
    {
        Logic* logic;
        get_simple_layout_container(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            &container_,
            &logic,
            layout_spec);
        slc_.begin(get_layout_traversal(ctx), container_);
    }

    void
    end()
    {
        if (container_)
        {
            slc_.end();
            container_ = nullptr;
        }
    }

    layout_box
    region() const
    {
        return get_container_region(*container_);
    }

    layout_box
    padded_region() const
    {
        return get_padded_container_region(*container_);
    }

    layout_vector
    offset() const
    {
        return get_container_offset(*container_);
    }

    layout_container_widget<simple_layout_container<Logic>>* container_
        = nullptr;
    scoped_layout_container slc_;
};

using scoped_column = simple_scoped_layout<column_layout_logic>;

} // namespace alia

#endif
