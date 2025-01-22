#include <alia/ui/library/lazy_list.hpp>

#include <alia/ui/events.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>

namespace alia {

// TODO: Move the component stuff somewhere more generic.

struct component_data
{
    data_block block;
    layout_system layout;
};

template<class Component>
void
refresh_component(ui_context ctx, component_data& data, Component&& component)
{
    // TODO: Make this all less hacky...

    refresh_event refresh;
    event_traversal traversal;
    traversal.targeted = false;
    traversal.type_code = REFRESH_EVENT;
    traversal.event_type = &typeid(refresh_event);
    traversal.event = &refresh;
    traversal.is_refresh = true;

    auto* old_event = ctx.contents_.storage->event;
    ctx.contents_.storage->event = &traversal;
    traversal.active_container = old_event->active_container;
    auto old_layout = ctx.contents_.storage->ui_traversal->layout;

    layout_traversal new_layout;
    initialize_layout_traversal(
        data.layout,
        new_layout,
        true,
        nullptr,
        old_layout.style_info,
        old_layout.ppi);
    ctx.contents_.storage->ui_traversal->layout = new_layout;

    auto& data_traversal = get_data_traversal(ctx);
    data_traversal.gc_enabled = data_traversal.cache_clearing_enabled = true;

    {
        // static component_container_ptr new_root_component
        //     = std::make_shared<component_container>();
        // scoped_component_container root(ctx, &new_root_component);

        scoped_data_block sdb(ctx, data.block);

        std::forward<Component>(component)(ctx);
    }

    data_traversal.gc_enabled = data_traversal.cache_clearing_enabled = false;
    ctx.contents_.storage->event = old_event;
    ctx.contents_.storage->ui_traversal->layout = old_layout;
}

struct lazy_list_item_data : component_data
{
    counter_type last_refresh;
};

struct lazy_list_data
{
    size_t item_count = 0;
    std::unordered_map<size_t, lazy_list_item_data> items;
    component_container_ptr component;
    relative_layout_assignment assignment;
    layout_scalar item_height = 0;
    std::pair<ptrdiff_t, ptrdiff_t> active_range = {0, 0};
};

template<class DoItem>
void
invoke_lazy_list_item(
    ui_context ctx, lazy_list_item_data& data, DoItem&& do_item, size_t index)
{
    scoped_data_block sdb(ctx, data.block);
    std::forward<DoItem>(do_item)(ctx, index);
}

template<class DoItem>
void
refresh_lazy_list_item(
    ui_context ctx,
    lazy_list_data& list,
    lazy_list_item_data& item,
    DoItem&& do_item,
    size_t index)
{
    if (item.last_refresh != get<core_system_tag>(ctx).refresh_counter)
    {
        refresh_component(ctx, item, [&](ui_context ctx) {
            std::forward<DoItem>(do_item)(ctx, index);
        });

        relative_layout_assignment item_assignment;
        item_assignment.region.corner[0] = list.assignment.region.corner[0];
        item_assignment.region.corner[1]
            = list.assignment.region.corner[1]
            + list.assignment.region.size[1] / list.item_count * index;
        item_assignment.region.size[0] = list.assignment.region.size[0];
        item_assignment.region.size[1]
            = list.assignment.region.size[1] / list.item_count;
        item_assignment.baseline_y = item_assignment.region.size[1];

        resolve_layout(item.layout.root_node, item_assignment.region);

        item.last_refresh = get<core_system_tag>(ctx).refresh_counter;
    }
}

struct lazy_list_layout_container : layout_node
{
    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        return layout_requirements{0, 0, 0, 1};
    }
    layout_requirements
    get_vertical_requirements(layout_scalar width) override
    {
        if (data.item_count > 0)
        {
            auto* root_node = data.items[0].layout.root_node;
            if (root_node)
            {
                auto const y = root_node->get_vertical_requirements(width);
                data.item_height = y.size;
            }
            else
            {
                data.item_height = 0;
            }
        }
        else
        {
            data.item_height = 0;
        }
        return layout_requirements{
            data.item_height * data.item_count, 0, 0, 0};
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        data.assignment = assignment;
    }

    lazy_list_data data;
};

inline ptrdiff_t
clamp_index(lazy_list_data const& list, ptrdiff_t index)
{
    return std::clamp(index, ptrdiff_t(0), ptrdiff_t(list.item_count) - 1);
}

