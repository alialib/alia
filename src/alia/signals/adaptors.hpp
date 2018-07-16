#ifndef ALIA_SIGNALS_ADAPTORS_HPP
#define ALIA_SIGNALS_ADAPTORS_HPP

#include <alia/signals/utilities.hpp>

namespace alia {

// fake_readability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have read capabilities. It will never actually be readable, but
// it will type-check as a readable signal.
template<class Wrapped>
struct readability_faker : signal<
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
struct signal_caster : regular_signal<To, typename Wrapped::direction_tag>
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
struct readability_signal : regular_signal<bool, read_only_signal>
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
struct writability_signal : regular_signal<bool, read_only_signal>
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

} // namespace alia

#endif
