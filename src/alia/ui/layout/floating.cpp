#include <alia/ui/layout/floating.hpp>

#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/traversal.hpp>
#include <alia/ui/layout/utilities.hpp>

namespace alia {

struct floating_layout_data
{
    data_graph measurement_cache, placement_cache;
    layout_vector size;
    floating_layout_data()
    {
    }
};

void
floating_layout::concrete_begin(
    layout_traversal& traversal,
    data_traversal& data,
    layout_vector const& min_size,
    layout_vector const& max_size)
{
    get_cached_data(data, &data_);

    traversal_ = &traversal;

    if (traversal.is_refresh_pass)
    {
        old_container_ = traversal.active_container;
        old_next_ptr_ = traversal.next_ptr;

        traversal.active_container = 0;
        traversal.next_ptr = &floating_root_;

        min_size_ = min_size;
        max_size_ = max_size;
    }

    // if (traversal.geometry)
    //     clipping_reset_.begin(*traversal.geometry);
}

void
floating_layout::end()
{
    if (traversal_)
    {
        layout_traversal& traversal = *traversal_;

        if (traversal.is_refresh_pass)
        {
            traversal_->active_container = old_container_;
            traversal_->next_ptr = old_next_ptr_;

            layout_vector measured_size = get_minimum_size(floating_root_);
            for (unsigned i = 0; i != 2; ++i)
            {
                data_->size[i] = measured_size[i];
                if (min_size_[i] >= 0 && data_->size[i] < min_size_[i])
                    data_->size[i] = min_size_[i];
                if (max_size_[i] >= 0 && data_->size[i] > max_size_[i])
                    data_->size[i] = max_size_[i];
            }
            resolve_layout(
                floating_root_,
                make_box(make_layout_vector(0, 0), data_->size));
        }

        // TODO
        // clipping_reset_.end();

        traversal_ = 0;
    }
}

layout_vector
floating_layout::size() const
{
    return data_->size;
}

} // namespace alia
