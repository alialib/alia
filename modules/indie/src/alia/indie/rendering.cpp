#include <alia/indie/rendering.hpp>
#include <alia/indie/system/object.hpp>

namespace alia { namespace indie {

void
render(indie::system& system, SkCanvas& canvas)
{
    if (system.render_root)
    {
        system.render_root->render(canvas);
    }
}

}} // namespace alia::indie
