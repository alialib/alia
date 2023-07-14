#include <alia/indie/system/object.hpp>

namespace alia { namespace indie {

void
system::operator()(alia::context vanilla_ctx)
{
    auto ctx = extend_context<system_tag>(vanilla_ctx, *this);
    this->controller(ctx);
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
