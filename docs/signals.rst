Signals
=======

Signals are values that may vary over time (likes those from signal processing).

If you think of an alia application as defining a dataflow graph where the inputs are application state and the outputs go into a back end, then the edges of this graph (where the values live) are all signals.

Flow
----

All signals have a *flow* property. This is a compile-time property which indicates the role that the signal is capable of playing in value flow. There are three possible values for this property:

* **input**: Values can be read from the signal.
* **output**: Values can be written to the signal.
* **inout**: Values can be read from and written to the signal.

Functions that take signals as arguments specify the flow that they require of the signal, and the signals that you provide must meet those requirements. (e.g., If a function expects an input signal, you can pass an input signal or an inout signal, but attempting to pass an output signal will generate a compile-time error.)
