#ifndef ALIA_UI_LIBRARY_COLLAPSIBLE_HPP
#define ALIA_UI_LIBRARY_COLLAPSIBLE_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

class scoped_collapsible_content : noncopyable
{
 public:
    scoped_collapsible_content()
    {
    }
    ~scoped_collapsible_content()
    {
        end();
    }

    scoped_collapsible_content(
        ui_context& ctx,
        bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, expanded, transition, offset_factor, layout_spec);
    }

    scoped_collapsible_content(
        ui_context& ctx,
        float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, expansion, offset_factor, layout_spec);
    }

    void
    begin(
        ui_context& ctx,
        bool expanded,
        animated_transition const& transition = default_transition,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void
    begin(
        ui_context& ctx,
        float expansion,
        double const offset_factor = 1.,
        layout const& layout_spec = default_layout);

    void
    end();

    bool
    do_content() const
    {
        return do_content_;
    }

 private:
    ui_context* ctx_;
    scoped_layout_container container_;
    scoped_clip_region clipper_;
    scoped_transformation transform_;
    column_layout column_;
    bool do_content_;
};

template<class Content>
void
collapsible_content(
    ui_context ctx, readable<bool> show_content, Content&& content)
{
    scoped_collapsible_content collapsible(
        ctx, condition_is_true(show_content));
    if_(ctx, collapsible.do_content(), std::forward<Content>(content));
}

template<class Content>
void
collapsible_content(
    ui_context ctx,
    readable<bool> show_content,
    layout const& layout_spec,
    Content&& content)
{
    scoped_collapsible_content collapsible(
        ctx,
        condition_is_true(show_content),
        default_transition,
        1.,
        layout_spec);
    if_(ctx, collapsible.do_content(), std::forward<Content>(content));
}

} // namespace alia

#endif
