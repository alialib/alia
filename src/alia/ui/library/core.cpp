#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>

// This file implements the core components from the UI API.

namespace alia {

// STYLING

void scoped_style::begin(ui_context& ctx, style_state const& style,
    layout_style_info const* info)
{
    ctx_ = &ctx;

    old_state_ = ctx.style;
    ctx.style = style;

    old_style_info_ = ctx.layout->style_info;
    ctx.layout->style_info = info;
}

void scoped_style::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ctx.style = old_state_;
        ctx.layout->style_info = old_style_info_;
        ctx_ = 0;
    }
}

void scoped_substyle::begin(
    ui_context& ctx, getter<string> const& substyle_name, widget_state state,
    scoped_substyle_flag_set flags)
{
    keyed_data<substyle_data>* data;
    if (get_cached_data(ctx, &data) || is_refresh_pass(ctx))
    {
        refresh_keyed_data(*data, combine_ids(ref(*ctx.style.id),
            combine_ids(ref(substyle_name.id()), make_id(state))));
    }
    if (!is_valid(*data))
    {
        update_substyle_data(ctx, data->value, substyle_name, state,
            (flags & SCOPED_SUBSTYLE_NO_PATH_SEPARATOR) ?
                ADD_SUBSTYLE_NO_PATH_SEPARATOR : NO_FLAGS);
        mark_valid(*data);
    }
    scoping_.begin(ctx, get(*data).state, &get(*data).style_info);
}
void scoped_substyle::end()
{
    scoping_.end();
}

// CULLING

void culling_block::begin(ui_context& ctx, layout const& layout_spec)
{
    ctx_ = &ctx;
    srr_.begin(ctx.routing);
    layout_.begin(ctx);
    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        is_relevant_ = true;
        break;
     case RENDER_CATEGORY:
        is_relevant_ =
            is_visible(get_geometry_context(ctx),
                box<2,double>(layout_.region()));
        break;
     case REGION_CATEGORY:
        if (ctx.event->type == MOUSE_HIT_TEST_EVENT ||
            ctx.event->type == WHEEL_HIT_TEST_EVENT)
        {
            is_relevant_ =
                mouse_is_inside_box(ctx, box<2,double>(layout_.region()));
            break;
        }
        // Other region events fall through.
     default:
        is_relevant_ = srr_.is_relevant();
    }
}
void culling_block::end()
{
    if (ctx_)
    {
        layout_.end();
        srr_.end();
        ctx_ = 0;
    }
}

// UI CACHING

void cached_ui_block::begin(ui_context& ctx, id_interface const& id)
{
    ctx_ = &ctx;

    culling_.begin(ctx);

    get_cached_data(ctx, &cacher_);
    ui_caching_node& cacher = *cacher_;

    cacher.parent = ctx.active_cacher;
    ctx.active_cacher = &cacher;

    // Before doing anything else, see if the content can be culled by the
    // culling block's criteria.
    if (!culling_.is_relevant())
    {
        is_relevant_ = false;
        return;
    }

    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        // Detect if there are changes that require the block to be traversed
        // this pass.
        is_relevant_ =
            !cacher.layout_valid ||
            !cacher.layout_id.matches(
                combine_ids(ref(*ctx.style.id), ref(id)));
        // If we're going to actually update the layout, record the current
        // value of the layout context's next_ptr, so we'll know where to look
        // for the address of the first node.
        if (is_relevant_)
        {
            layout_next_ptr_ = get_layout_traversal(ctx).next_ptr;
            // Store the ID here because it's only available within this
            // function.
            cacher.layout_id.store(combine_ids(ref(*ctx.style.id), ref(id)));
            // Need to mark it valid here because it could be invalidated
            // by something inside the block.
            // (Is this dangerous?)
            cacher.layout_valid = true;
        }
        // Otherwise, just splice in the cached subtree.
        else
        {
            *get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_head;
            get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_tail;
        }
        break;

     case RENDER_EVENT:
        is_relevant_ = true;
        break;

     default:
        is_relevant_ = true;
        // Any other event that makes it into the block could potentially
        // cause a state change, so record a change.
        ++cacher.layout_valid = false;
        break;
    }
}
void cached_ui_block::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ui_caching_node& cacher = *cacher_;

        // If the layout was just updated, record the head and tail of the
        // layout subtree so we can splice it into the parent tree on passes
        // where we skip layout.
        if (is_refresh_pass(ctx) && is_relevant_)
        {
            cacher.layout_subtree_head = *layout_next_ptr_;
            cacher.layout_subtree_tail = get_layout_traversal(ctx).next_ptr;
        }

        culling_.end();

        ctx.active_cacher = cacher.parent;

        ctx_ = 0;
    }
}

// LOCATIONS

void mark_location(ui_context& ctx, id_interface const& id,
    layout_vector const& position)
{
    widget_id region_id = get_widget_id(ctx);
    layout_box region;
    do_spacer(ctx, &region, layout(size(0, 0, PIXELS), UNPADDED));
    do_box_region(ctx, region_id, region);
    if (detect_event(ctx, RESOLVE_LOCATION_EVENT))
    {
        resolve_location_event& event = get_event<resolve_location_event>(ctx);
        if (event.id.matches(id))
        {
            event.routable_id = make_routable_widget_id(ctx, region_id);
            event.acknowledged = true;
        }
    }
}

void jump_to_location(ui_context& ctx, id_interface const& id)
{
    routable_widget_id routable_id;
    {
        owned_id owner;
        owner.store(id);
        resolve_location_event event(owner);
        issue_event(*ctx.system, event);
        if (!event.acknowledged)
            return;
        routable_id = event.routable_id;
    }
    {
        jump_to_widget_event event(routable_id.id);
        issue_targeted_event(*ctx.system, event, routable_id);
    }
}

}
