Flow Basics
===========

The Data Graph
--------------

This section attempts to explain why

Let's take a look at our "Hello, World!" example again (with a small addition):

[source](greeting.cpp ':include :fragment=expanded-greeting')

<div class="demo-panel">
<div id="expanded-greeting"></div>
</div>

Now alia also introduces itself. And this time, the source code is annotated
with the nodes that alia is keeping track of. Behind the scenes, alia maintains
a data graph that looks like this:

![flow](data-graph.svg)

Node `d` is inside the conditional block, so it's not always used, but alia
always knows that it's there, and if we want, it can keep that data around so
that

If it weren't for the `alia_if` macro, it would be impossible (or at least
messy) for alia to distinguish between the call to

