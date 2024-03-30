#include <alia/ui/text/components.hpp>

#include <alia/ui/text/shaping.hpp>
#include <alia/ui/text/widgets.hpp>

namespace alia {

// TODO: Move elsewhere.
inline SkColor
as_skia_color(rgba8 color)
{
    return SkColorSetARGB(color.a, color.r, color.g, color.b);
}

inline SkColor
as_skia_color(rgb8 color)
{
    return SkColorSetARGB(0xff, color.r, color.g, color.b);
}

void
do_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec)
{
    text_node* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        node_ptr->sys_ = &get_system(ctx);
        // TODO: This isn't necessarily readable right now.
        node_ptr->text_ = read_signal(text);
        // TODO: This also isn't necessarily readable right now.
        // TODO: Need to track changes in this.
        node_ptr->font_ = &get_font(
            read_signal(style).font_name, read_signal(style).font_size);
        node_ptr->color_ = as_skia_color(read_signal(style).color);
    }

    auto& node = *node_ptr;

    if (is_refresh_event(ctx))
    {
        if (update_layout_cacher(
                get_layout_traversal(ctx),
                node.cacher,
                layout_spec,
                TOP | LEFT | PADDED))
        {
            // Since this container isn't active yet, it didn't get marked
            // as needing recalculation, so we need to do that manually
            // here.
            node.last_content_change
                = get_layout_traversal(ctx).refresh_counter;
        }

        refresh_signal_view(
            node.text_id_,
            text,
            [&](std::string const& new_value) { node.text_ = new_value; },
            [&] { node.text_.clear(); },
            [&] {
                record_layout_change(get_layout_traversal(ctx));
                node.shape_ = std::nullopt;
                node.last_content_change
                    = get_layout_traversal(ctx).refresh_counter;
            });

        add_layout_node(get<ui_traversal_tag>(ctx).layout, &node);
    }
}

void
do_wrapped_text(
    ui_context ctx,
    readable<text_style> style,
    readable<std::string> text,
    layout const& layout_spec)
{
    wrapped_text_node* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        node_ptr->sys_ = &get_system(ctx);

        // TODO: This isn't necessarily readable right now.
        node_ptr->text_ = read_signal(text);
        // TODO: This also isn't necessarily readable right now.
        // TODO: Need to track changes in this.
        node_ptr->font_ = &get_font(
            read_signal(style).font_name, read_signal(style).font_size);
    }

    auto& node = *node_ptr;

    if (is_refresh_event(ctx))
    {
        if (update_layout_cacher(
                get_layout_traversal(ctx),
                node.cacher,
                layout_spec,
                TOP | LEFT | PADDED))
        {
            // Since this container isn't active yet, it didn't get marked
            // as needing recalculation, so we need to do that manually
            // here.
            node.last_content_change
                = get_layout_traversal(ctx).refresh_counter;
        }

        refresh_signal_view(
            node.text_id_,
            text,
            [&](std::string const& new_value) { node.text_ = new_value; },
            [&] { node.text_.clear(); },
            [&] {
                record_layout_change(get_layout_traversal(ctx));
                node.shape_width_ = 0;
                node.shape_ = ShapeResult();
                node.last_content_change
                    = get_layout_traversal(ctx).refresh_counter;
            });

        add_layout_node(get<ui_traversal_tag>(ctx).layout, &node);
    }
}

} // namespace alia
