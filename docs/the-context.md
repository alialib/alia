The Context
===========

<script>
    init_alia_demos(['custom-components', 'multiple-custom-components']);
</script>

Almost every function in alia takes a context as its first parameter. The
context is how reactive application code accesses the components of alia: the
data graph, event dispatch info, etc.

The context is designed to be extended with external components to give
applications additional capabilities. All of the examples in this documentation
use a `dom::context`, which is defined by the asm-dom wrapper. (The core alia
context is simply `alia::context`.) `dom::context` extends `alia::context` to
add the ability to interact with asm-dom and control a portion of the DOM.

A context is essentially an unordered collection of components. Components have
compile-time tags (to identify them) and run-time data (so they can do things).
Contexts use [structural
typing](https://en.wikipedia.org/wiki/Structural_type_system), not inheritance.
When calling another function, the context you provide must simply have a
superset of the components that the function requires.

Applications are also free to extend the context. The context can be a good
place to store information and resources that are repeatedly accessed throughout
your application code: the active user, session info, workspace info, I/O
objects, etc. The information doesn't even have to be global to your
application. You might create different contexts for different pages or displays
within the application.

Here's an example of how you might use a custom context to establish a username
as a globally accessible property in your app.

[source](context.cpp ':include :fragment=custom-components')

<div class="demo-panel">
<div id="custom-components"></div>
</div>

The call to `copy_context` ensures that your new context has its own underlying
storage for its components. This isn't always strictly necessary (and it's
unnecessary in this example), but it's a safe practice.

You can also, of course, add multiple components at once...

[source](context.cpp ':include :fragment=multiple-custom-components')

<div class="demo-panel">
<div id="multiple-custom-components"></div>
</div>
