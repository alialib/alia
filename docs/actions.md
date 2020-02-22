Actions
=======

<script>
    init_alia_demos(['unready-copier', 'action-operators', 'action-combining',
        'action-latching', 'action-binding', 'lambda-action-demo']);
</script>

Actions are the preferred way for application code to respond to events in alia.
They're a convenient, declarative way of specifying the side effects that should
be performed when an event occurs. For the most part, you can think of them as
simple callback functions, but as we'll see, they sometimes integrate more
naturally into reactive code than normal C++ callbacks would...

Actions are very similar to signals in the way that they're used in an
application. Like signals, they're typically created directly at the call site
as function arguments and are only valid for the life of the function call.

Signal Operators
----------------

The easiest way to create an action is through signal manipulation. When you
invoke an operator on a signal that would normally cause a mutation, you instead
create an action that performs that mutation. All of the following compound
assignment operators are overloaded in this fashion:

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

This property often allows actions to integrate more naturally into the dataflow
of your application. For example, the copy action produced by the `<<=` signal
operator is only considered ready if its left-hand operand is ready to write and
its right-hand operand has a value.

When alia is integrated with a UI library, this property can often be leveraged
so that widgets are disabled when the corresponding actions aren't ready to be
performed. The following example shows a DOM button picking up the (un)ready
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

Notice that in the combination `(in_hand <<= 0, in_bank += in_hand)`, the second
action is *not* affected by the first.

If you *need* to express imperative sequencing of effects, you should do it as
normal C++ code inside a single action (or in a background task that you
dispatch as an action).

Parameters & Binding
--------------------

Actions can be parameterized. For example, you might define an action that takes
a string as a parameter and sends it as a message in a chat application.

When an action takes one or more parameters, you can use the `<<=` operator to
*bind* a signal to the first argument in that action. For example, alia's
animation timer provides a parameterized action for starting it:

[source](actions.cpp ':include :fragment=action-binding')

<div class="demo-panel">
<div id="action-binding"></div>
</div>

Just like when we apply the `<<=` operator to copy signals, the action that
results from binding a signal to an action parameter will only be ready when the
action itself is ready and the signal has a value.

Custom Actions
--------------

For most purposes, custom actions can be specified using lambdas (or other
function objects). There are two options for defining lambda actions:

<dl>

<dt>lambda_action(perform)</dt><dd>

Creates an action that calls `perform`.

`perform` can take any number/type of arguments and defines the signature of the
action.

The action is always ready to be performed.

</dd>

<dt>lambda_action(is_ready, perform)</dt><dd>

Creates an action whose behavior is defined by `is_ready` and `perform`.

`is_ready` takes no parameters and simply returns true or false to indicate if
the action is ready to be performed.

`perform` can take any number/type of arguments and defines the signature of the
action.

</dd>

</dl>

Actions that are defined this way can take parameters, and of course those
parameters can be bound to signals using the `<<=` operator:

[source](actions.cpp ':include :fragment=lambda-action-demo')

<div class="demo-panel">
<div id="lambda-action-demo"></div>
</div>

If you want to do something more interesting, you may want to implement the
`action_interface`. The interface itself and example implementations can be
found in
[actions.hpp](https://github.com/tmadden/alia/blob/master/src/alia/flow/actions.hpp).

Action 'Library'
----------------

alia provides a small 'library' of actions that are generally useful:

<dl>

<dt>toggle(signal)</dt><dd>

Creates an action that toggles `signal` by applying the `!` operator.

This is exactly equivalent to `signal <<= !signal`.

</dd>

<dt>push_back(container)</dt><dd>

`push_back(container)`, where `container` is a signal carrying a container,
creates an action that takes a compatible item as a parameter and pushes it onto
the back of `container`.

</dd>

</dl>

Invoking Actions
----------------
