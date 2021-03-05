Working with IDs
================

alia provides a small suite of types for constructing and consuming small IDs.
It's used primarily for identifying signal values (for the purpose of detecting
changes), but the same system can be used to identify nodes in your
component-level application flow so that presentation objects (and other data)
can be stably associated with them.

IDs provide fairly limited capabilities: equality comparison (operators `==`
and `!=`), order comparison (operator `<`), and copying. Any type that provides
these capabilities can serve as an ID (with a little bit of help). Common ID
types include integers, strings, and pointers.

Unlike other alia objects like actions and signals, ID objects are *meant to
persist across application updates.* This is of course a necessity if we're
going to use them to detect changes across updates. Since they tend to be
simple, lightweight, self-contained objects, this tends not to be a concern.

Producing IDs
-------------

<dl>

<dt>make_id(value)</dt><dd>

Creates an ID with the given value. The value type must be copyable and
comparable for equality and order.

</dd>

<dt>combine_ids(id0, ...)</dt><dd>

Combines one or more IDs to create a tuple-like ID.

</dd>

<dt>unit_id</dt><dd>

`unit_id` is a constant that represents 'the unit ID'. (Think ['the unit
type'](https://en.wikipedia.org/wiki/Unit_type).) This is useful when you need
to identify something that has only one possible identity. For example, a
signal that carries a constant value could use the unit ID as its value
identity.

</dd>

<dt>null_id</dt><dd>

`null_id` is a constant that represents 'the null ID'. This is similar to
`nullptr` and, by convention, conveys that no ID is available (yet). In
particular, if a signal supplies the null ID as its value identity, observers
will know that it doesn't have a value and, furthermore, doesn't yet know how
to identify that value.

</dd>

</dl>

Consuming IDs
-------------

All ID types implement the `id_interface` base class. Typically, if you need to
write a function that takes in an ID as a type, you can accept a const
reference to that type.

Note that `id_interface` is *not* templated. It doesn't matter if the ID is
provided as a number or a string or a pointer or some user-defined 3D vector
type. From the consumer's perspective, an ID is an ID, and these are all just
different types of IDs.

When working with objects of type `id_interface const&`, you should be aware
that while they do provide the same equality and ordering operators as the
concrete signal types, they're not copyable, so if you want to, for example,
pass one into `combine_ids()` to construct a composite ID, you need to wrap it
in a call to `ref()` first. For example, `combine_ids(ref(array_id),
make_id(i))` might be a good ID for an item in an array.

Typically, if you're consuming IDs, you'll also want to capture them and store
them for later. This is done using the `captured_id` type. It takes care of
properly cloning the ID and ensuring that it doesn't reference anything outside
the object itself.

Here's a quick example of how you might use `captured_id`:

```cpp
struct my_persistent_data
{
    captured_id id;
    ...
};

void
process_id(my_persistent_data& data, id_interface const& id, ...)
{
    if (!data.id.matches(id))
    {
        // The ID that we've stored persistently doesn't match the ID that was
        // provided, so something has changed!

        // Update things in response to the change.
        ...

        // Capture the ID for later.
        data.id.capture(id);
    }
}
```

The full definition of `captured_id` (and everything else mentioned on this
page) can be found in
[id.hpp](https://github.com/alialib/alia/blob/main/src/alia/id.hpp).
