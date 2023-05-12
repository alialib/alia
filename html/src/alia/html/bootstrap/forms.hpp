#ifndef ALIA_HTML_BOOTSTRAP_FORMS_HPP
#define ALIA_HTML_BOOTSTRAP_FORMS_HPP

#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

namespace detail {
element_handle
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label);
}
template<class Label>
element_handle
checkbox(html::context ctx, duplex<bool> value, Label label)
{
    return detail::checkbox(ctx, value, signalize(label));
}

}}} // namespace alia::html::bootstrap

#endif
