#include <alia/html/bootstrap/tooltips.hpp>

namespace alia { namespace html { namespace bootstrap {

void
attach_tooltip(element_handle& element)
{
    element.init([&](auto&) {
        EM_ASM(
            { jQuery(Module['nodes'][$0]).tooltip(); }, element.asmdom_id());
    });
}

}}} // namespace alia::html::bootstrap
