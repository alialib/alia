Custom Signals
==============

Signal Capabilities
-------------------

Custom Types
------------



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
