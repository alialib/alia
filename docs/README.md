alia - A Library for Interactive Applications
=============================================

<div class="hide-when-deployed">

[![image](https://flat.badgen.net/travis/tmadden/alia/master?icon=travis)](https://travis-ci.org/tmadden/alia/branches)
[![image](https://flat.badgen.net/appveyor/ci/tmadden/alia/master?icon=appveyor)](https://ci.appveyor.com/project/tmadden/alia/branch/master)
[![image](https://flat.badgen.net/codecov/c/github/tmadden/alia/master?icon=codecov)](https://codecov.io/gh/tmadden/alia/branch/master)
![image](https://flat.badgen.net/badge/C++/14/green)
[![image](https://flat.badgen.net/badge/stability/unstable/yellow)](https://github.com/orangemug/stability-badges#unstable)

</div>

alia (pronounced uh-LEE-uh) is a modern C++ library for developing interactive
applications in a declarative manner. In alia, the UI of your application is
expressed as a composition of *component functions.*

alia component functions:

* **are backed by data** - alia tracks the control flow of your component-level
  code so that it can maintain data corresponding to each of your individual
  component function calls. This mechanic can be used to synchronize widget
  objects, cache computed results, or maintain local state where you need it.

* **are self-contained** - Although from an external perspective, alia component
  functions compose just like normal functions, they are more like classes in
  their capabilities. They can maintain internal state and respond to events
  where needed. This means that the full description of a UI component can be
  localized to one piece of (declarative) code.

* **naturally react to changes in application state** - alia enables you to use
  the familiar mechanics of conditionals, loops, and functional composition to
  model your UI as a function of application state so that *your UI naturally
  reflects the current state of your application.*

* **use dataflow semantics** - alia provides tools for modeling the computations
  in your application as a declarative flow of data. This flow favors the use of
  pure functions and naturally supports caching and 'unready' values (values
  that are waiting for user inputs, background calculations, remote queries,
  etc.).

alia is agnostic to the particular UI library you use. It provides the
*mechanics* for modeling an interactive application declaratively and is
designed to hook up to other libraries so that your application can drive those
libraries declaratively. (And it should be capable of driving any interactive
system: a game, a physics simulation, etc.)

**STABILITY/MATURITY WARNING:** This is the first public release of alia, so
it's likely unstable and definitely incomplete. It's going to require some
work/patience on your part to use it. See 'Project Status' below for details.

<div class="hide-when-deployed">

Check out [the documentation](https://alia.dev) for more info.

</div>

An Example
----------

Below is a simple tip calculator made using alia and an experimental
[asm-dom](https://github.com/mbasso/asm-dom) wrapper. You can see it in action
<a target="_self"
href="https://alia.dev/#/assorted-examples?id=tip-calculator">
here</a>, along with some other examples.

```cpp
void
tip_calculator(dom::context ctx)
{
    // Get some component-local state for the bill amount.
    auto bill = alia::get_state(ctx, empty<double>());
    dom::text(ctx, "How much is the bill?");
    // Display an input that allows the user to manipulate our bill state.
    dom::input(ctx, bill);

    // Get some more component-local state for the tip rate.
    auto tip_rate = alia::get_state(ctx, empty<double>());
    dom::text(ctx, "What percentage do you want to tip?");
    // Users like percentages, but we want to keep the 'tip_rate' state as a
    // rate internally, so this input presents a scaled view of it for the user.
    dom::input(ctx, scale(tip_rate, 100));
    // Add a few buttons that set the tip rate to common values.
    dom::button(ctx, "18%", tip_rate <<= 0.18);
    dom::button(ctx, "20%", tip_rate <<= 0.20);
    dom::button(ctx, "25%", tip_rate <<= 0.25);

    // Calculate the results and display them for the user.
    // Note that these operations have dataflow semantics, and since `bill` and
    // `tip_rate` both start out empty, nothing will actually be calculated
    // (or displayed) until the user supplies values for them.
    auto tip = bill * tip_rate;
    auto total = bill + tip;
    dom::text(ctx,
        alia::printf(ctx,
          "You should tip %.2f, for a total of %.2f.", tip, total));

    // Conditionally display a message suggesting cash for small amounts.
    alia_if (total < 10)
    {
        dom::text(ctx,
            "You should consider using cash for small amounts like this.");
    }
    alia_end
}
```

Project Status
--------------

alia as a concept is actually fairly mature and has been used successfully in a
few major internal desktop applications. This open-source release is an early
version in the latest generation of alia. This generation brings about two major
changes:

1. The core mechanics have far better documentation and testing, plus some minor
   improvements to interfaces and terminology.

2. Everything but the core mechanics has been stripped out. - Earlier
   generations of alia were immediate mode GUI libraries (like [Dear
   ImGui](https://github.com/ocornut/imgui)), but over the years, it became
   clear that the actual GUI code was no longer a novel feature and that to do
   it properly essentially required writing a traditional, "retained-mode" GUI
   library with a declarative wrapper around it, so this version of alia focuses
   solely on the declarative wrapper part.

This generation is still new and hasn't been used yet in any major projects.
While the mechanics should be fairly robust, I'm still in the process of
experimenting with hooking it up to other libraries and developing realistic
integrations for my own purposes. At the moment, I provide experimental/example
integrations with asm-dom and Qt, but I have no immediate plans of providing
"official" integrations for any external libraries.

**So at this point, if you're interested in using alia in anything resembling a
real project, it will require some work on your part to hook it up to the
libraries you need for user interfaces, rendering, etc.** *This release is
intended for people who'd like to play around with it, provide feedback, and
perhaps try integrating it with their favorite UI/game/physics library.*

If you're interested in sharing your integrations, I'm more than happy to
incorporate them as examples and/or link people to your projects.

Getting Started
---------------

Continue on to <a target="_self"
href="https://alia.dev/#/interactive-hello">the full
documentation</a> for more info.
