#ifndef ALIA_HTML_CONTEXT_HPP
#define ALIA_HTML_CONTEXT_HPP

#include <alia/core.hpp>

namespace alia { namespace html {

struct element_object;
ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<element_object>&)

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

struct context_storage : core_context_storage
{
    system* html_sys = nullptr;
    tree_traversal<element_object>* html_tree_traversal = nullptr;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(context_storage)
};

}} // namespace alia::html

#define ALIA_ADD_HTML_CONTEXT_ACCESSORS(storage)                              \
    ALIA_ADD_CORE_CONTEXT_ACCESSORS(storage)                                  \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(                                       \
        storage, alia::html::system_tag, html_sys)                            \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(                                       \
        storage, alia::html::tree_traversal_tag, html_tree_traversal)

ALIA_ADD_HTML_CONTEXT_ACCESSORS(alia::html::context_storage)

namespace alia { namespace html {

using dataless_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<html::context_storage>,
    core_system_tag,
    event_traversal_tag,
    timing_tag,
    html::system_tag,
    html::tree_traversal_tag>>;

using context = extend_context_type_t<dataless_context, data_traversal_tag>;

// A vanilla_ui_context has the storage of a UI context but with only the
// components from a core context.
using vanilla_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<html::context_storage>,
    core_system_tag,
    event_traversal_tag,
    timing_tag,
    data_traversal_tag>>;

inline system&
get_system(dataless_context ctx)
{
    return *ctx.contents_.storage->html_sys;
}

}} // namespace alia::html

#endif
