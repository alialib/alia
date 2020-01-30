Introduction
============

alia is designed to allow you to write reactive application code that interacts
with traditional, object-oriented C++ libraries. Let's start with a very simple
example::

    void
    do_greeting_ui(dom::context ctx, bidirectional<std::string> name)
    {
        // Allow the user to input their name.
        dom::do_input(ctx, name);

        // If we have a name, greet the user.
        alia_if (name != "")
        {
            dom::do_text(ctx, "Hello, " + name + "!");
        }
        alia_end
    }

This example uses an experimental wrapper for `the asm-dom library
<https://github.com/mbasso/asm-dom>`_. Since asm-dom allows us to write C++ web
UIs, you can see this example in action right here:

.. todo:: insert example

Most of the examples that you'll see in this documentation use a mixture of alia
and the experimental asm-dom wrapper. Since alia itself focuses on the mechanics
of writing reactive applications, it doesn't actually *do* anything to interact
with the world. It's a bit like a fancy programming language with no I/O
capabilities. It's designed to hook up to other libraries that do useful things,
so for these examples, we've hooked it up to asm-dom. For clarity, anything
specific to the asm-dom wrapper is prefixed with the ``dom::`` namespace.

If you're familiar with reactive programming, it will be obvious, but the key
takeaway from this example is that when using alia, application code is
developed as **functions that operate on application state and declare the
presence of objects in the presentation of the application**. In this case, our
application presents a web UI, so our function declares what widgets are in that
UI, but in other cases, we might declare the objects in a game world or a
physics simulation.

These functions are designed to be reinvoked as the state changes. Each time
they're invoked, they declare what the application looks like **at that point
in time** and **for that particular application state**. In our example, every
time you type into the input box, the ``name`` state changes and
``do_greeting_ui`` is reinvoked to decide what the new UI looks like. Every
time, it decides to show the input box that allows you to edit ``name``, and
sometimes it decides to show a greeting as well.

Of course, behind the scenes, we're not completely rebuilding the UI every time
anything changes. That would be prohibitively expensive for more complex UIs,
and it would likely lead to discontinuities in the behavior of the UI. Instead,
as our application declares the widgets that *should* be in the UI, the asm-dom
wrapper compares that specification to what *is* there and adjusts the UI
accordingly. The mechanics of maintaining this relationship are not trivial,
which is where alia comes in. It makes it possible to maintain this relationship
and compute these 'diffs' efficiently. To facilitate this, it introduces some
concepts into the application-level code. We'll touch on these below.

Signals
-------

You might have noticed that our ``name`` parameter is declared as a
``bidirectional<string>`` rather than just a normal std::string. In alia
parlance, ``name`` is a *signal*, and since it's "bidirectional", we know that
``do_greeting_ui`` can both read from it and write to it.

Control Flow Tracking
---------------------

Actions
-------
