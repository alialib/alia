A Reactive "Hello, World!"
==========================

<script src="asm-dom.js"></script>
<script src="reactive-hello.js"></script>

alia is designed to allow you to write reactive application code that interacts
with traditional, object-oriented C++ libraries. Let's start with a very simple
example:

[source](snippets/greeting_ui.cpp ':include')

This example uses an experimental wrapper for [the asm-dom
library](https://github.com/mbasso/asm-dom). Since asm-dom allows us to write
C++ web UIs, you can see this example in action right here:

<div class="demo-panel">
<div id="greeting-ui"></div>
</div>

?> Many of the examples in this documentation use a mixture of alia and the
asm-dom wrapper. Since alia itself focuses on the mechanics of writing reactive
applications, it doesn't actually *do* anything to interact with the outside
world. It's a bit like a fancy programming language with no I/O capabilities.
It's designed to hook up to other libraries that do useful things, so for these
examples, we've hooked it up to asm-dom. For clarity, anything specific to the
asm-dom wrapper is prefixed with the `dom::` namespace.

If you're familiar with reactive programming, it will be obvious, but the key
takeaway from this example is that when using alia, application code is
developed as **functions that operate on application state and declare the
presence of objects in the presentation of the application**. In this case, our
application presents a web UI, so our function declares what elements are in
that UI, but in other cases, we might declare the objects in a rendering scene
or a physics simulation.

These functions are designed to be reinvoked as the state changes. Each time
they're invoked, they declare what the application looks like **at that point in
time** and **for that particular application state**. In our example, every time
you type into the input box, the `name` state changes and `do_greeting_ui` is
reinvoked to decide what the new UI looks like. Every time, it decides to show a
message asking for your name and the input box that allows you answer, and
sometimes it decides to show a greeting as well.
