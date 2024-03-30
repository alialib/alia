#include "alia/ui/context.hpp"
#include "alia/ui/geometry.hpp"
#include <alia/ui/system/object.hpp>

#include <alia/ui/layout/containers/simple.hpp>
#include <alia/ui/layout/logic/linear.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/widget.hpp>

namespace alia {

struct root_widget : simple_layout_container<column_layout_logic>
{
    column_layout_logic logic_storage;

    void
    render(render_event& event) override
    {
        render_children(event, *this);
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        for (widget* node = this->widget_container::children; node;
             node = node->next)
        {
            node->hit_test(test, point);
        }
    }

    void
    process_input(ui_event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return identity_matrix<3, double>();
    }

    layout_box
    bounding_box() const override
    {
        return make_box(make_layout_vector(0, 0), this->assigned_size);
    }

    void
    reveal_region(region_reveal_request const&) override
    {
    }
};

void
ui_system::invoke_controller(vanilla_ui_context vanilla_ctx)
{
    layout_style_info style_info;

    ui_traversal traversal;
    initialize_layout_traversal(
        traversal.layout,
        &this->root_widget,
        is_refresh_event(vanilla_ctx),
        this->refresh_counter,
        &style_info,
        make_vector<float>(200, 200)); // TODO

    auto ctx = extend_context<ui_traversal_tag>(
        extend_context<ui_system_tag>(vanilla_ctx, *this), traversal);

    alia::root_widget* root;
    if (get_data(ctx, &root))
        root->logic = &root->logic_storage;

    if (is_refresh_event(ctx))
    {
        if (update_layout_cacher(
                get_layout_traversal(ctx),
                root->cacher,
                layout(),
                GROW_X | GROW_Y | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked as
            // needing recalculation, so we need to do that manually here.
            root->last_content_change
                = get_layout_traversal(ctx).refresh_counter;
        }

        // auto const& spec = root->cacher.resolved_spec;
        // std::cout << "-- (root) resolved_spec\n"
        //           << "growth_factor: " << spec.growth_factor << "\n"
        //           << "padding_size: " << spec.padding_size << "\n"
        //           << "flags: " << spec.flags.code << "\n"
        //           << "size: " << spec.size << std::endl;
    }
    scoped_layout_container root_scope(get_layout_traversal(ctx), root);

    this->controller(ctx);
}

} // namespace alia
