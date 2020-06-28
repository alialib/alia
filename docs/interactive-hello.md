"Hello, World!"
===============

<script>
    init_alia_demos(['greeting-ui']);
</script>

Since alia is designed to write interactive applications, let's start with an
interactive version of the traditional 'Hello, World!':

[source](greeting.cpp ':include :fragment=greeting')

This example uses an experimental wrapper for [the asm-dom
library](https://github.com/mbasso/asm-dom). Since asm-dom allows us to write
C++ web UIs, you can see this example in action right here:

<div class="demo-panel">
<div id="greeting-ui"></div>
</div>

Many of the examples in this documentation use a mixture of alia and the asm-dom
wrapper. Since alia itself focuses on the mechanics of writing interactive
applications in a declarative fashion, it doesn't actually *do* anything to
interact with the outside world. It's a bit like a fancy programming language
with no I/O capabilities. It's designed to hook up to other libraries that do
useful things, so for these examples, it's hooked up to asm-dom. For clarity,
anything specific to the asm-dom wrapper is prefixed with the `dom::` namespace,
and the `alia::` namespace is explicitly used in many cases where it wouldn't be
in real code.

If you're familiar with other declarative UI programming frameworks, it will be
obvious, but the key takeaway from this example is that when using alia,
application code is developed as **functions that operate on application state
and declare the presence of objects in the UI**.

These functions are designed to be reinvoked as the state changes. Each time
they're invoked, they declare what the application looks like **at that point in
time** and **for that particular application state**. In our example, every time
you type into the input box, the `name` state changes and `greeting_ui` is
reinvoked to decide what the new UI looks like. Every time, it decides to show a
message asking for your name and the input box that allows you to answer, and
sometimes it decides to show a greeting as well.

alia provides the mechanics to facilitate this style of development...
