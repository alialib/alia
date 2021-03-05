Consuming Signals
=================

The Signal Interface
--------------------

This section goes into detail about the actual C++ interface provided by a
signal object.

It's unlikely you would use these functions directly in component-level code,
but at some point, they are necessary for actually hooking your dataflow up to
the outside world (e.g., to get a signal's value displayed in a widget).

<dl>

### General Interface

The following are provided by all signals...

<dt>value_type</dt><dd>

a typedef for the type of value that the signal carries

<dd>

<dt>capabilities</dt><dd>

a type tag representing the capabilities of the signal - It's common to simply
use `read_only_signal` or `duplex_signal` here. For a full explanation of
capabilities, see [the
code](https://github.com/alialib/alia/blob/main/src/alia/signals/core.hpp).

<dd>

</dl>

### Reading Interface

The following are provided by readable signals...

<dt>bool<br>has_value() const</dt><dd>

Does the signal currently have a value?

</dd>

<dt>id_interface const&<br>value_id() const</dt><dd>

Get the value identity of the signal.

See below for details.

The returned ID reference is only guaranteed to be valid as long as the signal
itself is valid.

</dd>

<dt>value_type const&<br>read() const</dt><dd>

Read the signal's value. The reference returned here is only guaranteed to be
valid as long as the signal object itself is valid.

</dd>

### Writing Interface

The following are provided by writable signals...

<dt>bool<br>ready_to_write() const</dt><dd>

Is the signal currently ready to write?

</dd>

<dt>void<br>write(Value value) const</dt><dd>

Write the signal's value.

</dd>

</dl>

The above is captured by the `signal_interface` type, but it is rare that you
would use this type directly. See [Custom Signals](custom-signals.md) for info
on implementing this interface, and see [Signals as
Parameters](consuming-signals.md#signals-as-parameters) below for info on how
to reference signal objects.

Value Identity
--------------

alia doesn't place any requirements on a signal to have any sort of
*notification* mechanisms. Instead, it relies on polling. When interfacing alia
with a library, we frequently have to write code that asks "Is this value the
same as the last time we saw it?" For small values like names and numbers, it's
trivial to just store the old value and check it against the new one. However,
just like regular values in C++, a signal value can be arbitrarily large, and
this naive method of detecting changes can become impractical. (If, for
example, we were using alia to display a large image, we wouldn't want to
compare every pixel of the image every frame to make sure that the image hasn't
changed from what's already on screen.)

To address this concern, instead of using a signal's value to detect changes,
alia requires any signal with a value to also provide a *value identity*. A
value identity is a simple proxy for the actual value that is used purely for
detecting changes. It can be summarized with one simple rule:

*If a signal's value changes, its value identity must also change.*

Note that the converse is not true. A signal's value identity is allowed to
change without a change in the actual value of the signal. This would trigger
observers of that signal to unnecessarily reload its value, which would be
inefficient but wouldn't constitute 'incorrect' behavior. (In contrast, if a
signal's value changed without its value identity changing, observers of the
signal wouldn't notice the change and would continue showing a stale view of
the signal, and this *would* be incorrect.)

For smaller values, it's common for a single's value identity to be the value
itself. For larger values, this mechanisms gives the possibility of providing
an abbreviated ID as a proxy for the actual value. Often, these are readily
available in applications that are built on immutable data structures and/or do
revision tracking.

A simple method of generating compact value IDs is to keep an integer counter
and increment it whenever the value changes. Many of alia's built-in signal
types do this.

See [Working with IDs](working-with-ids.md) for more on value ID objects.

Signals as Parameters
---------------------

When accepting a signal as a parameter to a function, the general practice is
to use the types `readable<T>` and `duplex<T>`. `readable<T>` represents any
signal type that carries values of type `T` and supports reading. Similarly,
`duplex<T>` represents any signal type that carries values of type `T` and
supports *both reading and writing.* (Every `duplex<T>` is also a
`readable<T>`.)

?> There's also `writable<T>`, but it's much less common to want to work with a
signal that you're only planning to write to, and you should consider whether
or not you actually want an [action](actions.md).

Both `readable<T>` and `duplex<T>` are implemented as references internally, so
they should be passed by value, e.g.:

```cpp
// Do a simple text display widget.
void
text(context ctx, readable<std::string> text);

// Do a widget for editing a string of text.
void
text_control(context ctx, duplex<std::string> text);
```

In both of these cases, `text` is an actual signal object and can be used as
such internally. It can be passed into other functions that expects signals and
can be part of a signal composition.

?> These objects are essentially wrappers around references to the abstract
   base class `signal_interface`. If you're wondering why we don't just
   directly use references to that abstract base class, it's primarily because
   this approach fits in better with alia's propensity for composing signal
   objects on the stack (via copying). If `text` were passed in as a reference
   to `signal_interface`, then we couldn't compose it this way without wrapping
   it in a reference object, which past experience has shown becomes painful in
   many scenarios. With `readable` and `duplex` defined as they are now, this
   is handled for you.

### Signal Templates

Special care must be taken when writing template functions that consume
signals. If you try to write a function like the following, you'll run into
problems:

```cpp
template<class T>
void
input(context ctx, duplex<T> x)
{
   ...
}
```

The reason is that although any duplex signal that carries values of type `T`
is convertible to a `duplex<T>`, the relationship isn't close enough to allow
for the deduction of `T`. A simple workaround is to just change the template
parameter to be the signal type itself:

```cpp
template<class Signal>
void
input(context ctx, Signal const& x)
{
   // We can still access the actual value type...
   typedef typename Signal::value_type value_t;

   ...
}
```

To narrow down the function so that it only works on signal types, you can use
`std::enable_if` with a type predicate:

```cpp
template<class Signal>
std::enable_if_t<is_duplex_signal_type<Signal>::value, void>
input(context ctx, Signal const& x)
{
    return signal_has_value(x) && (read_signal(x) ? true : false);
}
```

`is_readable_signal_type`, `is_writable_signal_type` and `is_signal_type` are
also provided.

C++20 concepts should improve this situation.

### signalize()

Sometimes it's nice to allow those calling your function the option of passing
in *either* a raw C++ value or a signal. `signalize()` can be helpful here:

<dl>

<dt>signalize(x)</dt><dd>

`signalize(x)` turns `x` into a signal if it isn't already one.

Or, in other words...

`signalize(s)`, where `s` is a signal, returns `s`.

`signalize(v)`, where `v` is a raw value, returns a value signal carrying `v`.

<dd>

</dl>
