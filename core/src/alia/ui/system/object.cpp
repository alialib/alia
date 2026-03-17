#include <alia/ui/system/object.h>

#include <alia/abi/base/geometry.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>

namespace alia {

void
invoke_controller(ui_system& system, context& ctx)
{
    system.controller(ctx);
}

} // namespace alia
