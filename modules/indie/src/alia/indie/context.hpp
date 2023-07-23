#ifndef ALIA_INDIE_CONTEXT_HPP
#define ALIA_INDIE_CONTEXT_HPP

#include <alia/core/context/interface.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/widget.hpp>

namespace alia {
namespace indie {

struct traversal
{
    system* indie_sys = nullptr;

    widget_traversal widgets;

    layout_traversal layout;
};

ALIA_DEFINE_TAGGED_TYPE(traversal_tag, traversal)

struct context_storage : alia::context_storage
{
    indie::traversal* indie = nullptr;
};

} // namespace indie

#define ALIA_ADD_UI_CONTEXT_ACCESSORS(storage)                                \
    ALIA_ADD_CORE_CONTEXT_ACCESSORS(storage)                                  \
    ALIA_ADD_DIRECT_TAGGED_DATA_ACCESS(storage, indie::traversal_tag, indie)

ALIA_ADD_UI_CONTEXT_ACCESSORS(indie::context_storage)

namespace indie {

using dataless_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<indie::context_storage>,
    alia::system_tag,
    event_traversal_tag,
    timing_tag,
    indie::traversal_tag>>;

using context = extend_context_type_t<dataless_context, data_traversal_tag>;

using vanilla_context = context_interface<detail::add_tagged_data_types_t<
    detail::empty_structural_collection<indie::context_storage>,
    alia::system_tag,
    event_traversal_tag,
    timing_tag,
    data_traversal_tag>>;

} // namespace indie
} // namespace alia

#endif
