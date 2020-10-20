#ifndef ALIA_HTML_CONTEXT_HPP
#define ALIA_HTML_CONTEXT_HPP

#include <alia.hpp>

namespace alia {
namespace html {

struct element_object;
ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<element_object>&)

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

typedef extend_context_type_t<alia::context, tree_traversal_tag, system_tag>
    context;

} // namespace html
} // namespace alia

#endif
