The Context
===========

<script>
    init_alia_demos(['custom-context']);
</script>

!> Contexts are in the midst of change! The basic concepts are still the same,
   but the examples here is incomplete. Beware!

Almost every function in alia takes a context as its first parameter. The
context is how application code accesses the utilities provided by alia: the
data graph, event dispatch info, etc.

The context is designed to be extended with externally defined objects to give
applications additional capabilities. All of the examples in this documentation
use a `dom::context`, which is defined by the asm-dom wrapper. `dom::context`
extends `alia::context` to add the ability to interact with asm-dom and control
a portion of the DOM.

A context is essentially an unordered collection of objects. Context objects
have compile-time tags (to identify them) and run-time data (so they can do
things). Contexts use [structural
typing](https://en.wikipedia.org/wiki/Structural_type_system). When calling
another function, the context you provide must simply have a superset of the
objects that the function requires. (Unlike what might happen in an
inheritance-based system, it doesn't matter what order you add the objects to
your context.)

Applications are also free to extend the context. The context can be a good
place to store information and resources that are repeatedly accessed throughout
your application code: the active user, session info, workspace info, I/O
objects, etc. The information doesn't even have to be global to your
application. You might create different contexts for different pages or displays
within your application.

Here's an example of how you might use a custom context to establish a username
as a globally accessible property in your app.

[source](context.cpp ':include :fragment=custom-context')

<div class="demo-panel">
<div id="custom-context"></div>
</div>
