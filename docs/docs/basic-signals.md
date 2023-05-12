Basic Usage
===========

Signals are the main mechanic for modeling dataflow in alia, and they're
introduced on the [Feature Overview](feature-overview.md) page. If you haven't
already, you should read that before continuing on here.

Signal Objects
--------------

It's important to understand that although signals are conceptually values that
vary over time and persist across frames of an application, actual signal
objects are just transient interfaces to the underlying signal. It's common to
create signal objects on the stack, when and where they're needed, and even
adapt/compose them on the spot to create more interesting signals. For example,
assuming we have some floating point variable `x` that represents some
persistent application state, we might write code like this:

```cpp
html::input(ctx, scale(direct(x), 100));
```

`direct(x)` creates a signal object that directly exposes the variable `x` as a
signal. `scale(direct(x), 100)` presents a view of `x` scaled up by a factor of
100 (as another signal object), which is then passed into `html::input` to
allow the user to edit it. (This might be done because the user wants to edit
`x` as a percentage, while in code we want it as a simple ratio.)

Remember that that line of code lives in a function that is reinvoked on every
update, so while `x` persists across the lifetime of the application, the
signal objects created by `direct()` and `scale()` *do not.* They're created on
the stack solely for the purpose of exposing `x` to `html::input`, and they
cease to exist between updates. And, of course, `x` is the only real *state*
here, so `x` is all that really *needs* to persist between frames.

It's a general principle in alia that objects shouldn't persist across frames
without a good reason &mdash; after all, it would just mean more objects to
keep synchronized &mdash; so this is the most common way to work with signals
in alia. For the sake of brevity, this documentation refers to these transient
signal objects simply as 'signals', and from a conceptual standpoint, it's
perfectly fine to think of `scale(direct(x), 100)` as representing a persistent
signal that changes over time, but for the purpose of actually understanding
what's going on in your code, it's also helpful to be aware that there's no
actual persistent C++ object associated with that signal.

Capabilities
------------

In C++, when you want to pass a `std::string` parameter into a function, you
might do so using `std::string`, `std::string&`, `std::string const&` or even
`std::string&&` depending on the intended usage. Each, in effect, conveys a
different set of capabilities to the function receiving the parameter: the
ability to write back to the string, the ability to move it efficiently
somewhere else, the ability to refer to it by reference, etc. In alia, a
signal's type carries a similar set of information as a 'capabilities' type
tag.

Capabilities will be discussed more in depth later on. For now, it's sufficient
to understand that not all signals have the same capabilities with respect to
reading and writing. In particular, the documentation below will describe
signals as either 'read-only' or 'duplex'. While read-only signals only allow
you to read the value that they carry, *duplex* signals *also* allow you to
*write* a value back to the signal.

Basic Constructors
------------------

The following functions allow you to construct basic signals from raw C++
values. These are perfect for bringing in small values from your application
data: booleans, numbers, small strings, and perhaps small data structures.
However, all of them rely on making copies and invoking the equality operator
to detect changes in the values they carry, so use caution when applying them
to larger data structures. (The exception to this rule is data structures that
are particularly efficient at copying and testing for equality (like immutable
and copy-on-write types). These tend to naturally work well with alia.)

<dl>

<dt>value(T x)</dt><dd>

Returns a *read-only* signal carrying the value `x`.

Internally, the signal stores a *copy* of the value.
<dd>

<dt>direct(T& x)</dt><dd>

Returns a *duplex* signal carrying the value `x`.

Internally, the signal stores a *reference* to the value.
</dd>

<dt>direct(T const& x)</dt><dd>

Returns a *read-only* signal carrying the value `x`.

Internally, the signal stores a *reference* to the value.
</dd>

</dl>

The Empty Signal
----------------

Occasionally, it's useful to create a signal that never carries a value.

<dl>

<dt>empty&lt;T&gt;()</dt><dd>

Returns a signal that type checks as a readable signal carrying a value of type
`T` but never actually carries a value.

</dd>

</dl>

State Signals
-------------

Sometimes you need some temporary, local state somewhere in your UI, for
example to capture the open/closed state of a tree node, the selected item in a
list, or the name of a new item that the user is going to add to the list.
Thanks to alia's data graph, you can make this state appear where you need it
by calling `get_state`:

<dl>

<dt>get_state(ctx, initial_value)</dt><dd>

Returns a duplex signal that references some temporary, local state.

`initial_value` can be another signal or a raw value. It determines the value
type of the state and supplies the initial value. If `initial_value` is a
signal that has no value, the state is initially empty. It will get a value
when one is explicitly written to it or when `initial_value` produces a value
(whichever happens first). Once the state has a value, `initial_value` is
ignored. (The state won't change in response to changes in `initial_value`.)

</dd>

</dl>
