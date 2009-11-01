#ifndef ALIA_LAYOUT_LOGIC_HPP
#define ALIA_LAYOUT_LOGIC_HPP

#include <alia/forward.hpp>
#include <boost/noncopyable.hpp>

namespace alia {

class layout_logic : boost::noncopyable
{
 public:
    // Nodes call this during layout pass 0 to request space along the
    // horizontal axis.
    // minimum_size is the minimum amount of space required by the node.
    // proportion is the proportion of extra space that the node will get.
    // Note that some forms of layout may ignore the proportion argument.
    virtual void request_horizontal_space_for_node(int minimum_width,
        float proportion) = 0;

    // Nodes call this during layout pass 1 to determine their widths.
    virtual int get_width_for_node(int minimum_width, float proportion) = 0;

    // This is analogous to the above, but for the vertical direction, which
    // is done in layout pass 1.
    // For the vertical direction, nodes can be aligned according to their
    // baseline, if they have one.  Doing this ensures that text in the same
    // row will line up, even if there are different fonts or widgets used.
    // In order for this to work, nodes must specify a minimum ascent and
    // descent (i.e, the size required above and below the baseline,
    // respectively).
    // A node can set minimum_ascent and minimum_descent to zero to indicate
    // that it doesn't care where the baseline is.  Generally, if they are
    // non-zero, then their sum should equal minimum_height, which will ensure
    // that the baseline is in exactly the right place.
    virtual void request_vertical_space_for_node(int minimum_height,
        int minimum_ascent, int minimum_descent, float proportion) = 0;

    // Nodes call this during layout pass 2 to get the region assigned to them.
    // Note that the width may be larger than what was returned by
    // get_width_for_node() in pass 1.  This happens when a vertical scrollbar
    // is not needed (which isn't known until pass 2), and its width is
    // reclaimed by the associated scrollable_region.
    // baseline_y is set to the y-coordinate of the baseline within the
    // region (i.e, the distance from the top of the region to the baseline).
    // Some forms of layout ignore the baseline and always return a baseline_y
    // of 0.  The layout utility functions check for and handle this.
    virtual void get_region_for_node(box2i* region, int* baseline_y,
        vector2i const& minimum_size, float proportion) = 0;
};

}

#endif
