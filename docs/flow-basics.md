Flow Basics
===========

<script>
    init_alia_demos(['hello-button', 'expanded-greeting']);
</script>

High-Level Flow
---------------

The flow of a reactive user interface system typically looks like this:

![flow](reactive-flow.svg)

The important part (and what makes it reactive) is that UI event handlers don't
directly manipulate the UI but instead translate those events to *changes in
application state*, which are then reflected in the UI via the reactive UI
specification.

In alia, both the reactive UI specification and the event handlers are part of
the *controller function:*

![flow](alia-flow.svg)

The top-level interface to an alia application is always a controller function.
(And individual components are also implemented as controller functions, which
makes it simple to compose applications.) The controller function serves two
main purposes:

1. It declares what elements are present in the user interface.

2. It routes events to those elements.

Here's a very simple example that demonstrates both of these roles:

[source](greeting.cpp ':include :fragment=hello-button')

<div class="demo-panel">
<div id="hello-button"></div>
</div>

When you interact with this demo, the sequence of events is as follows:

1. The asm-dom wrapper invokes `do_demo` to determine the initial contents of
   the UI, and in response to what `do_demo` declares, a 'Toggle the Message'
   button is created.

2. At some point, you (the user) click the button and the asm-dom wrapper
   invokes `do_demo` again with a button click event. `do_demo` calls
   `dom::do_button`, and `dom::do_button` recognizes that the button click event
   is meant for it, so it invokes its [action](actions.md), which toggles the
   state of `show_message`.

3. Immediately after this, the asm-dom wrapper invokes `do_demo` *again* to
   refresh the UI. `do_demo` again declares that the 'Toggle the Message' button
   should be there, but since `show_message` is now set to `true`, it *also*
   declares that the 'Hello, World!' message should be included.

... And so on.

Note that all of this is transparent to `do_demo`. The event that's being
transported through the UI tree is tucked away in the context argument. All
`do_demo` has to do is declare what should be there and what it should do. The
leaves of the tree (e.g., `dom::do_button`) worry about what type of event is
being processed and how to respond to it. (In fact, steps 1 and 3 above are
actually just 'refresh' events. `dom::do_button` responds to refresh events by
making sure that there is a button in the DOM where it should be and that the
label is correct.)

The Data Graph
--------------

Much of the power of alia comes from the fact that function calls are allowed to
maintain their own internal data. In order to make this happen, alia maintains a
'data graph' behind the scenes that mimics the control flow graph of the
reactive portions of the application code. As that code executes, alia follows
along in the data graph and returns the correct node to each bit of code that
requests one, ensuring that they every function call is consistently given the
same data node.

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
and `b` are obviously pretty simple to keep track of. Every time our function is
invoked, `a` is the first node encountered and `b` is the second. However, if it
weren't for the `alia_if` macro, alia wouldn't know what to expect next.
Sometimes it would see `d` and other times `e`. This would cause confusion
whenever `d` appeared and disappeared. Sometimes `d` would get `e`'s data, or
vice versa. Imagine how disjointing this would be if `d` and `e` were different
types of widgets, or if they had internal state for selected items, scrollbar
positions, etc.

The `alia_if` macro solves this problem by creating a fork in the data graph. It
maintains a separate subtree for the code scoped inside it (its 'then' block),
and it ensures that any requests for data from inside that block go to the
subtree and don't interfere with the rest of the data graph.

Flow Restrictions
-----------------

In order to allow alia to do its job maintaining the data graph, you have to
follow some simple rules:

1. Wherever you have loops or conditionals in your reactive application code,
   use the alia flow tracking mechanisms in the following sections to allow alia
   to track it.

2. Don't `return` from the middle of a function. - alia doesn't actually know
   when you enter and leave functions, so if you suddenly leave a function at a
   different spot in the data graph, this confuses it.

   ?> In the near future, mechanisms will be added to allow this, but since most
      controller functions return `void` anyway, it's usually trivial to rewrite
      your code to avoid `return`.

3. Don't use `goto`.

Note that these rules only apply to the *reactive* portions of your application
code (i.e., the parts that declare what elements of your user interface are
present). alia is designed so that the code that [performs
computations](function-application.md) and the code that [produces side
effects](actions.md#custom-actions) can be written as normal C++, without
worrying about flow tracking/restrictions.
