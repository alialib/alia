Signals
=======

In alia, signals are values that vary over time. In other words, alia signals are like those from signal processing (and *not* like IPC signals, which are equivalent to events in alia).

..todo:: Add some example code and point out the signals.

If you think of an alia application as defining a dataflow graph where the inputs are application state and the outputs go into a back end (e.g., are displayed on the screen), then the edges of this graph (where the values live) are all signals.



Directionality
--------------

In general, signal values can be read (i.e., polled) and new values can be written to signals, but not all signals support both operations.

..todo:: Add an example of, say, adding two signals and point out how writing to the sum doesn't make sense.

Signals carry a compile-time property that indicates the direction(s) in which values can flow: read-only, write-only or two-way. Similarly, when a function takes a signal as a parameter, the function signature will specify the requirements of the signal:

* **input**: The function may try to read the signal.
* **output**: The function may try to write to the signal.
* **inout**: The function may try to read from or write to the signal.

..todo:: Add an example function declaration.

When calling a function, the signals that you provide must meet those requirements. (e.g., If a function expects an input signal but you try to pass a signal that only supports writing, a compile-time error will be generated.)

..todo:: Add a link to an advanced section about bypassing these checks.

Availability
------------

Even when a signal supports reading, it might not always have a value. There are any number of reasons why this might be the case:

* The signal is a user input and has no value until the user enters one.
* The signal represents some remote data that is still being retrieved.
* The signal is the result of some computationally intensive calculation that hasn't completed yet.

Before trying to read from a signal, you should check that it is actually readable using ``signal_is_readable``.

Similarly, a signal that supports writing isn't necessarily *always* writable. (e.g., Some data may require the user to explicitly unlock it, or it may be unwritable during certain sync operations.) Use ``signal_is_writable`` to check if a signal is writable before trying to write a value to it.

Value Identity
--------------

.. caution:: This is advanced stuff.

Just like regular values in C++, a signal value can be arbitrarily large. For example, a signal could carry a block of text or an image. Since alia uses polling to detect changes in signals, this can be prohibitively expensive for large signal values. (If we were using alia to display a large image, we wouldn't want alia to compare every pixel of the image every frame to make sure that the image we're providing is still the same one that's on the screen.)

To address this concern, any signal with a readable value must also provide a *value identity*.

There is one simple rule governing value identitiies:

*If a signal's value changes, it's value identity must also change.*

(Note that the converse is not true. A signal's value identity can change without a change in value.)

It is common for a single's value identity to be the value itself.

Constructors
------------

``value``

``direct_inout``

``constant``

``empty_signal<Value>()``

Lambdas
^^^^^^^

``lambda_input(is_readable, read)``

``lambda_input(is_readable, read, generate_id)``

``lambda_inout(is_readable, read, is_writable, write)``

``lambda_inout(is_readable, read, is_writable, write, generate_id)``

Adaptors
--------

``signal_cast<Value>(signal)``

