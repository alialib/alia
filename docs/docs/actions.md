Actions
=======

<script>
    init_alia_demos(['unready-copier', 'action-operators', 'action-combining',
        'action-latching', 'action-binding', 'callback-demo']);
</script>

Actions are the preferred way for application code to respond to events in
alia. They're a convenient way of specifying the side effects that should be
performed when an event occurs. For the most part, you can think of them as
simple callback functions, but they often integrate more naturally into
dataflow code than normal C++ callbacks would. (And when you just want to write
a callback function, you can.)

Actions are very similar to signals in the way that they're used in an
application. Like signals, they're typically created directly at the call site
as function arguments and are only valid for the life of the function call.

Signal Operators
----------------

The easiest way to create an action is through signal manipulation. When you
invoke an operator on a signal that would normally cause a mutation, you
instead create an action that performs that mutation. All of the following
compound assignment operators are overloaded in this fashion:

`+=` `-=` `*=` `/=` `%=` `^=` `&=` `|=`

The `++` and `--` operators work similarly. Both the prefix and postfix forms
are provided. However, since the operators themselves don't have any side
effects, there's no difference between the two forms.

The `<<=` operator is treated as a special *copy operator.* It produces an
action that copies the value of its right operand into its left operand.

As with the other (non-mutating) signal operators, wherever possible, these
overloads allow you to mix signals and raw (non-signal) C++ values.

The following example shows all of these forms in action:

[source](actions.cpp ':include :fragment=action-operators')

<div class="demo-panel">
<div id="action-operators"></div>
</div>

Action Readiness
----------------

There is one important difference between an action and a normal C++ callback:

**Like signals, actions have an implicit *unready* state.**

This property often allows actions to integrate more naturally into the
dataflow of your application. For example, the copy action produced by the
`<<=` signal operator is only considered ready if its left-hand operand is
ready to write and its right-hand operand has a value.

When alia is integrated with a UI library, this property can often be leveraged
so that widgets are disabled when the corresponding actions aren't ready to be
performed. The following example shows an HTML button picking up the (un)ready
status of a copy action.

[source](actions.cpp ':include :fragment=unready-copier')

<div class="demo-panel">
<div id="unready-copier"></div>
</div>

Combining Actions
-----------------

Actions can be combined using the `,` operator:

[source](actions.cpp ':include :fragment=action-combining')

<div class="demo-panel">
<div id="action-combining"></div>
</div>

