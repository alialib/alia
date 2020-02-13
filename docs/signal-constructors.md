Construction
============

Basic Constructors
------------------

The following functions allow you to construct basic signals from raw
C++ values. These are perfect for working with small values: booleans,
numbers, small strings, and perhaps small data structures. However, all
of them rely on making copies and invoking the equality operator to
detect changes in the values they carry, so they are generally not the
best choice for large data structures unless those structures are
especially efficient at these operations.

<dl>

<dt>value(T x)</dt><dd>

Returns a *read-only* signal carrying the value `x`.

Internally, the signal stores a *copy* of the value.
<dd>

<dt>value(char const* x)</dt><dd>

Constructs a *read-only* signal carrying a `std::string` initialized with `x`.

The value ID logic for this signal assumes that this overload is only used for
**string literals** (i.e., that the contents of the string will never change).
If you're doing real C-style string manipulations, you should convert them to
`std::string` first or use a custom signal.
</dd>

<dt>direct(T& x)</dt><dd>

Returns a *bidirectional* signal carrying the value `x`.

Internally, the signal stores a *reference* to the value.
</dd>

<dt>direct(T const& x)</dt><dd>

Returns a *read-only* signal carrying the value `x`.

Internally, the signal stores a *reference* to the value.
</dd>

</dl>

Lambda Constructors
-------------------

When you need a little more control but don't want to create a custom signal
type, you can create a signal from one or more lambdas functions (or other
function objects). For completeness, you can create a fully functional,
bidirectional signal using lambdas, but the further you go down this list, the
more likely it is that you should just create a custom signal type...

<dl>

<dt>lambda_reader(is_readable, read)</dt><dd>

Creates a read-only signal whose value is determined by calling the
`is_readable` and `read` lambdas. (Neither takes any arguments.)

The following is equivalent to `value(12)`:

```cpp
lambda_reader(always_readable, []() { return 12; });
```

`always_readable` is just a function that always returns `true`. It's
considered a clear and concise way to indicate that a signal is always
readable.
</dd>

<dt>lambda_reader(is_readable, read, generate_id)</dt><dd>

Creates a read-only signal whose value is determined by calling
`is_readable` and `read` and whose ID is determined by calling
`generate_id`. (None of which take any arguments.)

With this overload, you can achieve something that's impossible with the
basic constructors: a signal that carries a large value but doesn't actually
have to touch that large value every pass. For example:

```cpp
lambda_reader(
    always_readable,
    [&]() { return my_object; },
    [&]() { return make_id(my_object.uid); });
```

With the above signal, change detection can be done using the object's ID, so
the object's value itself only has to be touched when new values are
retrieved.
</dd>

<dt>lambda_bidirectional(is_readable, read, is_writable, write)</dt><dd>

Creates a bidirectional signal whose value is read by calling `is_readable`
and `read` and written by calling `is_writable` and `write`. Only
`write` takes an argument (the new value).
</dd>

<dt>lambda_bidirectional(is_readable, read, is_writable, write, generate_id)
</dt><dd>

Creates a bidirectional signal whose value is read by calling `is_readable`
and `read` and written by calling `is_writable` and `write`. Its ID is
determined by calling `generate_id`. Only `write` takes an argument (the
new value).
</dd>

</dl>

The Empty Signal
----------------

Occasionally, it's useful to create a signal that never carries a value.

<dl>

<dt>empty&lt;T&gt;()</dt><dd>

Creates a signal that type checks as a readable signal carrying a value of type
`T` but never actually provides a value.

</dd>

</dl>