std::pair<ptrdiff_t, ptrdiff_t>
compute_visible_range(ui_context ctx, lazy_list_data const& list)
{
    auto& geometry = get_geometry_context(ctx);
    auto const window_low = geometry.clip_region.corner;
    auto const window_high = get_high_corner(geometry.clip_region);
    auto const region = box<2, double>(list.assignment.region);
    auto const region_low
        = transform(geometry.transformation_matrix, region.corner);
    auto const first = clamp_index(
        list,
        ptrdiff_t(
            std::floor((window_low[1] - region_low[1]) / list.item_height)));
    auto const last = clamp_index(
        list,
        ptrdiff_t(
            std::ceil((window_high[1] - region_low[1]) / list.item_height)));
    return std::pair{first, last};
}

ptrdiff_t
index_from_position(lazy_list_data const& list, vector<2, double> position)
{
    return clamp_index(
        list,
        ptrdiff_t(std::floor(
            (position[1] - list.assignment.region.corner[1])
            / list.item_height)));
}

void
update_active_range(
    lazy_list_data& list, std::pair<ptrdiff_t, ptrdiff_t> new_range)
{
    // Clean up any items that are no longer in the active range...
    // If there's no overlap, we can just remove all items.
    if (new_range.first > list.active_range.second
        || new_range.second < list.active_range.first)
    {
        for (ptrdiff_t i = list.active_range.first;
             i <= list.active_range.second;
             ++i)
        {
            // Item 0 is special because we use it for layout.
            if (i != 0)
                list.items.erase(i);
        }
    }
    else
    {
        // Check for items that fell off the beginning.
        for (ptrdiff_t i = list.active_range.first; i < new_range.first; ++i)
        {
            // Item 0 is special because we use it for layout.
            if (i != 0)
                list.items.erase(i);
        }
        // Check for items that fell off the end.
        for (ptrdiff_t i = new_range.second + 1; i <= list.active_range.second;
             ++i)
        {
            // Item 0 is special because we use it for layout.
            if (i != 0)
                list.items.erase(i);
        }
    }
    list.active_range = new_range;
}

void
lazy_list_ui(
    ui_context ctx,
    size_t item_count,
    function_view<void(ui_context, size_t)> const& do_item)
{
    lazy_list_layout_container* container;
    if (get_cached_data(ctx, &container))
        container->data.component = std::make_shared<component_container>();
    auto& data = container->data;

    if (is_refresh_event(ctx))
    {
        data.item_count = item_count;
        add_layout_node(get_layout_traversal(ctx), container);
    }

    scoped_component_container scc(ctx, &data.component);

    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY: {
            if (item_count > 0)
            {
                refresh_component(ctx, data.items[0], [&](ui_context ctx) {
                    do_item(ctx, 0);
                });
            }
            break;
        }

        case RENDER_CATEGORY: {
            auto const [first, last] = compute_visible_range(ctx, data);

            // The active range includes one item before and after the visible
            // range, primarily so those items can handle focus queries.
            update_active_range(
                data,
                std::pair{
                    clamp_index(data, first - 1),
                    clamp_index(data, last + 1)});

            for (ptrdiff_t i = first; i <= last; ++i)
            {
                refresh_lazy_list_item(ctx, data, data.items[i], do_item, i);
                invoke_lazy_list_item(ctx, data.items[i], do_item, i);
            }

            break;
        }

        case REGION_CATEGORY: {
            if (get_event_type(ctx) == MOUSE_HIT_TEST_EVENT
                || get_event_type(ctx) == WHEEL_HIT_TEST_EVENT)
            {
                if (is_mouse_inside_box(
                        ctx, box<2, double>(data.assignment.region)))
                {
                    auto const index
                        = index_from_position(data, get_mouse_position(ctx));
                    refresh_lazy_list_item(
                        ctx, data, data.items[index], do_item, index);
                    invoke_lazy_list_item(
                        ctx, data.items[index], do_item, index);
                }
                break;
            }
            [[fallthrough]];
        }

        default:
            // TODO: Handle targeted events.
            if (scc.is_on_route())
            {
                for (ptrdiff_t i = data.active_range.first;
                     i <= data.active_range.second;
                     ++i)
                {
                    refresh_lazy_list_item(
                        ctx, data, data.items[i], do_item, i);
                    invoke_lazy_list_item(ctx, data.items[i], do_item, i);
                }
            }
            break;
    }
}

} // namespace alia
