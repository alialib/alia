Actions
=======

<script>
    init_alia_demos(['unready-copier', 'action-operators', 'action-combining',
        'action-latching']);
</script>

Actions are one way of responding to events in alia. They're intended to be a
convenient, declarative way of specifying the side effects that should be
performed when an event occurs. For the most part, you can think of them as
simple callback functions, but as we'll see, they sometimes integrate more
naturally into reactive code than normal C++ callbacks would...

Actions are very similar to signals in the way that they are used in an
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
operator is only considered ready if its left-hand operand is writable and its
right-hand operand is readable.

When alia is integrated with a UI library, this property can often be leveraged
so that widgets are disabled when the corresponding actions aren't ready to be
performed. The following example shows a DOM button picking up the (un)ready
status of a copy action.

[source](actions.cpp ':include :fragment=unready-copier')

<div class="demo-panel">
<div id="unready-copier"></div>
</div>

Lambda Actions
--------------

When all you need is a simple callback, you can create a lambda action. There
are two options available:

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
