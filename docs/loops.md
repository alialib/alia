Loops
=====

<script>
    init_alia_demos([
        'loop-macros-demo', 'for-each-map-demo', 'for-each-vector-demo',
        'named-blocks-demo']);
</script>

for_each
--------

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

In either case, `ctx` is the original context passed to `for_each` (and has the
same contents).

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

### Item/Data Associations

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

transform
---------

Often, we loop over signal containers so that we can *map* the individual items
to other values. Just as `std::transform` allows you to do this for raw C++
containers, `alia::transform` allows you to do this for signal containers.

`alia::transform` is documented [here](function-application.md#transform), along
with the other signal function application utilities.

alia_for/while
--------------

!> Although alia provides macros for tracking `for` and `while` loops, these do
   *not* integrate naturally into alia applications the way that the `if` and
   `switch` versions do. There's an inherent tension between alia's declarative
   style and the imperative looping constructs like `for` and `while`. While
   alia normally discourages introducing immediate, imperative-style side
   effects into your component-level code, `for` and `while` *depend* on those
   side effects to determine when to terminate the loop. And since alia's
   signals are meant to capture values that change over the life of the
   application (and *not* within a single traversal of your application
   content), `for` and `while` are essentially incompatible with signals.<br>
   <br> If possible, you should use `for_each` (above), since that plays nicely
   with signals and avoids introducing immediate side effects into your
   component-level code. The `alia_for` and `alia_while` macros are provided
   largely as a convenience for applications that are trying to transition to
   alia with a minimum of effort.

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

named_block
-----------

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
