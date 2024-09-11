#ifndef ALIA_UI_CONTEXT_HPP
#define ALIA_UI_CONTEXT_HPP

#include <alia/core/context/interface.hpp>
#include <alia/ui/layout/geometry.hpp>
#include <alia/ui/layout/traversal.hpp>

namespace alia {

struct ui_system;
ALIA_DEFINE_TAGGED_TYPE(ui_system_tag, ui_system&)

struct widget;
struct widget_container;

struct ui_traversal
{
    layout_traversal layout;
};
ALIA_DEFINE_TAGGED_TYPE(ui_traversal_tag, ui_traversal&)

struct ui_context_storage : core_context_storage
{
    ui_system* ui_sys = nullptr;
    alia::ui_traversal* ui_traversal = nullptr;

    ALIA_IMPLEMENT_STORAGE_OBJECT_ACCESSORS(ui_context_storage)
};

} // namespace alia

#define ALIA_ADD_UI_CONTEXT_ACCESSORS(storage)                                \
    ALIA_ADD_CORE_CONTEXT_ACCESSORS(storage)                                  \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, alia::ui_system_tag, ui_sys)  \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(                                       \
        storage, alia::ui_traversal_tag, ui_traversal)

ALIA_ADD_UI_CONTEXT_ACCESSORS(alia::ui_context_storage)

namespace alia {

using dataless_ui_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<ui_context_storage>,
    core_system_tag,
    event_traversal_tag,
    timing_tag,
    ui_system_tag,
    ui_traversal_tag>>;

using ui_context
    = extend_context_type_t<dataless_ui_context, data_traversal_tag>;

// A vanilla_ui_context has the storage of a UI context but with only the
// components from a core context.
using vanilla_ui_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<ui_context_storage>,
    core_system_tag,
    event_traversal_tag,
    timing_tag,
    data_traversal_tag>>;

inline ui_system&
get_system(dataless_ui_context ctx)
{
    return *ctx.contents_.storage->ui_sys;
}

inline layout_traversal&
get_layout_traversal(dataless_ui_context ctx)
{
    return get<ui_traversal_tag>(ctx).layout;
}

inline geometry_context&
get_geometry_context(dataless_ui_context ctx)
{
    return *get<ui_traversal_tag>(ctx).layout.geometry;
}

} // namespace alia

#endif
