alia - A Library for Interactive Applications
=============================================

<script>
    init_alia_demos(['tip-calculator-demo']);
</script>

alia (pronounced uh-LEE-uh) is a modern C++ library for declaratively
developing user interfaces. It currently targets HTML5 and client-side web
apps. In alia, the UI of your application is expressed as a composition of
*component functions.*

alia component functions:

* **are backed by data** - alia tracks the control flow of your component-level
  code so that it can maintain data corresponding to each of your individual
  component function calls. This mechanic can be used to synchronize widget
  objects, cache computed results, or maintain local state where you need it.

* **are self-contained** - Although from an external perspective, alia
  component functions compose just like normal functions, they are more like
  classes in their capabilities. They can maintain internal state and respond
  to events where needed. This means that the full description of a UI
  component can be localized to one piece of (declarative) code.

* **naturally react to changes in application state** - alia enables you to use
  the familiar mechanics of conditionals, loops, and functional composition to
  model your UI as a function of application state so that *your UI naturally
  reflects the current state of your application.*

* **use dataflow semantics** - alia provides tools for modeling the
  computations in your application as a declarative flow of data. This flow
  favors the use of pure functions and naturally supports caching and 'unready'
  values (values that are waiting for user inputs, background calculations,
  remote queries, etc.).

Although alia is currently focused on web development, its core mechanics are
generic, and the intention is to eventually extend it to other environments.

Live Example
------------

Below is a simple tip calculator coded in alia and running live in your
browser. Unlike many other C++-in-the-browser demos, this uses regular HTML
elements and doesn't require you to download an entire desktop windowing system
to run inside your browser. (This is all possible thanks to
[Emscripten](https://emscripten.org/) and
[asm-dom](https://github.com/mbasso/asm-dom).) You can see some other assorted
examples [here](assorted-examples.md) and [here](https://html.alia.dev).

<div class="demo-panel">
<div id="tip-calculator-demo"></div>
</div>

[source](numerical.cpp ':include :fragment=tip-calculator')

Project Status
--------------

alia is somewhere between experimental and production-ready. It has a long
history as an IMGUI library used internally in desktop applications (in the
domain of medical imaging and scientific computing). Experience with alia has
shown that writing a professional-quality IMGUI library requires first writing
a professional-quality retained-mode GUI library and then wrapping a
declarative interface around it, and since there are plenty of existing
retained-mode GUI libraries out there, alia now focuses on the "wrapping a
declarative interface around it" part.

This open-source release represents a major refresh of the core mechanics of
alia (dataflow, component identity management, etc.) with far better
documentation and testing and a focus on being able to integrate with
object-oriented GUI libraries and drive their object hierarchies declaratively.

The only integration that currently exists is
[alia/HTML](html-introduction.md), which connects alia to HTML5 and allows you
to write client-side web apps in alia. I'm currently using this for in-house
web apps and adding additional capabilities as I need them, so it definitely
shouldn't be considered mature, but it's headed in that direction.

alia is part of a long-term vision to enable declarative development of
computationally intensive user interfaces in C++. If you're interested in where
it's going, check out [the roadmap](roadmap.md), and if you're interested in
helping steer it, please don't hesitate to [get in
touch](https://github.com/alialib/alia/discussions). I appreciate any feedback
or interest you might have.

Onward
------

[Read on](interactive-hello.md) for an introduction to some of the concepts and
techniques used in alia to create declarative app UIs in C++. Or, if you want
to jump right in, try out [the starter
template](https://github.com/alialib/alia-html-starter).
