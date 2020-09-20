#ifndef ALIA_SIGNALS_CONTAINERS_HPP
#define ALIA_SIGNALS_CONTAINERS_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/signals/application.hpp>

namespace alia {

// hide_if_empty(s), where :s is a signal, yields a wrapper for :s that will
// claim to have no value if that value would be empty. (This is determined by
// calling the empty() member function of the value, so it works on most
// containers (including strings).)
template<class Signal>
auto
hide_if_empty(Signal s)
{
    return mask_reads(
        s, lazy_apply([](auto const& x) { return !x.empty(); }, s));
}

} // namespace alia

#endif
