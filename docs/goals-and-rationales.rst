Goals and Rationales
====================

Signals
-------

The goals of the signals component are as follows.

* Unify the interface to C-style data structures and OOP-style classes (whose data members may be protected behind accessor functions).
* Provide standard mechanisms for transforming a back end's view of model  state or applying constraints to its manipulations of that state.
* Provide mechanisms for efficiently detecting changes in signal values. (e.g., It should be possible to poll a signal carrying a large body of text and detect if it has changed since the last poll without examining the entire text.)
* The 'flow' of signals (i.e., input/output/inout) should be explicitly denoted in both the capabilities that a signal implementation provides and the requirements of a function that accepts a signal as a parameter.
* A function that expects an input signal should be able to accept an inout signal without requiring the function to do any template shenanigans. (More specifically, a function that accepts a signal should only have to declare the required flow and the value type of the signal. If both of those are concrete, the function shouldn't need to use templates to accept that signal.)
* Allow signal wrappers to be agnostic of the signal's flow. (e.g., If I want to implement a wrapper that presents a view of a signal where the value is 10x as large, I should have to define what that means for reading and writing the signal, but I shouldn't have to write three different versions in order to support all three flows.)
* Ensure that the passing of signal values across functions is as efficient  and lazy as possible. (i.e., Avoid unnecessary memory allocations and  computations of unused values.)
