Value Identity
--------------

alia doesn't place any requirements on a signal to have any sort of
*notification* mechanisms. Instead, it relies on polling. When interfacing alia
with a library (like asm-dom), we frequently have to write code that asks "Is
this value the same as the last time we saw it?" For small values like names and
numbers, it's trivial to just store the old value and check it against the new
one. However, just like regular values in C++, a signal value can be arbitrarily
large, and this naive method of detecting changes can become impractical. (If,
for example, we were using alia to display a large image, we wouldn't want to
compare every pixel of the image every frame to make sure that the image hasn't
changed from what's already on screen.)

To address this concern, instead of using a signal's value to detect changes,
alia requires any readable signal to provide a *value identity*. A value
identity is a simple proxy for the actual value that is used purely for
detecting changes.

*If a signal's value changes, its value identity must also change.*

Note that the converse is not true. A signal's value identity is allowed to
change without a change in the actual value of the signal. This would trigger
observers of that signal to unnecessarily reload its value, which would be
inefficient but wouldn't constitute 'incorrect' behavior. (In contrast, if a
signal's value changed without its value identity changing, observers of the
signal wouldn't notice the change and would continue showing a stale view of the
signal, and this *would* be incorrect.)

For smaller values, it's common for a single's value identity to be the value
itself. For larger values, this mechanisms gives the possibility of providing an
abbreviated ID as a proxy for the actual value. Often, these are readily
available in applications that are built on immutable data structures and/or do
revision tracking.

A simple method of generating compact value IDs is to keep an integer counter
and increment it whenever the value changes. Many of alia's built-in signal
types do this.
