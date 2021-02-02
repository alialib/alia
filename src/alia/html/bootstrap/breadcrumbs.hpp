#ifndef ALIA_HTML_BOOTSTRAP_BREADCRUMBS_HPP
#define ALIA_HTML_BOOTSTRAP_BREADCRUMBS_HPP

#include <alia/html/dom.hpp>

namespace alia { namespace html { namespace bootstrap {

struct breadcrumb
{
    scoped_element nav;
    scoped_element ol;

    breadcrumb()
    {
    }
    breadcrumb(html::context ctx)
    {
        this->begin(ctx);
    }
    ~breadcrumb()
    {
        this->end();
    }

    breadcrumb&
    begin(html::context ctx)
    {
        this->nav.begin(ctx, "nav").attr("aria-label", "breadcrumb");
        this->ol.begin(ctx, "ol").class_("breadcrumb");
        return *this;
    }
    void
    end()
    {
        this->ol.end();
        this->nav.end();
    }

    template<class Link>
    element_handle
    item(Link&& link)
    {
        return element(this->nav.context(), "li")
            .class_("breadcrumb-item")
            .children([&] { std::forward<Link>(link)(); });
    }

    element_handle
    active_item()
    {
        return element(this->nav.context(), "li")
            .classes("breadcrumb-item active")
            .attr("aria-current", "page");
    }

    template<class Label>
    element_handle
    active_item(Label label)
    {
        return active_item().text(label);
    }
};

}}} // namespace alia::html::bootstrap

#endif
