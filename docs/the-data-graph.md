The Data Graph
==============

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

