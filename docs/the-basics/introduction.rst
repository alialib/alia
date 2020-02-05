Introduction
============

.. raw:: html

   <script src="../asm-dom.js"></script>
   <script src="../introduction.js"></script>

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

.. raw:: html

   <div class="demo-panel">
   <div id="greeting-ui"></div>
   </div>

Most of the examples that you'll see in this documentation use a mixture of alia
and the asm-dom wrapper. Since alia itself focuses on the mechanics of writing
reactive applications, it doesn't actually *do* anything to interact with the
outside world. It's a bit like a fancy programming language with no I/O
capabilities. It's designed to hook up to other libraries that do useful things,
so for these examples, we've hooked it up to asm-dom. For clarity, anything
specific to the asm-dom wrapper is prefixed with the ``dom::`` namespace.

The Reactive Paradigm
---------------------

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

alia's Role
-----------

Of course, behind the scenes, we're not completely rebuilding the UI every time
anything changes. That would be prohibitively expensive for more complex UIs,
and it would likely lead to discontinuities in the behavior of the UI. Instead,
as our application declares the widgets that *should* be in the UI, the asm-dom
wrapper compares that specification to what *is* there and adjusts the UI
accordingly. The mechanics of maintaining this relationship are not trivial,
which is where alia comes in. It makes it possible to maintain this relationship
and compute these 'diffs' efficiently. To facilitate this, it introduces some
of its own concepts into the application-level code, which we'll touch on here.

Signals
^^^^^^^

You might have noticed that our ``name`` parameter is declared as a
``bidirectional<std::string>`` rather than ``std::string&``, as you might have
expected. In alia parlance, ``name`` is a *signal*, and since it's
*bidirectional*, ``do_greeting_ui`` can both read from it and write to it. In
reactive programming, it's useful to think of your application as defining a
dataflow and your variables as carrying values that change over time (like a
signal in signal processing). In alia, this concept is captured by signal types,
and they differ from normal C++ values in two important ways:

Availability
++++++++++++

It's often useful to think of a signal in your dataflow as carrying no value at
all (e.g., because the user hasn't input a value yet, or because the value is
still being computed or queried from some remote source). Since this "not
available yet" state tends to propagate through a dataflow, virtually all code
that works with signals has to account for it. With alia signals, this state is
implicitly part of the type and implicitly propagates through your application's
data flow.

For example, let's write a quick app that adds numbers::

  void
  do_addition_ui(
      dom::context ctx, bidirectional<double> a, bidirectional<double> b)
  {
      dom::do_input(ctx, a);
      dom::do_input(ctx, b);
      dom::do_text(ctx, a + b);
  }

.. raw:: html

   <div class="demo-panel">
   <div id="addition-ui"></div>
   </div>

As simple as this example is, it's actually setting up a dataflow (via the ``+``
operator). Notice that the sum doesn't actually appear until we supply a value
for both ``a`` and ``b``. The result of the ``+`` operator itself is a signal,
and if either of its inputs is unavailable, that state implicitly propagates
through to the sum.

Value Identification
++++++++++++++++++++

When interfacing alia with a library (like asm-dom), we frequently have to write
code that asks "Is this value the same as the last time we saw it?" For simple
things like names and numbers, it's trivial to just store the old value and
check it against the new one. For larger values, however, this could become
prohibitively expensive, so alia allows a signal to provide an abbreviated ID
for its value. alia knows that the value remains unchanged as long as that ID
stays the same. Often, these are readily available in applications that are
built on immutable data structures and/or do revision tracking.

You can read much more about signals in the :doc:`in-depth guide
<../in-depth/signals>`.

Control Flow Tracking
^^^^^^^^^^^^^^^^^^^^^

As mentioned above, alia's main job is to track the relationship between the
objects declared by your application code and the objects that actually exist in
a library. This job is impossible (or, at best, very messy) without knowing
something about your control flow. As such, alia asks that you notify it
whenever you introduce branching or looping into the parts of your application
that declare objects. There are various ways to do this, but the simplest is to
use some of the built-in constructs that alia provides, like the ``alia_if``
statement in the original example.

Besides allowing alia to do its job, these constructs also play nicely with
signals. For example, imagine we want to add some commentary on the sum we
computed in the addition example::

  void
  do_addition_ui(
      dom::context ctx, bidirectional<double> a, bidirectional<double> b)
  {
      dom::do_input(ctx, a);
      dom::do_input(ctx, b);

      auto sum = a + b;
      dom::do_text(ctx, sum);
      alia_if (sum > 0)
      {
          dom::do_text(ctx, "The sum is positive!");
      }
      alia_else_if (sum < 0)
      {
          dom::do_text(ctx, "The sum is negative!");
      }
      alia_else
      {
          dom::do_text(ctx, "The sum is zero!");
      }
      alia_end
  }

Notice that although our ``if``/``else`` branches have seemingly accounted for
all possibilities on the number line, there is still the possibility that we
haven't filled in the inputs yet and our sum doesn't have a value. The alia
macros account for this automatically, and in that case, none of the branches
are taken.

alia provides a large suite of options for tracking your control flow, some less
invasive than others. You can see them all in the :doc:`in-depth guide
<../in-depth/control-flow>`.
