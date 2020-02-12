Actions
=======

<script>
    init_alia_demos(['unready-copier', 'action-operators']);
</script>

Actions are one way of responding to events in alia. They're intended to be a
convenient, declarative way of specifying the side effects that should be
performed when an event occurs. For the most part, you can think of them a lot
like a simple callback function, but as we'll see, they sometimes integrate more
naturally into reactive code than a normal C++ callback would...

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
action that copies the value of its right operand into the left operand.

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
operator is only considered ready if its left-hand operand is writeable and its
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
