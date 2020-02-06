Signals
=======

In alia, signals are values that vary over time. In other words, alia signals
are like those from signal processing (and *not* like IPC signals, which are
equivalent to events in alia).

.. todo:: Add some example code and point out the signals.

If you think of an alia application as defining a dataflow graph where the
inputs are application state and the outputs go into a back end (e.g., are
displayed on the screen), then the edges of this graph (where the values live)
are all signals.



Directionality
--------------

In general, signal values can be read (i.e., polled) and new values can be
written to signals, but not all signals support both operations.

.. todo:: Add an example of, say, adding two signals and point out how writing
   to the sum doesn't make sense.

Signals carry a compile-time property that indicates the direction(s) in which
values can flow: read-only, write-only or bidirectional. Similarly, when a
function takes a signal as a parameter, the function signature will specify the
requirements of the signal:

* **readable**: The function may try to read the signal.
* **writable**: The function may try to write to the signal.
* **bidirectional**: The function may try to read from or write to the signal.

.. todo:: Add an example function declaration.

When calling a function, the signals that you provide must meet those
requirements. (e.g., If a function expects a readable signal but you try to pass
a signal that only supports writing, a compile-time error will be generated.)

.. todo:: Add a link to an advanced section about bypassing these checks.

Availability
------------

Even when a signal supports reading, it might not always have a value. There are
any number of reasons why this might be the case:

* The signal is a user input and has no value until the user enters one.

* The signal represents some remote data that is still being retrieved.

* The signal is the result of some computationally intensive calculation that
  hasn't completed yet.

Before trying to read from a signal, you should check that it is actually
readable using ``signal_is_readable``.

Similarly, a signal that supports writing isn't necessarily *always* writable.
(e.g., Some data may require the user to explicitly unlock it, or it may be
unwritable during certain sync operations or network outages.) Use
``signal_is_writable`` to check if a signal is writable before trying to write a
value to it.

Value Identity
--------------

Just like regular values in C++, a signal value can be arbitrarily large. For
example, a signal could carry a block of text or an image. Since alia uses
polling to detect changes in signals, this can be prohibitively expensive for
large signal values. (If we were using alia to display a large image, we
wouldn't want alia to compare every pixel of the image every frame to make sure
that the image we're providing is still the same one that's on the screen.)

To address this concern, any signal with a readable value must also provide a
*value identity*.

There is one simple rule governing value identities:

*If a signal's value changes, its value identity must also change.*

