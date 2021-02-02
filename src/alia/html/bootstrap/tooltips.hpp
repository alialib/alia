#ifndef ALIA_HTML_BOOTSTRAP_TOOLTIPS_HPP
#define ALIA_HTML_BOOTSTRAP_TOOLTIPS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html { namespace bootstrap {

void
attach_tooltip(element_handle& element);

template<class Text, class Placement>
void
tooltip(element_handle element, Text&& text, Placement&& placement)
{
    element.attr("data-toggle", "tooltip")
        .attr("title", std::forward<Text>(text))
        .attr("data-placement", std::forward<Placement>(placement))
        .on_init([&](auto&) { attach_tooltip(element); });
}

template<class Text>
void
tooltip(element_handle element, Text&& text)
{
    element.attr("data-toggle", "tooltip")
        .attr("title", std::forward<Text>(text))
        .on_init([&](auto&) { attach_tooltip(element); });
}

}}} // namespace alia::html::bootstrap

#endif
