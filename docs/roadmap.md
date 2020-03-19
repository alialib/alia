Roadmap
=======

Features
--------

### State Externalization

alia's data graph provides the ability to 'magically' get state where you need
it in a UI, but currently, that state vanishes when the UI is closed. It would
be nice to be able to have that state pulled from a dynamic data structure that
could be mapped to JSON or YAML for persistence. (Just like application code
dynamically determines the structure of alia's data graph, it could dynamically
determine the structure of this state. - A first attempt at this would likely
ignore issues related to versioning the state, so chunks of it might have to be
thrown away when the corresponding code changes, but that's fine if the state is
used for things like tree node open/close flags, selected items, temporary
dialog parameters, etc.)

### Debugging

alia applications can be difficult to debug: signal values can be difficult to
access, and alia's control flow principles can make it hard to set breakpoints
at the right place. Custom tools could help this situation. alia itself might
have to provide some instrumentation that could connect to an external
monitor or IDE plugin.

### Error Handling

Some parts of alia casually ignore or swallow errors. This actually hasn't been
a problem in practice for in-house applications, but for more general usage,
alia should provide the tools for dealing with errors that occur within
component functions. (Something like React's [Error
Boundaries](https://reactjs.org/docs/error-boundaries.html) would be a good
start.)

### Timer Events/Utilities

alia currently has an animation timer, but earlier versions of alia also
supported timer events. There should be a generic interface for requesting these
that can be implemented by back ends. Timer events allow for more efficient
implementations of timing utilities like second counters, signal deflickering,
etc.

### Validation

Although it can be treated as an entirely separate concern, validation often
overlaps with some of the core alia concerns. (Why isn't this action ready to be
performed? Why did this signal reject the value entered by the user?) alia
provides very cursory support for validation at the moment. It would be nice to
provide a more fully featured system that could be used to track validation
errors, define validation scopes, provide feedback to UI elements, mask signals
and actions when corresponding contexts/elements aren't valid, etc.

### Declarative Calculation Management

In recent years, for in-house development, alia has always been paired with a
sister library that provides declarative calculation management and resource
retrieval. In a nutshell, the idea is that just like your declarative UI code
analyzes application state and declares the UI elements/logic that should exist
for that state, your application can also analyze application data and declare
what calculation results are relevant for that data. A 'declaration of a
relevant calculation result' takes the form of a calculation tree with resources
at the leaves. An example might be an image filtering application that declares
'I need the image at this URL, with a red eye filter applied, and then a sepia
filter applied.' Maybe that image has already been retrieved from that URL but
hasn't had the filters applied, or maybe the red eye filter has been applied but
not the sepia one, or maybe the image is actually already available in memory
exactly the way the application wants it because it's been requesting the exact
same thing on every update for the last five minutes. Just like an application
with a declarative UI doesn't care how you get the UI to match what it declares,
the imaging application doesn't care how this image arrives. It just wants it
displayed for the user.

An alia application that deals with non-trivial calculations and resources can
benefit a lot from capabilities like this, but alia currently falls short of
providing them itself. It would be nice to either provide them in some form or
integrate with a library that does.

Integrations
------------

Obviously, it would be nice to have stable, production-ready integrations with
popular C++ libraries for creating user interfaces and rendering. These probably
don't belong in the core of alia, but they are essential to it being useful.

Similarly, it would be useful to provide more utilities for interacting with
common resources as signals and actions.

Additionally, there are certain like-minded, complementary libraries that it
might be useful for alia to integrate with more at the core level, like
[lager](https://sinusoid.es/lager/) and [immer](https://sinusoid.es/immer/).

Documentation
-------------

Provide better guidance on application design/structure principles, integrating
alia with external libraries, performance considerations, etc.

Provide some tutorials and examples of realistic applications.
