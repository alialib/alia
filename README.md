alia
====

[![image](https://img.shields.io/travis/tmadden/alia/master.svg?style=flat&logo=travis-ci&logoColor=white)](https://travis-ci.org/tmadden/alia/branches)
[![image](https://img.shields.io/appveyor/ci/tmadden/alia/master.svg?style=flat&logo=appveyor&logoColor=white)](https://ci.appveyor.com/project/tmadden/alia/branch/master)
[![image](https://img.shields.io/codecov/c/github/tmadden/alia/master.svg?style=flat&logo=codecov&logoColor=white)](https://codecov.io/gh/tmadden/alia/branch/master)
[![image](https://img.shields.io/badge/stability-unstable-yellow.svg?style=flat)](https://github.com/orangemug/stability-badges#unstable)
![image](https://img.shields.io/badge/C++-14-green.svg?style=flat&logo=c%2B%2B)

**WARNING**: This project is in an unstable, pre-release state. There
are missing links, missing documentation pages, missing features, and
APIs that may change in the future. That said, what's there now should
work well, so if you're interested in playing around with it, I welcome
any feedback or contributions!

alia (pronounced uh-LEE-uh) is a modern C++ library for developing reactive
applications. It provides a core of generic facilities for reactive programming
and is designed to hook up to other, more traditional libraries and allow them
to be used in a reactive manner. In particular, alia features:

* **data-backed control flow** - alia provides mechanisms for tracking your
  control flow so that you can stably associate arbitrary data objects with
  specific points in your application's reactive presentation logic.

  In alia, rather than explicitly managing trees of presentation objects, you
  compose your application as a tree of function calls, each of which is allowed
  to manage its own object(s) internally.

* **dataflow semantics** - alia provides tools for modeling the computations in
  your application as a dataflow that implicitly supports the reality that
  function inputs are often not immediately available (because they are waiting
  for user inputs, background calculations, remote queries, etc.).

alia also provides a whole suite of supporting features that help keep these
concepts practical in real-world applications: efficient change detection for
program state, efficient routing of events, declarative event handlers, etc.

Check out [the documentation](https://tmadden.github.io/alia) for more info.

An Example
----------

Below is a simple tip calculator written in alia using an experimental
[asm-dom](https://github.com/mbasso/asm-dom) wrapper. To try it out and see more
examples, click here.

```cpp
// Get the state we need.
auto bill = get_state(ctx, empty<double>()); // defaults to uninitialized
auto tip_ratio = get_state(ctx, 0.2); // defaults to 20%

// Show some controls for manipulating our state.
do_number_input(ctx, bill);
do_number_input(ctx, scale(tip_ratio, 100)); // Users like %, not ratios.

// Do some reactive calculations.
auto tip = bill * tip_ratio / 100;
auto total = bill + tip;

// Show the results.
do_text(ctx, printf(ctx, "tip: %.2f", tip));
do_text(ctx, printf(ctx, "total: %.2f", total));

// Allow the user to split the bill.
auto n_people = get_state(ctx, 1);
do_number_input(ctx, n_people);
alia_if (n_people > 1)
{
    do_text(ctx, printf(ctx, "tip per person: %.2f", tip / n_people));
    do_text(ctx, printf(ctx, "total per person: %.2f", total / n_people));
}
alia_end
```
