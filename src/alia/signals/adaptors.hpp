#ifndef ALIA_SIGNALS_ADAPTORS_HPP
#define ALIA_SIGNALS_ADAPTORS_HPP

#include <alia/signals/basic.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

// fake_readability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have read capabilities. It will never actually be readable, but
// it will type-check as a readable signal.
template<class Wrapped>
struct readability_faker : signal<
                               readability_faker<Wrapped>,
                               typename Wrapped::value_type,
                               typename signal_direction_union<
                                   read_only_signal,
                                   typename Wrapped::direction_tag>::type>
{
    readability_faker(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    id_interface const&
    value_id() const
    {
        return no_id;
    }
    bool
    is_readable() const
    {
        return false;
    }
    // Since this is only faking readability, read() should never be called.
    // LCOV_EXCL_START
    typename Wrapped::value_type const&
    read() const
    {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
        return *(typename Wrapped::value_type const*) nullptr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    // LCOV_EXCL_STOP
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        return wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
readability_faker<Wrapped>
fake_readability(Wrapped const& wrapped)
{
    return readability_faker<Wrapped>(wrapped);
}

// fake_writability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have write capabilities. It will never actually be writable, but
// it will type-check as a writable signal.
template<class Wrapped>
struct writability_faker : signal<
                               writability_faker<Wrapped>,
                               typename Wrapped::value_type,
                               typename signal_direction_union<
                                   write_only_signal,
                                   typename Wrapped::direction_tag>::type>
{
    writability_faker(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    id_interface const&
    value_id() const
    {
        return wrapped_.value_id();
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    is_writable() const
    {
        return false;
    }
    // Since this is only faking writability, write() should never be called.
    // LCOV_EXCL_START
    void
    write(typename Wrapped::value_type const& value) const
    {
    }
    // LCOV_EXCL_STOP

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
writability_faker<Wrapped>
fake_writability(Wrapped const& wrapped)
{
    return writability_faker<Wrapped>(wrapped);
}

// signal_cast<Value>(x), where :x is a signal, yields a proxy for :x with
// the value type :Value. The proxy will apply static_casts to convert its
// own values to and from :x's value type.
template<class Wrapped, class To>
struct signal_caster : regular_signal<
                           signal_caster<Wrapped, To>,
                           To,
                           typename Wrapped::direction_tag>
{
    signal_caster(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    To const&
    read() const
    {
        return lazy_reader_.read(
            [&] { return static_cast<To>(wrapped_.read()); });
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(To const& value) const
    {
        return wrapped_.write(static_cast<typename Wrapped::value_type>(value));
    }

 private:
    Wrapped wrapped_;
    lazy_reader<To> lazy_reader_;
};
template<class To, class Wrapped>
signal_caster<Wrapped, To>
signal_cast(Wrapped const& wrapped)
{
    return signal_caster<Wrapped, To>(wrapped);
}

// is_readable(x) yields a signal to a boolean which indicates whether or
// not x is readable. (The returned signal is always readable itself.)
template<class Wrapped>
struct readability_signal
    : regular_signal<readability_signal<Wrapped>, bool, read_only_signal>
{
    readability_signal(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.is_readable();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
is_readable(Wrapped const& wrapped)
{
    return readability_signal<Wrapped>(wrapped);
}

// is_writable(x) yields a signal to a boolean which indicates whether or
// not x is writable. (The returned signal is always readable.)
template<class Wrapped>
struct writability_signal
    : regular_signal<writability_signal<Wrapped>, bool, read_only_signal>
{
    writability_signal(Wrapped const& wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    bool const&
    read() const
    {
        value_ = wrapped_.is_writable();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<class Wrapped>
auto
is_writable(Wrapped const& wrapped)
{
    return writability_signal<Wrapped>(wrapped);
}

// add_fallback(primary, fallback), where :primary and :fallback are both
// signals, yields another signal whose value is that of :primary if it's
// readable and that of :fallback otherwise.
// All writes go directly to :primary.
template<class Primary, class Fallback>
struct fallback_signal : signal<
                             fallback_signal<Primary, Fallback>,
                             typename Primary::value_type,
                             typename Primary::direction_tag>
{
    fallback_signal()
    {
    }
    fallback_signal(Primary primary, Fallback fallback)
        : primary_(primary), fallback_(fallback)
    {
    }
    bool
    is_readable() const
    {
        return primary_.is_readable() || fallback_.is_readable();
    }
    typename Primary::value_type const&
    read() const
    {
        return primary_.is_readable() ? primary_.read() : fallback_.read();
    }
    id_interface const&
    value_id() const
    {
        id_ = combine_ids(
            make_id(primary_.is_readable()),
            primary_.is_readable() ? ref(primary_.value_id())
                                   : ref(fallback_.value_id()));
        return id_;
    }
    bool
    is_writable() const
    {
        return primary_.is_writable();
    }
    void
    write(typename Primary::value_type const& value) const
    {
        primary_.write(value);
    }

 private:
    mutable id_pair<simple_id<bool>, id_ref> id_;
    Primary primary_;
    Fallback fallback_;
};
template<class Primary, class Fallback>
fallback_signal<Primary, Fallback>
add_fallback(Primary const& primary, Fallback const& fallback)
{
    return fallback_signal<Primary, Fallback>(primary, fallback);
}

// simplify_id(s), where :s is a signal, yields a wrapper for :s with the exact
// same read/write behavior but whose value ID is a simple_id (i.e., it is
// simply the value of the signal).
//
// The main utility of this is in cases where you have a signal carrying a
// small value with a complicated value ID (because it was picked from the
// signal of a larger data structure, for example). The more complicated ID
// might change superfluously.
//
template<class Wrapped>
struct simplified_id_wrapper : regular_signal<
                                   simplified_id_wrapper<Wrapped>,
                                   typename Wrapped::value_type,
                                   typename Wrapped::direction_tag>
{
    simplified_id_wrapper(Wrapped wrapped) : wrapped_(wrapped)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        return wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
};
template<class Wrapped>
simplified_id_wrapper<Wrapped>
simplify_id(Wrapped wrapped)
{
    return simplified_id_wrapper<Wrapped>(wrapped);
}

// signalize(x) turns x into a signal if it isn't already one.
// Or, in other words...
// signalize(s), where s is a signal, returns s.
// signalize(v), where v is a raw value, returns a value signal carrying s.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, Signal>
signalize(Signal s)
{
    return s;
}
template<class Value, std::enable_if_t<!is_signal_type<Value>::value, int> = 0>
auto
signalize(Value v)
{
    return value(std::move(v));
}

// mask(signal, condition) does the equivalent of bitmasking on individual
// signals. If :condition evaluates to true, the mask evaluates to :signal.
// Otherwise, it evaluates to an empty signal of the same type.
template<class Primary, class Mask>
struct masking_signal : signal<
                            masking_signal<Primary, Mask>,
                            typename Primary::value_type,
                            typename Primary::direction_tag>
{
    masking_signal()
    {
    }
    masking_signal(Primary primary, Mask mask) : primary_(primary), mask_(mask)
    {
    }
    bool
    is_readable() const
    {
        return mask_.is_readable() && mask_.read() && primary_.is_readable();
    }
    typename Primary::value_type const&
    read() const
    {
        return primary_.read();
    }
    id_interface const&
    value_id() const
    {
        if (mask_.is_readable() && mask_.read())
            return primary_.value_id();
        else
            return no_id;
    }
    bool
    is_writable() const
    {
        return mask_.is_readable() && mask_.read() && primary_.is_writable();
    }
    void
    write(typename Primary::value_type const& value) const
    {
        primary_.write(value);
    }

 private:
    Primary primary_;
    Mask mask_;
};
template<class Signal, class Condition>
auto
make_masking_signal(Signal signal, Condition condition)
{
    return masking_signal<Signal, Condition>(signal, condition);
}
template<class Signal, class Condition>
auto
mask(Signal signal, Condition condition)
{
    return make_masking_signal(signalize(signal), signalize(condition));
}

} // namespace alia

#endif
