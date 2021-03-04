alia - A Library for Interactive Applications
=============================================

alia (pronounced uh-LEE-uh) is a modern C++ library for declaratively
developing user interfaces. It currently targets the web. In alia, the UI of
your application is expressed as a composition of *component functions.*

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

Some Code
---------

Below is a simple tip calculator made using alia. You can see it in action <a
target="_self" href="https://alia.dev/#/assorted-examples?id=tip-calculator">
here</a>, along with some other examples.

```cpp
void
tip_calculator(html::context ctx)
{
    // Get some component-local state for the bill amount.
    auto bill = alia::get_state(ctx, empty<double>());
    html::p(ctx, "How much is the bill?");
    // Display an input that allows the user to manipulate our bill state.
    html::input(ctx, bill);

    // Get some more component-local state for the tip rate.
    auto tip_rate = alia::get_state(ctx, empty<double>());
    html::p(ctx, "What percentage do you want to tip?");
    // Users like percentages, but we want to keep the 'tip_rate' state as a
    // rate internally, so this input presents a scaled view of it for the user.
    html::input(ctx, scale(tip_rate, 100));
    // Add a few buttons that set the tip rate to common values.
    html::button(ctx, "18%", tip_rate <<= 0.18);
    html::button(ctx, "20%", tip_rate <<= 0.20);
    html::button(ctx, "25%", tip_rate <<= 0.25);

    // Calculate the results and display them for the user.
    // Note that these operations have dataflow semantics, and since `bill` and
    // `tip_rate` both start out empty, nothing will actually be calculated
    // (or displayed) until the user supplies values for them.
    auto tip = bill * tip_rate;
    auto total = bill + tip;
    html::p(ctx,
        alia::printf(ctx,
          "You should tip %.2f, for a total of %.2f.", tip, total));

    // Conditionally display a message suggesting cash for small amounts.
    alia_if (total < 10)
    {
        html::p(ctx,
            "You should consider using cash for small amounts like this.");
    }
    alia_end
}
```

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
web apps, but it definitely shouldn't be considered mature.

alia is part of a long-term vision to enable declarative development of
computationally intensive user interfaces in C++. If you're interested in where
it's going, check out [the roadmap](roadmap.md), and if you're interested in
helping steer it, please don't hesitate to get in touch. I appreciate any
feedback or interest you might have.
