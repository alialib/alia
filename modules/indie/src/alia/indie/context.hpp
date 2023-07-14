#ifndef ALIA_INDIE_CONTEXT_HPP
#define ALIA_INDIE_CONTEXT_HPP

#include <alia/core/context/interface.hpp>

namespace alia { namespace indie {

// struct element_object;
// ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<element_object>&)

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

typedef extend_context_type_t<alia::context, system_tag> context;

}} // namespace alia::indie

#endif