(Note that the converse is not true. A signal's value identity can change
without a change in value. This might be inefficient, but it's not incorrect.)

It is common for a single's value identity to be the value itself.

Constructing Signals
--------------------

Basic Constructors
^^^^^^^^^^^^^^^^^^

The following functions allow you to construct basic signals from raw C++
values. These are perfect for working with small values: booleans, numbers,
small strings, and perhaps small data structures. However, all of them rely on
making copies and invoking the equality operator to detect changes in the values
they carry, so they are generally not the best choice for large data structures
unless those structures are especially efficient at these operations.

.. cpp:function:: value(T x)

   Returns a *read-only* signal carrying the value ``x``.

   Internally, the signal stores a *copy* of the value.

.. cpp:function:: value(char const* x)

   Constructs a *read-only* signal carrying a ``std::string`` initialized with
   ``x``.

   The value ID logic for this signal assumes that this overload is only used
   for **string literals** (i.e., that the contents of the string will never
   change). If you're doing real C-style string manipulations, you should
   convert them to ``std::string`` first or use a custom signal.

.. cpp:function:: direct(T& x)

   Returns a *bidirectional* signal carrying the value ``x``.

   Internally, the signal stores a *reference* to the value.

.. cpp:function:: direct(T const& x)

   Returns a *read-only* signal carrying the value ``x``.

   Internally, the signal stores a *reference* to the value.

Lambda Constructors
^^^^^^^^^^^^^^^^^^^

When you need a little more control but don't want to create a custom signal
type, you can create a signal from one or more lambdas functions. (For
completeness, you can create a fully functional, bidirectional signal using
lambdas, but the further you go down this list, the more likely it is that you
should just create a custom signal type.)

.. cpp:function:: lambda_reader(is_readable, read)

   Creates a read-only signal whose value is determined by calling the
   ``is_readable`` and ``read`` lambdas. (Neither takes any arguments.)

   The following is equivalent to ``value(12)``::

      lambda_reader(always_readable, []() { return 12; });

   ``always_readable`` is just a function that always returns ``true``. It's
   considered a clear and concise way to indicate that a signal is always
   readable.

.. cpp:function:: lambda_reader(is_readable, read, generate_id)

   Creates a read-only signal whose value is determined by calling
   ``is_readable`` and ``read`` and whose ID is determined by calling
   ``generate_id``. (None of which take any arguments.)

   With this overload, you can achieve something that's impossible with the
   basic constructors: a signal that carries a large value but doesn't actually
   have to touch that large value every pass. For example::

      lambda_reader(
          always_readable,
          [&]() { return my_object; },
          [&]() { return make_id(my_object.uid); });

   With the above signal, change detection can be done using the object's ID, so
   the object's value itself only has to be touched when new values are
   retrieved.

.. cpp:function:: lambda_bidirectional(is_readable, read, is_writable, write)

   Creates a bidirectional signal whose value is read by calling ``is_readable``
   and ``read`` and written by calling ``is_writable`` and ``write``. Only
   ``write`` takes an argument (the new value).

.. cpp:function:: lambda_bidirectional(is_readable, read, is_writable, write, generate_id)

   Creates a bidirectional signal whose value is read by calling ``is_readable``
   and ``read`` and written by calling ``is_writable`` and ``write``. Its ID is
   determined by calling ``generate_id``. Only ``write`` takes an argument (the
   new value).

The Empty Signal
^^^^^^^^^^^^^^^^

Occasionally, it's useful to create a signal that never carries a value. This is
done with ``empty<T>``::

    auto n = empty<double>();

Now ``n`` can be passed into functions expecting a readable<double>, but it will
never actually provide one.

Operators and Casts
-------------------

Basic Operators
^^^^^^^^^^^^^^^

All the basic operators work as you'd expect with signals, producing signals
that carrying the result of the operation and are only readable when both
arguments are readable.

All the basic arithmetic, bitwise arithmetic/shifting, and comparison operators
are provided:

``+`` ``-`` ``*`` ``/``  ``%`` ``^`` ``&`` ``|`` ``<<`` ``>>`` ``==`` ``!=``
``<`` ``<=`` ``>`` ``>=``

The unary operators ``-`` and ``!`` are also provided.

Logical Operators
^^^^^^^^^^^^^^^^^

The logical operators (``||`` and ``&&``) also work as you'd expect them to work
with signals. Of course, like any library-defined logical operators, they don't
provide the same type of short-circuiting behavior as the built-in versions, but
they provide short-circuiting in the signal space. In particular:

* When the first operand is readable and fully resolves the result of the
  operation, the second operand isn't read.

* When *either* operand is readable and fully resolves the result of the
  operation, the other is allowed to unreadable. The result will still be
  readable.

The Ternary Operator
^^^^^^^^^^^^^^^^^^^^

Since the built-in ternary operator can't be overloaded in C++, alia provides
the equivalent in the form of the ``conditional`` function. It mimics
``std::conditional``:

.. cpp:function:: conditional(b, t, f)

   Yields ``t`` if ``b``'s value is ``true`` (or evaluates similarly in a
   boolean context) and ``f`` if ``b``'s value is ``false`` (or similar).

Mutating Operators
^^^^^^^^^^^^^^^^^^

Operators that would normally mutate one of the operands (i.e., the compound
assignment operators and the increment/decrement operators) instead produce
*actions* in alia and are covered in that section of the documentation.

Operator Strictness
^^^^^^^^^^^^^^^^^^^


Casts
^^^^^

``signal_cast<Value>(signal)``

Subscripts and Field Access
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The subscript operator is defined for signals.

Function Application
--------------------

Maps
^^^^

Other Adaptors
--------------


Creating Custom Signals
-----------------------

Expected Interface
^^^^^^^^^^^^^^^^^^

Utilities
^^^^^^^^^

regular_signal

lazy_reader

Signals As Parameters
---------------------
