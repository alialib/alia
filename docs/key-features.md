Key Features
============

<script>
    init_alia_demos(['addition-ui', 'numerical-analysis']);
</script>

Dataflow Semantics
------------------

In reactive programming, it's useful to think of your application as defining a
dataflow and your variables as carrying values that change over time. In alia,
this type of variable is called a *signal.* If you think of an alia application
as defining a dataflow graph where the inputs are application state and the
outputs go into a presentation layer, then the edges of this graph (where the
values live) are all signals. (In our 'Hello, World!' example, `name` is a
signal.)

The most important difference between an alia signal and a regular C++ variable
is the concept of *availability.* It's often useful to think of a signal in your
dataflow as carrying no value at all (e.g., because the user hasn't input a
value yet, or because the value is still being computed or queried from some
remote source). Since this "not available yet" state tends to propagate through
a dataflow, virtually all code that works with signals has to account for it.
With alia signals, this state is implicitly part of the type and implicitly
propagates through your application's data flow.

For example, let's write a quick app that adds numbers:

[source](numerical.cpp ':include :fragment=addition-ui')

<div class="demo-panel">
<div id="addition-ui"></div>
</div>

As simple as this example is, it's actually setting up a dataflow (via the `+`
operator). Notice that the sum doesn't actually appear until we supply a value
for both `a` and `b`. The result of the `+` operator itself is a signal, and if
either of its inputs is unavailable, that state implicitly propagates through to
the sum.

The Data Graph
--------------

<script>
    init_alia_demos(['addition-ui', 'addition-analysis']);
</script>

As seen in the 'Hello, World!' example, application code in alia is expressed as
functions that declare the presence of objects in the UI (or other presentation
layer). Whenever the UI needs to be updated, the application essentially gives a
fresh new specification of what the UI should look like at that point in time.
This is the beauty of reactive programming. The application developer can focus
on *what the UI should be now* and not worry about *what the UI was before* or
*how to transform it* from one to the other.

Of course, behind the scenes, we generally don't have this luxury. We can't
simply throw away the old UI every time anything happens and build a new one
according to the application's latest specification. That would be prohibitively
expensive for more complex UIs, and it would likely lead to discontinuities in
the behavior of the UI as scrollbar positions, cursor positions, and other bits
of hidden state reset themselves.

Instead, as our application declares the widgets that *should* be in the UI,
we'd like to compare that specification to what *is* there and adjust the UI
accordingly. The mechanics of this are not trivial, which is where alia comes
in. It makes it possible to maintain the relationship between the *declared* UI
and the *actual* UI and efficiently detect where changes have occurred.

alia does this by *maintaining a graph-like data structure that models the
control flow of the reactive portions of application.* As the application code
executes and makes calls to functions like `do_text` and `do_input`, alia
ensures that **each of those calls is consistently associated with the same node
in this data graph.**

In order to make this work, alia asks that you notify it whenever you introduce
branching or looping into the parts of your application that declare objects.
There are various ways to do this, but the simplest is to use some of the
built-in constructs that alia provides, like the `alia_if` statement in the
'Hello, World!' example.

Besides allowing alia to do its job, these constructs also play nicely with
signals. For example, let's write an example that classifies a number as
positive, negative, or zero:

[source](numerical.cpp ':include :fragment=analysis')

<div class="demo-panel">
<div id="numerical-analysis"></div>
</div>

Notice that although our `if`/`else` branches have seemingly accounted for all
possibilities on the number line, there is still the possibility that we haven't
filled in the input yet and `n` doesn't have a value. The alia macros account
for this automatically, and in that case, none of the branches are taken.

alia provides a [large suite of options](tracking-mechanisms.md) for tracking
your control flow, some less invasive than others.
