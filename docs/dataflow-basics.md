Dataflow & Signals
==================

<script>
    init_alia_demos(['addition-ui', 'addition-analysis']);
</script>

Signals
-------

You might have noticed that our `name` parameter is declared as a
`bidirectional<std::string>` rather than `std::string&`, as you might
have expected. In alia parlance, `name` is a *signal*, and since it's
*bidirectional*, `do_greeting_ui` can both read from it and write to it.
In reactive programming, it's useful to think of your application as
defining a dataflow and your variables as carrying values that change
over time (like a signal in signal processing). In alia, this concept is
captured by signal types, and they differ from normal C++ values in two
important ways:

### Availability

It's often useful to think of a signal in your dataflow as carrying no
value at all (e.g., because the user hasn't input a value yet, or
because the value is still being computed or queried from some remote
source). Since this "not available yet" state tends to propagate through
a dataflow, virtually all code that works with signals has to account
for it. With alia signals, this state is implicitly part of the type and
implicitly propagates through your application's data flow.

For example, let's write a quick app that adds numbers:

[source](addition.cpp ':include :fragment=ui')

<div class="demo-panel">
<div id="addition-ui"></div>
</div>

As simple as this example is, it's actually setting up a dataflow (via
the `+` operator). Notice that the sum doesn't actually appear until we
supply a value for both `a` and `b`. The result of the `+` operator
itself is a signal, and if either of its inputs is unavailable, that
state implicitly propagates through to the sum.

### Value Identification

When interfacing alia with a library (like asm-dom), we frequently have
to write code that asks "Is this value the same as the last time we saw
it?" For simple things like names and numbers, it's trivial to just
store the old value and check it against the new one. For larger values,
however, this could become prohibitively expensive, so alia allows a
signal to provide an abbreviated ID for its value. alia knows that the
value remains unchanged as long as that ID stays the same. Often, these
are readily available in applications that are built on immutable data
structures and/or do revision tracking.

You can read much more about signals in the in-depth guides
&lt;../signals/introduction&gt;.

