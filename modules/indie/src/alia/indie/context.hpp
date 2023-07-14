#ifndef ALIA_INDIE_CONTEXT_HPP
#define ALIA_INDIE_CONTEXT_HPP

#include <alia/core/context/interface.hpp>

namespace alia {
namespace indie {

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

struct render_traversal;
ALIA_DEFINE_TAGGED_TYPE(render_traversal_tag, render_traversal&)

// TODO: Fix
} // namespace indie
struct layout_traversal;
namespace indie {
ALIA_DEFINE_TAGGED_TYPE(layout_traversal_tag, layout_traversal&)

typedef extend_context_type_t<
    alia::context,
    system_tag,
    layout_traversal_tag,
    render_traversal_tag>
    context;

} // namespace indie
} // namespace alia

#endif
