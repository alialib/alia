High-Level Flow
===============

<script>
    init_alia_demos(['hello-button']);
</script>

The flow of a declarative user interface system generally looks something like
this:

![flow](declarative-flow.svg)

The important part is that UI event handlers don't directly manipulate the UI
but instead translate those events to *changes in application state*, which are
then reflected in the UI via the declarative UI specification.

In alia, both the declarative UI specification and the event handlers are part
of the *controller function:*

![flow](alia-flow.svg)

The controller function is just the top-level component in a user interface.
From a high-level perspective, it serves two purposes:

1. It declares what components are present in the user interface.

2. It delivers events to those components.

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
