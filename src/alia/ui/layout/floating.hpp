#ifndef ALIA_UI_LAYOUT_FLOATING_LAYOUT_HPP
#define ALIA_UI_LAYOUT_FLOATING_LAYOUT_HPP

#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

struct data_traversal;
struct layout_traversal;
struct layout_container;
struct layout_node;

// A floating_layout detaches its contents from the parent context.
// It should only have one child (generally another container).
// This child becomes the root of an independent layout tree.
// The caller may specify the minimum and maximum size of the child.
// Note that the child is simply placed at (0, 0).
// It's assumed that the caller will take care of positioning it properly.
struct floating_layout_data;
struct floating_layout
{
    floating_layout() : traversal_(0)
    {
    }

    template<class Context>
    floating_layout(
        Context& ctx,
        layout_vector const& min_size = make_layout_vector(-1, -1),
        layout_vector const& max_size = make_layout_vector(-1, -1))
    {
        begin(ctx, min_size, max_size);
    }

    ~floating_layout()
    {
        end();
    }

    template<class Context>
    void
    begin(
        Context& ctx,
        layout_vector const& min_size = make_layout_vector(-1, -1),
        layout_vector const& max_size = make_layout_vector(-1, -1))
    {
        concrete_begin(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            min_size,
            max_size);
    }

    void
    end();

    layout_vector
    size() const;

    layout_box
    region() const
    {
        return layout_box(make_layout_vector(0, 0), size());
    }

 private:
    void
    concrete_begin(
        layout_traversal& traversal,
        data_traversal& data,
        layout_vector const& min_size,
        layout_vector const& max_size);

    layout_traversal* traversal_;
    layout_container* old_container_;
    layout_node** old_next_ptr_;
    layout_node* floating_root_;
    floating_layout_data* data_;
    scoped_clip_region_reset clipping_reset_;
    layout_vector min_size_, max_size_;
};

} // namespace alia

#endif
