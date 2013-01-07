#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

// This file implements the core components from the UI API.

namespace alia {

// SUBSTYLES

struct substyle_data
{
    owned_id key;
    style_search_path path;
    primary_style_properties properties;
    style_state state;
    layout_style_info style_info;
    value_id_by_reference<local_id> id;
    local_identity identity;
};

void scoped_substyle::begin(
    ui_context& ctx, getter<string> const& substyle_name, widget_state state)
{
    substyle_data* data;
    get_cached_data(ctx, &data);

    if (ctx.event->type == REFRESH_EVENT &&
        !data->key.matches(combine_ids(ref(*ctx.style.id),
            combine_ids(ref(substyle_name.id()), make_id(state)))))
    {
        inc_version(data->identity);

        style_tree const* substyle =
            find_substyle(ctx.style.path, get(substyle_name), state);
        if (substyle)
        {
            data->path.tree = substyle;
            data->path.rest = ctx.style.path;
        }
        else
            data->path = *ctx.style.path;

        data->state.path = &data->path;

        read_primary_style_properties(
            *ctx.system, &data->properties, data->state.path);
        data->state.properties = &data->properties;

        data->state.theme = ctx.style.theme;

        data->state.id = &data->id;

        read_layout_style_info(&data->style_info, data->properties.font,
            data->state.path);

        data->key.store(combine_ids(ref(*ctx.style.id),
            combine_ids(ref(substyle_name.id()), make_id(state))));

        data->id = get_id(data->identity);
    }

    old_state_ = ctx.style;
    ctx.style = data->state;

    old_style_info_ = ctx.layout->style_info;
    ctx.layout->style_info = &data->style_info;

    ctx_ = &ctx;
}
void scoped_substyle::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ctx.style = old_state_;
        ctx.layout->style_info = old_style_info_;
        ctx_ = 0;
    }
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

}
