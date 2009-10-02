#include <alia/artist.hpp>
#include <alia/context.hpp>

namespace alia {

void artist::set_context(context& ctx)
{
    context_ = &ctx;
    surface_ = ctx.surface;
}

}
