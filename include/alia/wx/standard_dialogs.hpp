#ifndef ALIA_WX_STANDARD_DIALOGS_HPP
#define ALIA_WX_STANDARD_DIALOGS_HPP

#include <string>
#include <boost/optional/optional.hpp>
#include <alia/color.hpp>

namespace alia { namespace wx {

bool ask_for_color(rgb8* result, rgb8 const* initial = 0);

void show_message(std::string const& message);
void show_warning(std::string const& message);
void show_error(std::string const& message);

}}

#endif
