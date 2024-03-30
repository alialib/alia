#include <alia/ui/layout/containers/utilities.hpp>

namespace alia {

void
scoped_layout_container::begin(
    layout_traversal<widget_container, widget>& traversal,
    widget_container* container)
{
    if (traversal.is_refresh_pass)
    {
        traversal_ = &traversal;

        set_next_node(traversal, container);
        container->parent = traversal.active_container;

        traversal.next_ptr = &container->children;
        traversal.active_container = container;
    }
    else
    {
        traversal_ = nullptr;
    }
}
void
scoped_layout_container::end()
{
    if (traversal_)
    {
        set_next_node(*traversal_, nullptr);

        widget_container* container = traversal_->active_container;
        traversal_->next_ptr = &container->next;
        traversal_->active_container = container->parent;

        traversal_ = nullptr;
    }
}

bool
update_layout_cacher(
    layout_traversal<widget_container, widget>& traversal,
    layout_cacher& cacher,
    layout const& layout_spec,
    layout_flag_set default_flags)
{
    resolved_layout_spec resolved_spec;
    // std::cout << "-- (update_layout_cacher) inputs\n"
    //           << "default_flags: " << default_flags.code << "\n"
    //           << "spec.flags: " << layout_spec.flags.code << "\n"
    //           << "spec.growth_factor: " << layout_spec.growth_factor <<
    //           "\n";
    resolve_layout_spec(traversal, resolved_spec, layout_spec, default_flags);
    // std::cout << "-- (update_layout_cacher) resolved_spec\n"
    //           << "growth_factor: " << resolved_spec.growth_factor << "\n"
    //           << "padding_size: " << resolved_spec.padding_size << "\n"
    //           << "flags: " << resolved_spec.flags.code << "\n"
    //           << "size: " << resolved_spec.size << std::endl;
    return detect_layout_change(
        traversal, &cacher.resolved_spec, resolved_spec);
}

} // namespace alia
