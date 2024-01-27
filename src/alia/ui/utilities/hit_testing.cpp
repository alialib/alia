#include <alia/indie/utilities/hit_testing.hpp>

namespace alia { namespace indie {

void
hit_test_box(
    hit_test_base& test,
    vector<2, double> const& point,
    internal_element_ref element,
    layout_box const& box,
    mouse_cursor cursor)
{
    if (is_inside(box, vector<2, float>(point)))
    {
        if (test.type == indie::hit_test_type::MOUSE)
        {
            static_cast<indie::mouse_hit_test&>(test).result
                = indie::mouse_hit_test_result{
                    externalize(element), cursor, box, ""};
        }
    }
}

}} // namespace alia::indie
