#ifndef ALIA_UI_UTILITIES_DRAGGING_HPP
#define ALIA_UI_UTILITIES_DRAGGING_HPP

#include <alia/ui/internals.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/styling.hpp>

namespace alia {

struct draggable_style
{
    float outline_width, outline_margin;
    float outline_dashing;
    rgba8 outline_color;
    box_corner_sizes corner_radii;
    rgba8 fill_color;
    float fill_size;
};

template<class Index>
struct draggable_collection_data
{
    keyed_data<draggable_style> style;
    // If an object is being dragged, this is its ID.
    // Otherwise, this is 0.
    widget_id dragging_id;
    // While dragging, this stores the vector from the upper-left corner of
    // the dragged object's region to the mouse cursor.
    // The vector is relative to the size of the dragged object
    // (i.e., (1, 1) represents the bottom-right corner of the object).
    vector<2,double> drag_delta;
    // While dragging, this is used to render the location where the dragged
    // object will be dropped.
    caching_renderer_data drop_location_renderer;

    draggable_collection_data() : dragging_id(0) {}
};

template<class Index>
struct draggable_object_index_query : ui_event
{
    draggable_object_index_query(widget_id target)
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
      , target(target)
    {}
    widget_id target;
    optional<Index> index;
};

// When a drag operation results in an object needing to be moved within a
// collection, a move_request is generated. It represents a request to
// move the object at index `from` to index `to`. (All objects between
// should be rotated towards 'from'.)
template<class Index>
struct draggable_move_request
{
    draggable_move_request() {}
    draggable_move_request(Index from, Index to) : from(from), to(to) {}
    Index from, to;
};

void draw_drop_location(dataless_ui_context& ctx, layout_box const& region,
    caching_renderer_data& renderer_data, draggable_style const& style);

void refresh_draggable_style(dataless_ui_context& ctx,
    keyed_data<draggable_style>& style_data);

vector<2,double>
calculate_relative_drag_delta(
    dataless_ui_context& ctx, layout_box const& region);

vector<2,double>
evaluate_relative_drag_delta(
    dataless_ui_context& ctx, vector<2,double> const& size,
    vector<2,double> const& relative_delta);

template<class Index>
struct draggable_collection : noncopyable
{
    draggable_collection() : ctx_(0) {}
    draggable_collection(ui_context& ctx) { begin(ctx); }
    ~draggable_collection() { end(); }

    void begin(ui_context& ctx)
    {
        ctx_ = &ctx;
        get_data(ctx, &data_);
        refresh_draggable_style(ctx, data_->style);
    }
    void end() {}

    // The caller should check get_move_request() each frame after executing
    // all objects within the collection.
    // If it returns a valid move_request, the caller should execute the
    // corresponding move.
    optional<draggable_move_request<Index> > const& get_move_request() const
    { return move_request_; }

 private:
    template<class Index>
    friend struct draggable_object;
    ui_context* ctx_;
    draggable_collection_data<Index>* data_;
    optional<draggable_move_request<Index> > move_request_;
};

template<class Index>
struct draggable_object : noncopyable
{
    draggable_object() : collection_(0) {}
    draggable_object(draggable_collection<Index>& collection, widget_id id,
        Index const& index, layout const& layout_spec = default_layout)
    { begin(collection, id, index, layout_spec); }
    ~draggable_object() { end(); }

    void begin(draggable_collection<Index>& collection, widget_id id,
        Index const& index, layout const& layout_spec = default_layout)
    {
        collection_ = &collection;
        id_ = id;

        ui_context& ctx = *collection.ctx_;
        draggable_collection_data<Index>& data = *collection.data_;

        dragging_ = data.dragging_id == id;

        column_.begin(ctx, layout_spec);

        if (dragging_ && !is_refresh_pass(ctx))
        {
            draggable_style const& style = get(data.style);

            if (is_render_pass(ctx))
            {
                draw_drop_location(ctx, column_.region(),
                    data.drop_location_renderer, style);
            }

            // Only transform render events because the hit testing the
            // overlay will just get in the way.
            if (ctx.event->type == RENDER_EVENT ||
                ctx.event->type == OVERLAY_RENDER_EVENT)
            {
                overlay_.begin(ctx, id);
            }

            if (is_render_pass(ctx))
            {
                clip_reset_.begin(get_geometry_context(ctx));

                vector<2,double> content_size(column_.region().size);

                // If the style specifies a fill color, then fill the drag
                // area.
                if (style.fill_color.a != 0)
                {
                    // TODO: Support rounded corners.
                    vector<2,double> fill_size =
                        style.fill_size != 0
                      ? make_vector<double>(style.fill_size, style.fill_size)
                      : content_size;
                    ctx.surface->draw_filled_box(
                        style.fill_color,
                        box<2,double>(
                            evaluate_relative_drag_delta(
                                ctx, fill_size, data.drag_delta),
                            fill_size));

                    // Hide the render event so that the actual content doesn't
                    // render.
                    ctx.event->category = NO_CATEGORY;
                    ctx.event->type = NO_EVENT;
                }
                else
                {
                    // Otherwise, set up a transformation matrix so that the
                    // content itself renders at the dragged position.
                    transform_.begin(get_geometry_context(ctx));
                    transform_.set(translation_matrix(
                        evaluate_relative_drag_delta(
                            ctx, content_size, data.drag_delta)));
                }
            }
        }

        if (detect_drag(ctx, id, LEFT_BUTTON))
        {
            if (!dragging_)
            {
                set_active_overlay(ctx, id);
                data.drag_delta =
                    calculate_relative_drag_delta(ctx, column_.region());
                data.dragging_id = id;
            }
            if (!is_region_hot(ctx, id))
            {
                routable_widget_id target = ctx.system->input.hot_id;
                if (is_valid(target))
                {
                    draggable_object_index_query<Index> query(target.id);
                    issue_targeted_event(*ctx.system, query, target);
                    if (query.index)
                    {
                        collection.move_request_ =
                            draggable_move_request<Index>(
                                index, get(query.index));
                    }
                }
            }
        }
        if (detect_mouse_release(ctx, id, LEFT_BUTTON))
        {
            clear_active_overlay(ctx);
            data.dragging_id = 0;
        }

        // Process index query events.
        if (detect_event(ctx, CUSTOM_EVENT))
        {
            draggable_object_index_query<Index>* query =
                dynamic_cast<draggable_object_index_query<Index> *>(ctx.event);
            if (query && query->target == id)
                query->index = index;
        }
    }

    void end()
    {
        if (collection_)
        {
            dataless_ui_context& ctx = *collection_->ctx_;
            draggable_collection_data<Index>& data = *collection_->data_;

            if (dragging_)
            {
                override_mouse_cursor(ctx, id_, FOUR_WAY_ARROW_CURSOR);

                transform_.end();
                clip_reset_.end();
                overlay_.end();
            }
            // If some other object in the collection is being dragged, make
            // sure the ID for this object covers the whole region so that the
            // whole region acts as a drop receptor.
            else if (data.dragging_id != 0)
                do_box_region(ctx, id_, column_.region());

            column_.end();

            collection_ = 0;
        }
    }

    bool is_dragging() const { return dragging_; }

 private:
    draggable_collection<Index>* collection_;
    widget_id id_;
    column_layout column_;
    // Is this object being dragged?
    bool dragging_;
    // The following are used to implement the arbitrary movement of the
    // contents while dragging.
    overlay_event_transformer overlay_;
    scoped_transformation transform_;
    scoped_clip_region_reset clip_reset_;
};

}

#endif
