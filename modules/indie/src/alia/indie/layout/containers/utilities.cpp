#include <alia/indie/layout/containers/utilities.hpp>

namespace alia { namespace indie {

bool
update_layout_cacher(
    layout_traversal<widget_container, widget>& traversal,
    layout_cacher& cacher,
    layout const& layout_spec,
    layout_flag_set default_flags)
{
    resolved_layout_spec resolved_spec;
    resolve_layout_spec(traversal, resolved_spec, layout_spec, default_flags);
    return detect_layout_change(
        traversal, &cacher.resolved_spec, resolved_spec);
}

horizontal_layout_query::horizontal_layout_query(
    layout_cacher& cacher, counter_type last_content_change)
    : cacher_(&cacher), last_content_change_(last_content_change)
{
}
void
horizontal_layout_query::update(
    calculated_layout_requirements const& calculated)
{
    resolve_requirements(
        cacher_->horizontal_requirements,
        cacher_->resolved_spec,
        0,
        calculated);
    cacher_->last_horizontal_query = last_content_change_;
}

vertical_layout_query::vertical_layout_query(
    layout_cacher& cacher,
    counter_type last_content_change,
    layout_scalar assigned_width)
    : cacher_(&cacher),
      last_content_change_(last_content_change),
      assigned_width_(assigned_width)
{
}
void
vertical_layout_query::update(calculated_layout_requirements const& calculated)
{
    resolve_requirements(
        cacher_->vertical_requirements, cacher_->resolved_spec, 1, calculated);
    cacher_->last_vertical_query = last_content_change_;
    cacher_->assigned_width = assigned_width_;
}

relative_region_assignment::relative_region_assignment(
    layout_node_interface& node,
    layout_cacher& cacher,
    counter_type last_content_change,
    relative_layout_assignment const& assignment)
    : cacher_(&cacher), last_content_change_(last_content_change)
{
    if (cacher_->last_relative_assignment != last_content_change_
        || cacher_->relative_assignment != assignment)
    {
        auto resolved_assignment = resolve_relative_assignment(
            cacher_->resolved_spec,
            assignment,
            cacher_->horizontal_requirements,
            node.get_vertical_requirements(assignment.region.size[0]));
        if (cacher_->resolved_relative_assignment.region.size
                != resolved_assignment.region.size
            || cacher_->resolved_relative_assignment.baseline_y
                   != resolved_assignment.baseline_y)
        {
            // This ensures that an update will be performed.
            cacher_->last_relative_assignment = 0;
        }
        cacher_->resolved_relative_assignment = resolved_assignment;
        cacher_->relative_assignment = assignment;
    }

    update_required_
        = cacher_->last_relative_assignment != last_content_change_;
}
void
relative_region_assignment::update()
{
    cacher_->last_relative_assignment = last_content_change_;
}

}} // namespace alia::indie