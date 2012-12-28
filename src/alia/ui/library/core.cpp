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

    old_style_info_ = ctx.layout.style_info;
    ctx.layout.style_info = &data->style_info;

    ctx_ = &ctx;
}
void scoped_substyle::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ctx.style = old_state_;
        ctx.layout.style_info = old_style_info_;
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
            is_visible(ctx.geometry, box<2,double>(layout_.region()));
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

struct ui_caching_node
{
    ui_caching_node* parent;
    counter_type last_content_change;

    // cached layout info
    counter_type last_layout_update;
    owned_id layout_id;
    layout_node* layout_subtree_head;
    layout_node** layout_subtree_tail;

    // cached rendering info
    counter_type last_render_update;
    owned_id render_id;
    unsigned render_pass_count;
    cached_rendering_content_ptr cached_content;
};

void record_content_change(ui_context& ctx)
{
    ui_caching_node* cacher = ctx.active_cacher;
    while (cacher && cacher->last_content_change != get_refresh_counter(ctx))
    {
        cacher->last_content_change = get_refresh_counter(ctx);
        cacher = cacher->parent;
    }
}

void cached_ui_block::begin(ui_context& ctx, id_interface const& id)
{
    ctx_ = &ctx;

    culling_.begin(ctx);

    ui_caching_node* cacher;
    get_cached_data(ctx, &cacher);

    cacher->parent = ctx.active_cacher;
    ctx.active_cacher = cacher;

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
            cacher->last_layout_update != cacher->last_content_change ||
            !cacher->layout_id.matches(
                combine_ids(ref(*ctx.style.id), ref(id)));
        // If we're going to actually update the layout, record the current
        // value of the layout context's next_ptr, so we'll know where to look
        // for the address of the first node.
        if (is_relevant_)
        {
            layout_next_ptr_ = ctx.layout.next_ptr;
            // Store the ID here because it's only available within this
            // function.
            cacher->layout_id.store(combine_ids(ref(*ctx.style.id), ref(id)));
            // Ensure that the layout content is considered invalid until it's
            // updated.
            cacher->last_layout_update = 0;
        }
        // Otherwise, just splice in the cached subtree.
        else
        {
            *ctx.layout.next_ptr = cacher->layout_subtree_head;
            ctx.layout.next_ptr = cacher->layout_subtree_tail;
        }
        break;

     case RENDER_EVENT:
        // Detect if the block needs to be rerendered.
        if (cacher->last_render_update != cacher->last_content_change ||
            !cacher->render_id.matches(
                combine_ids(ref(*ctx.style.id), ref(id))) ||
            !is_valid(cacher->cached_content))
        {
            cacher->render_pass_count = 0;
            cacher->render_id.store(combine_ids(ref(*ctx.style.id), ref(id)));
            cacher->last_render_update = cacher->last_content_change;
        }
        // render_pass_count counts the number of frames that the block has
        // remained static. If it's 0, then the block has just changed.
        // We don't attempt to record the rendering information on this pass.
        // We only record it on the second pass that the block has remained
        // static. This is for two reasons.
        // 1. Recording the content may be expensive, so if the content is
        //    changing every frame, we probably just want to render it
        //    normally.
        // 2. The surface may do special things the first pass after a change,
        //    such as caching textures on the video card. If we avoid recording
        //    on this pass, we don't have to worry about what happens when
        //    those actions are recorded.
        switch (cacher->render_pass_count)
        {
         case 0:
            is_relevant_ = true;
            break;
         case 1:
            is_relevant_ = true;
            if (!is_valid(cacher->cached_content))
                ctx.surface->create_cached_content(cacher->cached_content);
            cacher->cached_content->start_recording();
            break;
         default:
            cacher->cached_content->playback(*ctx.surface);
            is_relevant_ = false;
            break;
        }
        break;

     default:
        is_relevant_ = true;
        break;
    }
}
void cached_ui_block::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;

        ui_caching_node* cacher = ctx.active_cacher;

        switch (ctx.event->type)
        {
         case REFRESH_EVENT:
            // If the layout was just updated, record the head and tail of the
            // layout subtree so we can splice it into the parent tree on
            // passes where we skip layout.
            if (is_relevant_)
            {
                cacher->layout_subtree_head = *layout_next_ptr_;
                cacher->layout_subtree_tail = ctx.layout.next_ptr;
                cacher->last_layout_update = cacher->last_content_change;
            }
            break;

         case RENDER_EVENT:
            if (is_relevant_)
            {
                switch (cacher->render_pass_count)
                {
                 case 1:
                    cacher->cached_content->stop_recording();
                 case 0:
                    ++cacher->render_pass_count;
		}
	    }
        }

        culling_.end();

        ctx.active_cacher = cacher->parent;
        ctx_ = 0;
    }
}

}
