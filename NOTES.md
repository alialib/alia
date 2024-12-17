# A CROSS-LANGUAGE VISION FOR ALIA

What would it look like if alia defined a C API and were usable from any
low-level language? (e.g., C, C++, Rust, etc.)

Certainly some aspects of alia are C++ specific (and mostly have counterparts
in Rust), but what could be captured in C?

Could you even develop a single application in multiple languages?

## Defining the Core Functionality

What is actually offered by alia?

- the data graph: a data structure that is implicitly defined by the control
  flow graph of your functional component composition

- a dataflow system: "signals", lazy/cached operations on those signals,
  background computation/retrieval of signal values

- an event system: a way of delivering events through the functional component
  composition

- an "action" system: a way of declaratively specifying event handlers that
  integrates nicely with the event system

- a UI system: a rendering and input-handling system that uses the above to
  implement UIs

- a UI library: a set of common UI components written more-or-less at the user
  level

- external integrations: a way of driving external OO systems in a declarative
  way

- debugging tools: tools that utilize the UI to understand what is happening in
  the various systems

## What Could Be Done in C

### The Data Graph

The data structure is already very C-like. It would benefit from a more
C-oriented approach to memory management (e.g., arenas).

*ISSUE*: With alia_if and similar constructors, we can't predict how big the
data graph will be in inactive branches. Similarly when we add, remove, or
reorder blocks in a for loop.

POSSIBLE RESOLUTION: Allow fragmentation to occur but monitor and defragment
during idle time when possible.

POSSIBLE RESOLUTION: Allow `alia_if` blocks to be marked as "safe for
exploration", which could allow the refresh pass to enter those blocks and
iterate over their data nodes without actually emitting any content.
(Perhaps not worth the added effort (by both the programmer and the CPU).)

**ISSUE**: What about storing strings? These are pretty fundamental. How do we
handle dynamically resizing them?

ISSUE: The C++ code currently supports destructors, and even takes pains to
ensure that the destructors are called in the appropriate order.

RESOLUTION: We could accomodate this by storing a 'destructor' function
pointer:

https://herbsutter.com/2016/09/25/to-store-a-destructor/

struct destructor {
    const void* p;
    void(*destroy)(const void*);
};

// Called indirectly from deferred_heap::make<T>.
// Here, t is a T&.
dtors.push_back({
    std::addressof(t), // address of object
    [](const void* x) { static_cast<const T*>(x)->~T(); }
});                    // dtor to invoke

And it would be simple enough to keep the reverse order of the destructor
invocation. This wouldn't be very cache-friendly, but it's not a per-frame
operation. (But it would affect update times. Ideally it would actually be
deferred to idle time, since it's not really necessary for the update.)

## Dataflow System

At its heart, a signal is a normal value type with:

1. a change-tracking ID value
2. the possibility of not having a value (possibly dynamically determined)
3. the possibility of not *accepting* new values (again, possibly dynamically
   determined)
4. the possibility of executing custom event code when a value is written

<!-- ### Signal IDs

A signal ID is defined to be a value that changes whenever the value of the
signal changes. Thus, to support this, we need to be able to:

- capture the value of an ID and store it for later
- test if two IDs match (one would typically be stored)
- destruct a stored ID

For simple values like ints and floats, the ID is just the value itself.

And for more complex data, a simple way to create an ID is to keep a 'revision'
counter (i.e., an integer that gets incremented every time the data changes).

For each of these, the three operations above boil down to:

- memcpy (of 4 to 8 bytes)
- memcmp (of 4 to 8 bytes)
- noop -->

### Another Take on Signals

Instead of a widget implementation accepting the signal directly, it could
instead take some sort of "value I/O" structure which is a mutable structure
storing the value and some bit flags. The bit flags would indicate things like:
- signal is readable (input)
- signal value has changed (input)
- signal is writable (input)
- signal was written to (output)

(I suppose that instead of a mutable I/O structure, this could just be a
combination of an input structure and an return value.)

Some of these flags might only be relevant on certain passes (e.g., the inputs
might only be provided on refresh passes).

All the custom logic in detecting changes in the signal value and responding to
changes from the user could be handled in C++ wrapper code that knows the
actual types of the signals.
