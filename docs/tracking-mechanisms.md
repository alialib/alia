Tracking Mechanisms
===================

<script>
    init_alia_demos(['numerical-analysis', 'switch-example', 'transform-demo',
        'metered-transform-demo', 'metered-direct-counting',
        'loop-macros-demo', 'for-each-map-demo', 'for-each-vector-demo',
        'named-blocks-demo']);
</script>

This page documents all the various mechanisms you can use to allow alia to
observe the flow of your application and properly maintain the data graph...

Conditionals
------------

### alia_if/else

alia provides the equivalent of `if`, `else if` and `else` as macros. Here again
is a simple example that makes use of all three:

?> The alia control flow macros come in both uppercase and lowercase forms.
   Traditionally, they have been lowercase to more closely resemble the actual
   C++ keywords that they mimic. However, the uppercase form makes it more
   obvious to readers (and clang-format) that the macros are indeed macros.
   Ultimately, it's up to you which style you prefer. If you want to be strict
   in your project, you can disable the lowercase form in [the
   configuration](configuration.md).

[source](numerical.cpp ':include :fragment=analysis')

<div class="demo-panel">
<div id="numerical-analysis"></div>
</div>

As with all alia control flow macros, `alia_if` blocks must be terminated with
`alia_end`.

The conditions that you provide to `alia_if` (and `alia_else_if`) can be signals
or raw C++ values. If a signal is provided and the signal has no value, alia
considers the condition *neither true nor false.* As such, the code dependent on
that statement isn't executed, but any subsequent `else` blocks are *also not
considered.* A condition without a value essentially terminates the entire
sequence of `alia_if/else` statements. As you can see in the above example,
before `n` is given a value, none of the `do_text` calls are executed.

### alia_switch

alia provides a similar set of macros that mirror the behavior of `switch`,
`case`, and `default`. Note that all the normal semantics of `switch` statements
work as expected, including fallthrough:

[source](tracking.cpp ':include :fragment=switch-example')

<div class="demo-panel">
<div id="switch-example"></div>
</div>

The value passed to `alia_switch` is a signal, and just like `alia_if`
conditionals, if that signal doesn't have a value, none of the cases will match
(including the default, if any).

Looping Constructs
------------------

### for_each

`for_each` is the preferred way to loop over containers in alia. It takes care
of flow tracking for you, and it operates on signals: you pass in the container
as a signal, and it passes items back to you as signals.

<dl>

<dt>for_each(ctx, container, f)</dt><dd>

Invoke `f` for each item in `container`. (`container` must be a signal.)

If `container` carries a map-like container (i.e., a container that associates
keys with values), `f` is invoked as `f(ctx, key, value)`, where `key` and
`value` are both signals.

Otherwise, `f` is invoked as `f(ctx, item)`, where `item` is a signal.

In either case, `ctx` is the original context passed to `for_each` (and has all
the same components).

</dd>

</dl>

Here's an example of using `for_each` with a `std::map`:

[source](tracking.cpp ':include :fragment=for-each-map-demo')

<div class="demo-panel">
<div id="for-each-map-demo"></div>
</div>

And here's an equivalent example using a `std::vector` to represent the same
data:

[source](tracking.cpp ':include :fragment=for-each-vector-demo')

<div class="demo-panel">
<div id="for-each-vector-demo"></div>
</div>

#### Item/Data Associations

`for_each` tries to do a reasonable job of associating items consistently with
the same nodes in the data graph, even if those items move around within the
container. For map-like containers, it identifies items by their key. For lists,
it uses the address of the item, which is stable. Otherwise, it simply uses the
index, which of course isn't stable.

If you use `for_each` on a `std::vector` where items can move around, *the
underlying data associated with each item will change when the item moves.*
Depending on what you're associating with your items, the effects of this can
vary from slight inefficiencies to complete discontinuities in your interface.

You can override the default association by defining a `get_alia_id` function
for your item's type. (It should be accessible from within the `alia` namespace,
either via ADL or because it's defined there.) It should take the item as a
parameter and return an alia ID. (See [Working with IDs](working-with-ids.md).)
It can also return `null_id` to fall back to the default ID behavior.

### transform

