#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/rendering.hpp>

namespace alia { namespace indie {

void
system::operator()(alia::context vanilla_ctx)
{
    render_traversal rt;

    layout_traversal lt;
    scoped_layout_refresh slr;
    scoped_layout_traversal slt;

    if (is_refresh_event(vanilla_ctx))
    {
        rt.next_ptr = &this->render_root;
        slr.begin(this->layout, lt, make_vector<float>(200, 200)); // TODO
    }
    else
    {
        // TODO:
        // slt.begin(this->layout, lt, ...
    }

    auto ctx = extend_context<render_traversal_tag>(
        extend_context<layout_traversal_tag>(
            extend_context<system_tag>(vanilla_ctx, *this), lt),
        rt);

    this->controller(ctx);
    rt.next_ptr = nullptr;
}

void
initialize(
    indie::system& system, std::function<void(indie::context)> controller)
{
    // Initialize the alia::system and hook it up to the indie::system.
    initialize_system(system.alia_system, std::ref(system), nullptr);
    system.controller = std::move(controller);
}

}} // namespace alia::indie
