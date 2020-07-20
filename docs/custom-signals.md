Custom Signals
==============

Custom Signal Types
-------------------

Implementing your own signal type is often the best way to integrate complex
application data types into alia's dataflow system. Before attempting this, it's
recommended that you read [Consuming Signals](consuming-signals.md) to
understand the expected interface and the expectation of a value identity.

To implement a custom signal type, you create a `struct`/`class` that derives
from `signal` and implements the relevant parts of [the signal
interface](consuming-signals.md#the-signal-interface). The signature of `signal`
is as follows:

```cpp
template<class Derived, class Value, class Capabilities>
struct signal;
```

`Derived` is the signal type that you're implementing.

`Value` is the type of value carried by the signal.

`Capabilities` is a tag type indicating the read/write capabilities of the
signal. It's common to simply use `read_only_signal` or `duplex_signal` here.
For a full explanation of capabilities, see [the
code](https://github.com/tmadden/alia/blob/master/src/alia/signals/core.hpp).

For signals with small value types, it's common for the value ID to simply be a
copy of the signal value. These are called 'regular' signals and can be
implemented by deriving from `regular_signal` instead. It has the same signature
as `signal` but provides the implementation of `value_id` for you.

For illustration, here's the actual implementation of the signal you create when
calling `direct()` on a non-const reference. It's a regular, duplex signal whose
values are copyable:

```cpp
template<class Value>
struct direct_signal
    : regular_signal<direct_signal<Value>, Value, copyable_duplex_signal>
{
    explicit direct_signal(Value* v) : v_(v)
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
        return *v_;
    }
    Value
    movable_value() const
    {
        Value movable = std::move(*v_);
        return movable;
    }
    bool
    ready_to_write() const
    {
        return true;
    }
    void
    write(Value value) const
    {
        *v_ = std::move(value);
    }

 private:
    Value* v_;
};
```

And here's an example of a read-only signal that uses a version counter as its
value ID:

```cpp
template<class Value>
struct async_signal : signal<async_signal<Value>, Value, read_only_signal>
{
    async_signal(async_operation_data<Value>& data) : data_(&data)
    {
    }

    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->version);
        return id_;
    }

    bool
    has_value() const
    {
        return data_->status == async_status::COMPLETE;
    }

    Value const&
    read() const
    {
        return data_->result;
    }

 private:
    async_operation_data<Value>* data_;
    mutable simple_id<counter_type> id_;
};
```

This is the signal that you get when calling `async()`. The source code is full
of other examples and should be a good source of guidance if you're implementing
your own signals or signal adaptors.

Lambda Constructors
-------------------

When you need a little more control but don't want to create a custom signal
type, you can create a signal from one or more lambda functions (or other
function objects). For completeness, you can create a fully functional, duplex
signal using lambdas, but the further you go down this list, the more likely it
is that you should just create a custom signal type...

<dl>

<dt>lambda_constant(read)</dt><dd>

Creates a read-only signal whose value is constant and is determined by calling
`read` (which doesn't take any arguments).

Note that `read` is expected to *always return the same value.*

`lambda_constant` should be used when a signal carries a constant value and the
construction of that value is non-trivial. Consider the following two
statements:

```cpp
auto fruits = value(std::vector<std::string>{"apple", "banana", "cherry"});
```

```cpp
auto fruits = lambda_constant(
    []() { return std::vector<std::string>{"apple", "banana", "cherry"}; });
```

Although the two have equivalent behavior, in the `value` version, our vector is
constructed *every pass through our component function.* Moreover, wherever our
`fruits` signal is used, the consuming code will store the whole vector and,
every pass, *compare it to the newly constructed vector to detect changes.*

The use of `lambda_constant` avoids both of these inefficiencies. Our vector
will only be constructed when it's actually needed by the consumer(s) of the
`fruits` signal, and since the signal is explicitly marked as constant, the
unnecessary copies and comparisons will be eliminated.

<dt>lambda_reader(read)</dt><dd>

Creates a read-only signal that always has a value and whose value is determined
by calling `read` (which doesn't take any arguments).

In contrast to `lambda_constant`, the signal created here is *not* considered to
be constant, so `read` is allowed to return different values over time.

The following is equivalent to `value(12)`:

```cpp
lambda_reader([]() { return 12; });
```

</dd>

<dt>lambda_reader(has_value, read)</dt><dd>

Creates a read-only signal whose value is determined by calling `has_value` and
`read`. (Neither takes any arguments.)

The following is also equivalent to `value(12)`:

```cpp
lambda_reader(always_has_value, []() { return 12; });
```

`always_has_value` is just a function that always returns `true`. It's
considered a clear and concise way to indicate that a signal always carries a
value.
</dd>

<dt>lambda_reader(has_value, read, generate_id)</dt><dd>

Creates a read-only signal whose value is determined by calling `has_value` and
`read` and whose value ID is determined by calling `generate_id`. (None of which
take any arguments.)

With this overload, you can achieve something that's impossible with the
basic constructors: a signal that carries a large value but doesn't actually
have to touch that large value every pass. For example:

```cpp
lambda_reader(
    always_has_value,
    [&]() { return my_object; },
    [&]() { return make_id(my_object.uid); });
```

With the above signal, change detection can be done using the object's `uid`, so
the object's value itself only has to be touched when new values are retrieved.
</dd>

<dt>lambda_duplex(has_value, read, ready_to_write, write)</dt><dd>

Creates a duplex signal whose value is read by calling `has_value` and `read`
and written by calling `ready_to_write` and `write`. Only `write` takes an
argument (the new value).
</dd>

<dt>lambda_duplex(has_value, read, ready_to_write, write, generate_id)
</dt><dd>

Creates a duplex signal whose value is read by calling `has_value` and `read`
and written by calling `ready_to_write` and `write`. Its value ID is determined
by calling `generate_id`. Only `write` takes an argument (the new value).
</dd>

</dl>
