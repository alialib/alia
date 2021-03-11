"Hello, World!"
===============

<script>
    init_alia_demos(['greeting-ui']);
</script>

Since alia is designed to write interactive applications, let's start with an
interactive version of the traditional 'Hello, World!':

[source](greeting.cpp ':include :fragment=greeting')

This example uses alia/HTML, which allows us to develop web apps in alia, so
you can see it in action right here:

<div class="demo-panel">
<div id="greeting-ui"></div>
</div>

?> We'll use alia/HTML a lot here so that we can demonstrate alia's core
   capabilities in your browser. For clarity, most of the examples make very
   explicit use of the namespaces `alia::` and `html::` to help illustrate
   what's part of the core of alia and what's alia/HTML. (Normal code would be
   slightly less verbose.)

If you're familiar with other declarative UI programming frameworks, it will be
obvious, but the key takeaway from this example is that when using alia,
application code is developed as **functions that operate on application state
and declare the presence of objects in the UI**.

These functions are designed to be reinvoked as the state changes. Each time
they're invoked, they declare what the application looks like **at that point
in time** and **for that particular application state**. In our example, every
time you type into the input box, the `name` state changes and `greeting_ui` is
reinvoked to decide what the new UI looks like. Every time, it decides to show
a message asking for your name and the input box that allows you to answer, and
sometimes it decides to show a greeting as well.

alia provides the mechanics to facilitate this style of development...
