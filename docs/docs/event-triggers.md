Event Triggers
==============

Most events come from the outside world. (See, for example, the section on
[HTML events](elements.md#event-handling).) However, there are a few events
that alia understands even at its core. This page documents how you can respond
to them in your component-level code.

Initialization
--------------

<dl>

<dt>on_init(ctx, action)</dt><dd>

Declares that `action` should be performed when the surrounding component is
initialized.

The action will be performed once and only once for the component.

Note that if the action is initially unready, it will still be performed once
it's ready.

</dd>

<dt>on_activate(ctx, action)</dt><dd>

Declares that `action` should be performed when the surrounding component is
activated.

Activation occurs whenever the component is introduced *or reintroduced* into
the UI. For example, if a component lives inside an `alia_if` block, activation
will occur whenever the `alia_if` condition transitions into the `true` state.
Initialization, on the other hand, will only occur once.

Note that if the action is unready when activation occurs, it will be performed
as soon as it's ready.

</dd>

</dl>

Refresh Events
--------------

<dl>

<dt>refresh_handler(ctx, handler)</dt><dd>

Declares that `handler` should be called whenever the component is refreshed.
`handler` is a regular C++ function object and is invoked as
`handler(dataless_ctx)`, where `dataless_ctx` is a reduced-capabilities version
of the original `ctx` argument.

The purpose of passing a dataless context back to your handler is that this
helps prevent you from calling component-level code that interacts with the data
graph and thus shouldn't be invoked from within an event handler. For example:

```cpp
refresh_handler(ctx, [&](auto ctx) {
    html::p(ctx); // Error because `ctx` doesn't have access to the data graph.
});
```

</dd>

</dl>

Timers
------

Coming soon...
