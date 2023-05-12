Roadmap
=======

This is the current roadmap for alia, very roughly in order of priority. If
you're interested in helping out, providing feedback, or you'd like guidance on
extending alia in a different direction, feel free to [get in
touch](https://github.com/alialib/alia/discussions)!

Developer Experience Improvements
---------------------------------

**Escape Emscripten** - Emscripten is awesome for deploying C++ to the web, but
it's pretty terrible for debugging. alia/HTML should provide a separate
development mode where your C++ code runs natively (from an IDE) and
communicates with a special web server (and/or webview) for manipulating the
DOM and executing JS. This could largely be transparent to the application
code, but a lot of work is needed at lower levels to make this happen.

**Debugging Tools** - Perhaps building off the above, provide alia-specific
debugging tools:
* Visualize the component hierarchy
* Inspect individual signal values
* Break inside component instance
* Break when a signal value changes
* Visualize signal values over time
* View event logs
* Externalize state for hot reloading or time travel
* Natvis for signals
* etc.

Expanded Web API
----------------

Flesh out alia's bindings for the HTML API: more Bootstrap components, canvas
rendering, fullscreen mode, better input event detection, etc. Also add
C++-oriented APIs for popular JS libraries or REST APIs.

Documentation Improvements
--------------------------

Provide better guidance on application design/structure principles, integrating
alia with external libraries, performance considerations, etc.

Start using an automated system like Doxygen to provide reference documentation
and give the documentation as a whole a better separation into guides and
reference.

Provide a tutorial that walks through the creation of a simple app.

Performance Benchmarks
----------------------

Add proper benchmarks for the overhead that alia introduces into an app, both
at runtime and compile time. Start working on specific improvements to reduce
those overheads.

alia on the Desktop
-------------------

Building off of the "Escape Emscripten" goal above, allow alia/HTML apps to
actually be deployed as desktop apps (inside a webview). The sweet spot for
this would be intensely graphical applications (e.g., games and scientific
visualizations) where C++ is needed for rendering and you'd like a sane,
professional-looking C++ UI solution for adding overlays and side panels (and
you don't mind wasting another 200MB of RAM running a browser inside your app).
The same could apply for non-graphical apps where C++ is the meat of the code
for performance or hardware reasons and you want to create a modern-looking UI
without first exposing everything to JavaScript.

Another avenue for getting alia onto the desktop would be to simply hook it up
to one of the many C++ UI frameworks out there. I would definitely support such
efforts, but I'm not personally prioritizing it.

The long-term, pie-in-the-sky vision for this would be to eventually define a
common set of UI capabilities that are available through both the web and some
reasonable C++ desktop library (Qt?) and allow alia apps to compile natively to
either environment.

Expanded Dataflow Capabilities
------------------------------

`alia::apply` and `alia::async` do a reasonable job of ensuring performance in
applications with modest computation needs while still allowing you to express
your dataflow declaratively. However, when computations get more expensive,
it's important to share results across the application and potentially across
time. alia should facilitate this too (potentially through interfaces to
external libraries).
