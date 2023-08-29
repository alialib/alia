#ifndef ALIA_INDIE_CONTEXT_HPP
#define ALIA_INDIE_CONTEXT_HPP

#include <alia/core/context/interface.hpp>

#include <alia/indie/layout/api.hpp>

namespace alia {
namespace indie {

struct system;
ALIA_DEFINE_TAGGED_TYPE(system_tag, system&)

// TODO: Move this elsewhere.
struct widget;
struct widget_traversal
{
    widget** next_ptr = nullptr;
};

struct traversal
{
    widget_traversal widgets;
    layout_traversal layout;
};
ALIA_DEFINE_TAGGED_TYPE(traversal_tag, traversal&)

struct context_storage : alia::context_storage
{
    system* indie_sys = nullptr;
    indie::traversal* indie_traversal = nullptr;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(context_storage)
};

} // namespace indie

#define ALIA_ADD_UI_CONTEXT_ACCESSORS(storage)                                \
    ALIA_ADD_CORE_CONTEXT_ACCESSORS(storage)                                  \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, indie::system_tag, indie_sys) \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(                                       \
        storage, indie::traversal_tag, indie_traversal)

ALIA_ADD_UI_CONTEXT_ACCESSORS(indie::context_storage)

namespace indie {

using event_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<indie::context_storage>,
    alia::system_tag,
    event_traversal_tag,
    timing_tag,
    indie::system_tag>>;

using dataless_context
    = extend_context_type_t<event_context, indie::traversal_tag>;

using context = extend_context_type_t<dataless_context, data_traversal_tag>;

using vanilla_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<indie::context_storage>,
    alia::system_tag,
    event_traversal_tag,
    timing_tag,
    data_traversal_tag>>;

inline system&
get_system(event_context ctx)
{
    return *ctx.contents_.storage->indie_sys;
}

inline layout_traversal&
get_layout_traversal(dataless_context ctx)
{
    return get<indie::traversal_tag>(ctx).layout;
}

inline widget_traversal&
get_widget_traversal(dataless_context ctx)
{
    return get<indie::traversal_tag>(ctx).widgets;
}

} // namespace indie
} // namespace alia

#endif
