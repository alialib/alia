#include <alia/html/bootstrap/cards.hpp>

#include <alia/html/bootstrap/utilities.hpp>
#include <alia/html/elements.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

html::element_handle
card(
    html::context ctx,
    alia::function_view<void(internal_card_handle&)> content)
{
    element_handle card = element(ctx, "div");
    card.class_("card").content([&] {
        internal_card_handle handle(card);
        content(handle);
    });
    return card;
}

}}} // namespace alia::html::bootstrap