When working with container signals, it's common to apply some function
individually to each item in the container. Just like `std::transform` allows
you to do this on regular C++ containers, `alia::transform` allows you to do
this on container signals:

<dl>

<dt>transform(ctx, container, f)</dt><dd>

Coming soon...

</dd>

</dl>

[source](application.cpp ':include :fragment=transform-demo')

<div class="demo-panel">
<div id="transform-demo"></div>
</div>

Again, we also could've just counted the number of primes directly with a single
call to `apply`, like this:

[source](application.cpp ':include :fragment=direct-counting')

However, there's one very important difference between the two implementations:
In the `transform` version, the call to `apply(ctx, is_prime, n)` is
*individually caching* the result of `is_prime` for each input number, so as we
update them, `is_prime` is only reinvoked on the individual numbers that have
changed. This is in contrast to the 'single apply' version where any change to
any part of the vector causes the entire `std::count_if` calculator to be
redone.

Below are the two versions with counters to show how many times each has called
`is_prime`.

First the `transform` version:

<div class="demo-panel">
<div id="metered-transform-demo"></div>
</div>

And now the 'single apply' version:

<div class="demo-panel">
<div id="metered-direct-counting"></div>
</div>

Of course, given that this is a contrived example and `is_prime` is a fairly
cheap function to call, it doesn't make much difference which way we do it, but
in real world applications where you might be retrieving data from remote
sources or doing computationally-intensive background calculations, the
difference can be significant.

### alia_for/while

!> Although alia provides macros for tracking `for` and `while` loops, these do
   *not* integrate naturally into alia applications the way that the `if` and
   `switch` versions do. There's an inherent tension between alia's reactive
   style and the imperative looping constructs like `for` and `while`. While
   alia normally discourages introducing immediate, imperative-style side
   effects into your reactive application code, `for` and `while` *depend* on
   those side effects to determine when to terminate the loop. And since alia's
   signals are meant to capture values that change over the life of the
   application (and *not* within a single traversal of your application
   content), `for` and `while` are essentially incompatible with signals.<br>
   <br>
   If possible, you should use `for_each` (above), since that plays nicely with
   signals and avoids introducing immediate side effects into your
   application-level reactive code. The `alia_for` and `alia_while` macros are
   provided largely as a convenience for applications that are trying to
   transition to alia with a minimum of effort.

!> Also note that these macros *assume that your iteration order remains
   constant.* If the data that you're looping over changes order (including
   adding or removing items in the middle of the sequence), *this will cause a
   shift in the underlying alia objects that are associated with your items.*
   Depending on what you're associating with your items, the effects of this can
   vary from slight inefficiencies to complete discontinuities in your
   interface. If you want to avoid this, you should use `named_block`s instead
   of these macros.

If you've read all the above and still want to use `alia_for` or `alia_while`,
here's an example of how you might use `alia_for` to hook up an existing
application's data structures to alia:

[source](tracking.cpp ':include :fragment=loop-macros-demo')

<div class="demo-panel">
<div id="loop-macros-demo"></div>
</div>

### named_block

If you need more control over how you iterate over your data structures, you can
use named blocks. By 'naming' a block of code, you ensure that the piece of the
data graph that's associated with that code is always the same, regardless of
what order your blocks appear in in the execution of your code. A typical usage
follows this form:

```cpp
naming_context nc(ctx);
for(/* Iterate over your items however you want. */)
{
    named_block nb(nc, make_id(id_for_this_item));

    // Do the UI for this item, as you normally would...
}
```

The naming context anchors the named blocks to the data graph. It also provides
a unique context for naming, so if you need to iterate over those same items in
another part of your UI, you can use the same IDs without worrying about
collisions.

`named_block` is a scoped object. It takes effect immediately upon construction,
and when its scope ends and it's destructed, your code will naturally resume
using the normal alia data graph. It also provides `begin` and `end` member
functions in case you need to control its scope more explicitly.

See [Working with IDs](working-with-ids.md) for info about generating the IDs
for your blocks.

Here's a full example in action:

[source](tracking.cpp ':include :fragment=named-blocks-demo')

<div class="demo-panel">
<div id="named-blocks-demo"></div>
</div>
