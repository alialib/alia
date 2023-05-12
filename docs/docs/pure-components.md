Pure Components
===============

<script>
    init_alia_demos(['pure-component']);
</script>

By default, alia traverses your entire component tree whenever it needs to
refresh the UI. For modest UIs, this isn't an issue. alia is designed to make
this process fast, especially for cases where not much has changed. However, as
your UI grows more complex, the act of traversing it may become prohibitively
expensive. Pure components are designed to solve this.

Pure components are alia's analogue to pure functions. Just as the result of a
pure function is only dependent on its inputs, the result of a pure component
is only dependent on its inputs and state. And just as we can cache the result
of invoking a pure function on specific inputs, alia can *skip over* a pure
component if it knows that its inputs and state haven't changed.

Through judicious use of pure components, you can change your UI's refresh
complexity from linear to logarithmic, isolating the traversal to just those
components that need updates (and their ancestors).

You don't have to do anything special to mark a component as pure, but you need
to follow two simple rules:

1. Your component function should take all of its arguments (other than the
   context argument) as signals.

1. It should only access data via the context and those signal arguments.

If you follow those rules, you can invoke your component via
`invoke_pure_component`, and alia will skip it whenever possible.

<dl>

<dt>invoke_pure_component(ctx, f, ...args)</dt><dd>

Equivalent to calling `f(ctx, args...)` except that the call will be elided
whenever possible.

`f` is only invoked when:

* One or more of `args` has changed.
* Internal state within `f` has changed.
* An event needs to be delivered to a component within `f`.

</dd>

</dl>

Below is an example that demonstrates the definition and use of a pure
component.

[source](pure.cpp ':include :fragment=pure-component')

<div class="demo-panel">
<div id="pure-component"></div>
</div>

Notice that the invocation count goes up by 1 whenever you change the name but
by 2 when you click the button inside it. Changing an argument to a pure
component only requires one invocation (to refresh). Clicking a button inside
it requires one invocation to deliver the event and then another to refresh it.
