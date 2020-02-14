Reactive Control Flow
=====================

<script>
    init_alia_demos(['expanded-greeting', 'addition-analysis']);
</script>

As seen in the 'Hello, World!' example, application code in alia is expressed as
functions that declare the presence of objects in the UI (or other presentation
layer). Whenever the UI needs to be updated, the application essentially gives a
fresh new specification of what the UI should look like at that point in time.
This is the beauty of reactive programming. The application developer can focus
on *what the UI should be now* and not worry about *what the UI was before* or
*how to transform it* from one to the other.

Of course, behind the scenes, we generally don't have this luxury. We can't
simply throw away the old UI every time anything happens and build a fresh new
one according to the application's latest specification. That would be
prohibitively expensive for more complex UIs, and it would likely lead to
discontinuities in the behavior of the UI as all the scrollbar positions, cursor
positions, and other bits of hidden state reset themselves.

Instead, each update, we'd like to

This is where alia comes in. It provides the mechanics to track the relationship
between the specification that's declared by the application code and the
objects that actually exist behind the scenes.

Tracking
--------

alia does this by *maintaining a
graph-like data structure that models the control flow of the reactive portions
of application.* As the application code executes and makes calls to functions
like `do_text` and `do_input`, alia ensures that **each of those calls is
consistently associated with the same node in this graph of data.**

This is why our "Hello, World!" example had to use `alia_if` instead of a
regular C++ `if` statement. Let's look at it again (with a small addition):

[source](greeting.cpp ':include :fragment=expanded-greeting')

<div class="demo-panel">
<div id="expanded-greeting"></div>
</div>

This time, the source code is annotated with the nodes that alia is keeping
track of. Behind the scenes, alia maintains a data graph that looks like this:

![flow](data-graph.svg)

Node `d` is inside the conditional block, so it's not always used, but alia
always knows that it's there, and if we want, it can keep that data around so
that

If it weren't for the `alia_if` macro, it would be impossible (or at least
messy) for alia to distinguish between the call to

As mentioned above, alia's main job is to track the relationship between
the objects declared by your application code and the objects that
actually exist in a library. This job is impossible (or, at best, very
messy) without knowing something about your control flow. As such, alia
asks that you notify it whenever you introduce branching or looping into
the parts of your application that declare objects. There are various
ways to do this, but the simplest is to use some of the built-in
constructs that alia provides, like the `alia_if` statement in the
original example.

Besides allowing alia to do its job, these constructs also play nicely
with signals. For example, imagine we want to add some commentary on the
sum we computed in the addition example:

[source](addition.cpp ':include :fragment=analysis')

<div class="demo-panel">
<div id="addition-analysis"></div>
</div>

Notice that although our `if`/`else` branches have seemingly accounted
for all possibilities on the number line, there is still the possibility
that we haven't filled in the inputs yet and our sum doesn't have a
value. The alia macros account for this automatically, and in that case,
none of the branches are taken.

alia provides a large suite of options for tracking your control flow,
some less invasive than others. You can see them all in the
in-depth guides
&lt;../control-flow/flow-concepts&gt;.
