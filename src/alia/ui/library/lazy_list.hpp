#ifndef ALIA_UI_LIBRARY_LAZY_LIST_HPP
#define ALIA_UI_LIBRARY_LAZY_LIST_HPP

#include <alia/ui/context.hpp>

namespace alia {

void
lazy_list_ui(
    ui_context ctx,
    size_t item_count,
    function_view<void(ui_context, size_t)> const& do_item);

} // namespace alia

#endif
