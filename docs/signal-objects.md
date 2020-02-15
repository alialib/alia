Signal Objects
==============

In alia, signals are values that vary over time. In other words, alia signals
are like those from signal processing (and *not* like IPC signals, which are
equivalent to events in alia).

If you think of an alia application as defining a dataflow graph where the
inputs are application state and the outputs go into a back end (e.g., are
displayed on the screen), then the edges of this graph (where the values live)
are all signals.

Directionality
--------------

In general, signal values can be read (i.e., polled) and new values can be
written to signals, but not all signals support both operations.

Signals carry a compile-time property that indicates the direction(s) in which
values can flow: read-only, write-only or bidirectional. Similarly, when a
function takes a signal as a parameter, the function signature will specify the
requirements of the signal:

- **readable**: The function may try to read the signal.
- **writable**: The function may try to write to the signal.
- **bidirectional**: The function may try to read from or write to the signal.

When calling a function, the signals that you provide must meet those
requirements. (e.g., If a function expects a readable signal but you try to pass
a signal that only supports writing, a compile-time error will be generated.)

Availability
------------

Even when a signal supports reading, it might not always have a value. There are
any number of reasons why this might be the case:

- The signal is a user input and has no value until the user enters one.
- The signal represents some remote data that is still being retrieved.
- The signal is the result of some computationally intensive calculation that
  hasn't completed yet.

Before trying to read from a signal, you should check that it is actually
readable using `signal_is_readable`.

Similarly, a signal that supports writing isn't necessarily *always* writable.
(e.g., Some data may require the user to explicitly unlock it, or it may be
unwritable during certain sync operations or network outages.) Use
`signal_is_writable` to check if a signal is writable before trying to write a
value to it.

Value Identity
--------------

When interfacing alia with a library (like asm-dom), we frequently have to write
code that asks "Is this value the same as the last time we saw it?" For simple
things like names and numbers, it's trivial to just store the old value and
check it against the new one. For larger values, however, this could become
prohibitively expensive, so alia allows a signal to provide an abbreviated ID
for its value. alia knows that the value remains unchanged as long as that ID
stays the same. Often, these are readily available in applications that are
built on immutable data structures and/or do revision tracking.

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