Note that actions are purposefully designed to follow the semantics of latches
from [synchronous digital
circuits](https://en.wikipedia.org/wiki/Synchronous_circuit). Just as a digital
circuit uses logic gates to construct signals that are latched into memory when
the clock ticks, your application constructs signals to carry the values you
need to perform your actions. When your actions are triggered, just like on a
clock tick, all of them are executed *using the values that those signals
carried into the action.*

It's important to keep this in mind when constructing a combined action.
Although the individual actions in a combined action are executed in the order
that they're specified, you shouldn't rely on the effects of actions in the
combination to affect signals going into actions that are later in the sequence
*because they won't.*

You can see this in action here:

[source](actions.cpp ':include :fragment=action-latching')

<div class="demo-panel">
<div id="action-latching"></div>
</div>

Notice that in the combination `(in_hand <<= 0, in_bank += in_hand)`, the
second action is *not* affected by the first.

If you *need* to express imperative sequencing of effects, you should do it as
normal C++ code inside a single action (or in a background task that you
dispatch as an action).

Parameters & Binding
--------------------

Actions can be parameterized. For example, you might define an action that
takes a string as a parameter and sends it as a message in a chat application.

When an action takes one or more parameters, you can use the `<<` operator to
*bind* a signal to the first argument in that action. For example, alia's
animation timer provides a parameterized action for starting it:

[source](actions.cpp ':include :fragment=action-binding')

<div class="demo-panel">
<div id="action-binding"></div>
</div>

Just like when we apply the `<<=` operator to copy signals, the action that
results from binding a signal to an action parameter will only be ready when
the action itself is ready and the signal has a value.

Custom Actions
--------------

Actions can also call C++ functions to produce external side effects. This is
typically how you would integrate custom side effects into your declarative
application logic. Just like the functions that you [apply to
signals](function-application.md) are *normal C++ functions with no side
effects*, the functions that you use to define actions are *normal C++
functions **with side effects***.

For most purposes, custom actions can be specified using lambdas (or other
function objects). There are two options for defining lambda actions:

<dl>

<dt>callback(perform)</dt><dd>

Creates an action that's equivalent to a simple C++ callback.

`perform` is any callable C++ object.

The action is always considered ready and simply calls `perform` when invoked.

`perform` can take any number/type of arguments and defines the signature of
the action.

</dd>

<dt>callback(is_ready, perform)</dt><dd>

Creates an action whose behavior is defined by `is_ready` and `perform`.

`is_ready` takes no arguments and simply returns true or false to indicate if
the action is ready to be performed.

`perform` can take any number/type of arguments and defines the signature of
the action.

</dd>

</dl>

Actions that are defined this way can have parameters, and of course those
parameters can be bound to signals using the `<<` operator:

[source](actions.cpp ':include :fragment=callback-demo')

<div class="demo-panel">
<div id="callback-demo"></div>
</div>

If you want to do something more interesting, you may want to implement the
`action_interface`. The interface itself and example implementations can be
found in
[actions.hpp](https://github.com/alialib/alia/blob/main/src/alia/flow/actions.hpp).

Action Library
--------------

alia provides a small library of actions that are generally useful.

This library resides in the `alia::actions` namespace. This is meant to avoid
confusion between functions that create actions and functions that are actually
imperative.

<dl>

<dt>noop()</dt><dd>

`actions::noop()` creates an action that is always ready to perform but does
nothing.

This can be useful when you are mocking up a UI and aren't ready to fill in
real actions.

You can optionally provide type parameters to indicate the types of the
action's parameters. e.g., `actions::noop<int>()` creates an action that takes
a single parameter of type `int`.

</dd>

<dt>unready()</dt><dd>

`actions::unready()` creates an action that is never ready to perform.

Similar to `noop`, you can provide optional type parameters.
`actions::unready<int>()` creates an action that takes a single parameter of
type `int`.

</dd>

<dt>apply(f, state, [arg])</dt><dd>

`actions::apply` is used when you want to apply a function to a state signal to
transform it to a new state.

When the action is performed, `state` is passed into `f` as an argument (along
with `arg`, if provided), and the result of `f` is written back to `state`. If
`state` allows it, its value will be moved out and back in, so if `f` is
written with normal value semantics, this action won't actually invoke any copy
constructors for the state.

This is roughly equivalent to the following:

```cpp
state <<= lazy_apply(f, alia::move(state), arg);
```

</dd>

<dt>toggle(signal)</dt><dd>

`actions::toggle(signal)` creates an action that toggles `signal` by applying
the `!` operator.

This is equivalent to `signal <<= !signal`.

</dd>

<dt>push_back(container)</dt><dd>

`actions::push_back(container)`, where `container` is a signal carrying a
container, creates an action that takes a compatible item as a parameter and
pushes it onto the back of `container`.

</dd>

<dt>erase_index(container, index)</dt><dd>

`actions::erase_index(container, index)`, where `container` is a signal
carrying a random access container and `index` is a signal (or raw value)
carrying a `size_t`, creates an action that erases the item at `index` from
`container`.

</dd>

<dt>erase_key(container, key)</dt><dd>

`actions::erase_key(container, key)`, where `container` is a signal carrying an
associative container and `index` is a signal (or raw value) carrying a key for
that container, creates an action that erases `key` from `container`.

</dd>

</dl>

'Consuming' Actions
-------------------

Actions are passed around via the `action` type. It's parameterized over the
types of arguments that the action takes, so an action with one `int` parameter
would be `action<int>`, and an action with no parameters would be `action<>`,
e.g.:

```cpp
void
button(my_context ctx, readable<std::string> text, action<> on_click)
{
    ...
}
```

You can determine if an action is ready to be performed by calling its
`is_ready()` member function (or using the free function `action_is_ready`),
and you can invoke it using the free function `perform_action` (e.g.,
(`perform_action(on_click)`)).
