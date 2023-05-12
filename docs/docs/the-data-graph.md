The Data Graph
==============

<script>
    init_alia_demos(['expanded-greeting']);
</script>

Much of the power of alia comes from the fact that function calls are allowed
to maintain their own internal data. In order to make this happen, alia
maintains a 'data graph' behind the scenes that mimics the control flow graph
of the component-level portions of the application code. As that code executes,
alia follows along in the data graph and returns the correct node to each bit
of code that requests one, ensuring that every function call is consistently
given the same data node.

Let's take a look back at our "Hello, World!" example (with a small addition):

[source](greeting.cpp ':include :fragment=expanded-greeting')

<div class="demo-panel">
<div id="expanded-greeting"></div>
</div>

Now alia also introduces itself. And this time, the source code is annotated
with the nodes that alia is keeping track of. Behind the scenes, alia maintains
a data graph that looks like this:

![flow](data-graph.svg)

Hopefully now it's becoming clearer why the `alia_if` macro exists... Nodes `a`
and `b` are obviously pretty simple to keep track of. Every time our function
is invoked, `a` is the first node encountered and `b` is the second. However,
if it weren't for the `alia_if` macro, alia wouldn't know what to expect next.
Sometimes it would see `d` and other times `e`. This would cause confusion
whenever `d` appeared and disappeared. Sometimes `d` would get `e`'s data, and
vice versa. Imagine how disjointing this would be if `d` and `e` were different
types of widgets, or if they had internal state for selected items, scrollbar
positions, etc.

The `alia_if` macro solves this problem by creating a fork in the data graph.
It maintains a separate subtree for the code scoped inside it (its 'then'
block), and it ensures that any requests for data from inside that block go to
the subtree and don't interfere with the rest of the data graph.

alia provides a whole suite of similar mechanisms that aim to allow you to
easily represent familiar C++ control flow patterns in a way that alia can keep
track of...
