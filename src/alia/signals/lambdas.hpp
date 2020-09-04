#ifndef ALIA_SIGNALS_LAMBDAS_HPP
#define ALIA_SIGNALS_LAMBDAS_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines utilities for constructing custom signals via lambda
// functions.

namespace alia {

// lambda_constant(read) creates a read-only signal whose value is constant and
// is determined by calling :read.
template<class Value, class Read>
struct simple_lambda_constant_signal
    : signal<
          simple_lambda_constant_signal<Value, Read>,
          Value,
          read_only_signal>
{
    simple_lambda_constant_signal(Read read)
        : read_(read), value_(decltype(read())())

    {
    }
    bool
    has_value() const
    {
        return true;
    }
    id_interface const&
    value_id() const
    {
        return unit_id;
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    Read read_;
    mutable decltype(read_()) value_;
};
template<class Read>
auto
lambda_constant(Read read)
{
    return simple_lambda_constant_signal<std::decay_t<decltype(read())>, Read>(
        read);
}

// lambda_reader(read) creates a read-only signal whose value is determined by
// calling :read.
template<class Value, class Read>
struct simple_lambda_reader_signal
    : regular_signal<
          simple_lambda_reader_signal<Value, Read>,
          Value,
          read_only_signal>
{
    simple_lambda_reader_signal(Read read)
        : read_(read), value_(decltype(read())())
    {
    }
    bool
    has_value() const
    {
        return true;
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    Read read_;
    mutable decltype(read_()) value_;
};
template<class Read>
auto
lambda_reader(Read read)
{
    return simple_lambda_reader_signal<std::decay_t<decltype(read())>, Read>(
        read);
}

// lambda_reader(has_value, read) creates a read-only signal whose value is
// determined by calling :has_value and :read.
template<class Value, class HasValue, class Read>
struct lambda_reader_signal : regular_signal<
                                  lambda_reader_signal<Value, HasValue, Read>,
                                  Value,
                                  read_only_signal>
{
    lambda_reader_signal(HasValue has_value, Read read)
        : has_value_(has_value), read_(read), value_(decltype(read())())
    {
    }
    bool
    has_value() const
    {
        return has_value_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    HasValue has_value_;
    Read read_;
    mutable decltype(read_()) value_;
};
template<class HasValue, class Read>
auto
lambda_reader(HasValue has_value, Read read)
{
    return lambda_reader_signal<
        std::decay_t<decltype(read())>,
        HasValue,
        Read>(has_value, read);
}

// lambda_reader(has_value, read, generate_id) creates a read-only signal
// whose value is determined by calling :has_value and :read and whose ID is
// determined by calling :generate_id.
template<class Value, class HasValue, class Read, class GenerateId>
struct lambda_reader_signal_with_id
    : signal<
          lambda_reader_signal_with_id<Value, HasValue, Read, GenerateId>,
          Value,
          read_only_signal>
{
    lambda_reader_signal_with_id(
        HasValue has_value, Read read, GenerateId generate_id)
        : has_value_(has_value),
          read_(read),
          value_(decltype(read())()),
          generate_id_(generate_id)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = generate_id_();
        return id_;
    }
    bool
    has_value() const
    {
        return has_value_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }

 private:
    HasValue has_value_;
    Read read_;
    mutable decltype(read_()) value_;
    GenerateId generate_id_;
    mutable decltype(generate_id_()) id_;
};
template<class HasValue, class Read, class GenerateId>
auto
lambda_reader(HasValue has_value, Read read, GenerateId generate_id)
{
    return lambda_reader_signal_with_id<
        std::decay_t<decltype(read())>,
        HasValue,
        Read,
        GenerateId>(has_value, read, generate_id);
}

// lambda_duplex(has_value, read, ready_to_write, write) creates a duplex
// signal whose value is read by calling :has_value and :read and written by
// calling :ready_to_write and :write.
template<
    class Value,
    class HasValue,
    class Read,
    class ReadyToWrite,
    class Write>
struct lambda_duplex_signal
    : regular_signal<
          lambda_duplex_signal<Value, HasValue, Read, ReadyToWrite, Write>,
          Value,
          signal_capabilities<signal_readable, signal_writable>>
{
    lambda_duplex_signal(
        HasValue has_value,
        Read read,
        ReadyToWrite ready_to_write,
        Write write)
        : has_value_(has_value),
          read_(read),
          value_(decltype(read())()),
          ready_to_write_(ready_to_write),
          write_(write)
    {
    }
    bool
    has_value() const
    {
        return has_value_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }
    bool
    ready_to_write() const
    {
        return ready_to_write_();
    }
    void
    write(Value value) const
    {
        write_(std::move(value));
    }

 private:
    HasValue has_value_;
    Read read_;
    mutable decltype(read_()) value_;
    ReadyToWrite ready_to_write_;
    Write write_;
};
template<class HasValue, class Read, class ReadyToWrite, class Write>
auto
lambda_duplex(
    HasValue has_value, Read read, ReadyToWrite ready_to_write, Write write)
{
    return lambda_duplex_signal<
        std::decay_t<decltype(read())>,
        HasValue,
        Read,
        ReadyToWrite,
        Write>(has_value, read, ready_to_write, write);
}

// lambda_duplex(has_value, read, ready_to_write, write, generate_id) creates a
// duplex signal whose value is read by calling :has_value and :read and
// written by calling :ready_to_write and :write. Its ID is determined by
// calling :generate_id.
template<
    class Value,
    class HasValue,
    class Read,
    class ReadyToWrite,
    class Write,
    class GenerateId>
struct lambda_duplex_signal_with_id
    : signal<
          lambda_duplex_signal_with_id<
              Value,
              HasValue,
              Read,
              ReadyToWrite,
              Write,
              GenerateId>,
          Value,
          signal_capabilities<signal_readable, signal_writable>>
{
    lambda_duplex_signal_with_id(
        HasValue has_value,
        Read read,
        ReadyToWrite ready_to_write,
        Write write,
        GenerateId generate_id)
        : has_value_(has_value),
          read_(read),
          value_(decltype(read())()),
          ready_to_write_(ready_to_write),
          write_(write),
          generate_id_(generate_id)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = generate_id_();
        return id_;
    }
    bool
    has_value() const
    {
        return has_value_();
    }
    Value const&
    read() const
    {
        value_ = read_();
        return value_;
    }
    bool
    ready_to_write() const
    {
        return ready_to_write_();
    }
    void
    write(Value value) const
    {
        write_(std::move(value));
    }

 private:
    HasValue has_value_;
    Read read_;
    mutable decltype(read_()) value_;
    ReadyToWrite ready_to_write_;
    Write write_;
    GenerateId generate_id_;
    mutable decltype(generate_id_()) id_;
};
template<
    class HasValue,
    class Read,
    class ReadyToWrite,
    class Write,
    class GenerateId>
auto
lambda_duplex(
    HasValue has_value,
    Read read,
    ReadyToWrite ready_to_write,
    Write write,
    GenerateId generate_id)
{
    return lambda_duplex_signal_with_id<
        std::decay_t<decltype(read())>,
        HasValue,
        Read,
        ReadyToWrite,
        Write,
        GenerateId>(has_value, read, ready_to_write, write, generate_id);
}

// This is just a clear and concise way of indicating that a lambda signal
// always has a value.
inline bool
always_has_value()
{
    return true;
}

// This is just a clear and concise way of indicating that a lambda signal is
// always ready to write.
inline bool
always_ready()
{
    return true;
}

} // namespace alia

#endif
