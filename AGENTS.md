# Project Status

The project is undergoing a transition from a pure C++ library to one that has
a similar C++ API layered on top of a core C ABI (with a C++ implementation).
Remnants of the C++ library live in `legacy/`. They shouldn't be touched, but
they can be a good model for understanding functionality and C++ API direction.
The intention is that the new C ABI (and its C++ implementation) will focus
more on low-level efficiency, data-oriented design, etc.

The project is not currently concerned with ABI or API stability. The goal is
to break/improve things now while it is still in an intentionally unstable
state.

# Build Environment and Execution

A build directory is already set up in `build/Release`. To build it, use
`scripts/build.bat` from a normal Windows command prompt. It will take care of
setting up the Visual Studio environment and invoking CMake. It takes a target
as its own argument. Right now most development is done around the `alia_app`
demo/sandbox target, e.g.:

`c:\dev\alialib\alia\scripts\build.bat alia_app`

# Comments

Prefer to put comments above the code that they refer to. Use proper
capitalization and punctuation for comments. The one exception to this is when
documenting a variable/field and the comment text is just a noun clause, e.g.:

```
    // screen resolution in pixels
    alia_vec2i res;
```

Or:

```
    // timer event cycle counter - This prevents timer requests from being
    // serviced in the same frame that they're requested (which could throw
    // the event handler into a loop).
    uint64_t timer_event_cycle = 0;
```
