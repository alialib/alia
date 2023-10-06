#ifndef ALIA_INDIE_LAYOUT_CONTAINERS_UTILITIES_HPP
#define ALIA_INDIE_LAYOUT_CONTAINERS_UTILITIES_HPP

namespace alia { namespace indie {

// scoped_layout_container makes a layout container active for its scope.
struct scoped_layout_container : noncopyable
{
    scoped_layout_container() : traversal_(0)
    {
    }
    scoped_layout_container(
        layout_traversal<widget_container, widget>& traversal,
        widget_container* container)
    {
        begin(traversal, container);
    }
    ~scoped_layout_container()
    {
        end();
    }
    void
    begin(
        layout_traversal<widget_container, widget>& traversal,
        widget_container* container);
    void
    end();

 private:
    layout_traversal<widget_container, widget>* traversal_;
};

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

template<class Calculator>
layout_requirements const&
cache_horizontal_layout_requirements(
    layout_cacher& cacher,
    counter_type last_content_change,
    Calculator&& calculator)
{
    if (cacher.last_horizontal_query != last_content_change)
    {
        resolve_requirements(
            cacher.horizontal_requirements,
            cacher.resolved_spec,
            0,
            std::forward<Calculator>(calculator)());
        cacher.last_horizontal_query = last_content_change;
    }
    return cacher.horizontal_requirements;
}

template<class Calculator>
layout_requirements const&
cache_vertical_layout_requirements(
    layout_cacher& cacher,
    counter_type last_content_change,
    layout_scalar assigned_width,
    Calculator&& calculator)
{
    if (cacher.assigned_width != assigned_width
        || cacher.last_vertical_query != last_content_change)
    {
        resolve_requirements(
            cacher.vertical_requirements,
            cacher.resolved_spec,
            1,
            std::forward<Calculator>(calculator)());
        cacher.last_vertical_query = last_content_change;
        cacher.assigned_width = assigned_width;
    }
    return cacher.vertical_requirements;
}

template<class Assigner>
void
update_relative_assignment(
    layout_node_interface& node,
    layout_cacher& cacher,
    counter_type last_content_change,
    relative_layout_assignment const& assignment,
    Assigner&& assigner)
{
    if (cacher.last_relative_assignment != last_content_change
        || cacher.relative_assignment != assignment)
    {
        auto resolved_assignment = resolve_relative_assignment(
            cacher.resolved_spec,
            assignment,
            cacher.horizontal_requirements,
            node.get_vertical_requirements(assignment.region.size[0]));
        if (cacher.last_relative_assignment != last_content_change
            || cacher.resolved_relative_assignment.region.size
                   != resolved_assignment.region.size
            || cacher.resolved_relative_assignment.baseline_y
                   != resolved_assignment.baseline_y)
        {
            std::forward<Assigner>(assigner)(resolved_assignment);
            cacher.last_relative_assignment = last_content_change;
        }
        cacher.resolved_relative_assignment = resolved_assignment;
        cacher.relative_assignment = assignment;
    }
}

// Get the resolved relative assignment for a layout cacher.
inline relative_layout_assignment const&
get_assignment(layout_cacher const& cacher)
{
    return cacher.resolved_relative_assignment;
}

template<class LayoutContainer>
struct layout_container_widget : LayoutContainer
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = get_assignment(this->cacher).region;
        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0]);
        bounds.fTop = SkScalar(region.corner[1]);
        bounds.fRight = SkScalar(region.corner[0] + region.size[0]);
        bounds.fBottom = SkScalar(region.corner[1] + region.size[1]);
        if (!canvas.quickReject(bounds))
        {
            canvas.save();
            auto const& offset = region.corner;
            canvas.translate(offset[0], offset[1]);
            indie::render_children(canvas, *this);
            canvas.restore();
        }
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto const& region = get_assignment(this->cacher).region;
        if (is_inside(region, vector<2, float>(point)))
        {
            auto local_point = point - vector<2, double>(region.corner);
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }
        }
    }

    void
    process_input(event_context) override
    {
    }
};

#define ALIA_BEGIN_SIMPLE_LAYOUT_CONTAINER(logic_type)                        \
    logic_type* logic;                                                        \
    get_simple_layout_container(                                              \
        traversal, data, &container_, &logic, layout_spec);                   \
    slc_.begin(traversal, container_);

}} // namespace alia::indie

#endif
