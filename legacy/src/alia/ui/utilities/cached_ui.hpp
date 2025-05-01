#ifndef ALIA_UI_UTILITIES_CACHED_UI_HPP
#define ALIA_UI_UTILITIES_CACHED_UI_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/layout/internals.hpp>
#include <alia/ui/utilities/culling.hpp>

// TODO: Integrate this with pure components in the core library.
// TODO: Rewrite this so the implementation doesn't have to be exposed.

namespace alia {

struct layout_node;
struct ui_caching_node;

template<class Layout>
struct scoped_cached_ui_block
{
    scoped_cached_ui_block()
    {
    }
    scoped_cached_ui_block(
        ui_context ctx,
        id_interface const& id,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, id, layout_spec);
    }
    ~scoped_cached_ui_block()
    {
        end();
    }
    void
    begin(
        ui_context ctx,
        id_interface const& id,
        layout const& layout_spec = default_layout);
    void
    end();
    bool
    is_relevant() const
    {
        return is_relevant_;
    }

 private:
    optional_context<ui_context> ctx_;
    ui_caching_node* cacher_;
    scoped_culling_block<Layout> culling_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

struct ui_caching_node
{
    // ui_caching_node* parent;

    // cached layout info
    bool layout_valid;
    captured_id layout_id;
    layout_node* layout_subtree_head;
    layout_node** layout_subtree_tail;
};

template<class Layout>
void
scoped_cached_ui_block<Layout>::begin(
    ui_context ctx, id_interface const& id, layout const& layout_spec)
{
    ctx_.reset(ctx);

    culling_.begin(ctx, layout_spec);

    get_cached_data(ctx, &cacher_);
    ui_caching_node& cacher = *cacher_;

    // cacher.parent = ctx.active_cacher;
    // ctx.active_cacher = &cacher;

    // Caching content in the middle of a validation block is not
    // currently supported. assert(!ctx.validation.detection &&
    // !ctx.validation.reporting);

    // Before doing anything else, see if the content can be culled by
    // the culling block's criteria.
    if (!culling_.is_relevant())
    {
        is_relevant_ = false;
        return;
    }

    if (get_event_type(ctx) == REFRESH_EVENT)
    {
        // Detect if there are changes that require the block to be
        // traversed this pass.
        is_relevant_ = !cacher.layout_valid || !cacher.layout_id.matches(id);
        // If we're going to actually update the layout, record the
        // current value of the layout context's next_ptr, so we'll
        // know where to look for the address of the first node.
        if (is_relevant_)
        {
            layout_next_ptr_ = get_layout_traversal(ctx).next_ptr;
            // Store the ID here because it's only available within
            // this function.
            cacher.layout_id.capture(id);
            // Need to mark it valid here because it could be
            // invalidated by something inside the block. (Is this
            // dangerous?)
            cacher.layout_valid = true;
        }
        // Otherwise, just splice in the cached subtree.
        else
        {
            *get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_head;
            get_layout_traversal(ctx).next_ptr = cacher.layout_subtree_tail;
        }
    }
    else
    {
        is_relevant_ = true;
    }
}

template<class Layout>
void
scoped_cached_ui_block<Layout>::end()
{
    if (ctx_)
    {
        ui_context ctx = *ctx_;
        ui_caching_node& cacher = *cacher_;

        // If the layout was just updated, record the head and tail of
        // the layout subtree so we can splice it into the parent tree
        // on passes where we skip layout.
        if (is_refresh_pass(ctx) && is_relevant_)
        {
            cacher.layout_subtree_head = *layout_next_ptr_;
            cacher.layout_subtree_tail = get_layout_traversal(ctx).next_ptr;
        }

        switch (get_event_type(ctx))
        {
            case REFRESH_EVENT:
            case RENDER_EVENT:
            case MOUSE_HIT_TEST_EVENT:
            case WHEEL_HIT_TEST_EVENT:
                break;
            default:
                // Any other event that makes it into the block could
                // potentially cause a state change, so record a
                // change.
                cacher.layout_valid = false;
        }

        culling_.end();

        // ctx.active_cacher = cacher.parent;

        ctx_.reset();
    }
}

template<class Layout, class Content>
void
cached_ui_block(
    ui_context& ctx,
    id_interface const& id,
    layout const& layout_spec,
    Content&& content)
{
    scoped_cached_ui_block<Layout> block(ctx, id, layout_spec);
    {
        event_dependent_if_block if_block(
            get_data_traversal(ctx), block.is_relevant());
        if (block.is_relevant())
        {
            content();
        }
    }
}

} // namespace alia

#endif
