alia
====

.. image:: https://img.shields.io/travis/tmadden/alia/master.svg?style=flat&logo=travis-ci&logoColor=white
    :target: https://travis-ci.org/tmadden/alia/branches

.. image:: https://img.shields.io/appveyor/ci/tmadden/alia/master.svg?style=flat&logo=appveyor&logoColor=white
    :target: https://ci.appveyor.com/project/tmadden/alia/branch/master

.. image:: https://img.shields.io/codecov/c/github/tmadden/alia/master.svg?style=flat&logo=codecov&logoColor=white
    :target: https://codecov.io/gh/tmadden/alia/branch/master

|

**WARNING**: This project is in an unstable, pre-release state. There are
missing links, missing documentation pages, missing features, and APIs that may
change in the future. That said, what's there now should work well, so if you're
interested in playing around with it, I welcome any feedback or contributions!

alia (pronounced uh-LEE-uh) is a modern C++ library for developing reactive
applications. It provides a core of generic facilities for reactive programming
(data flow modeling, event processing, etc.) as well as interfaces to some
existing C++ libraries, allowing those libraries to be used reactively from
within alia applications.

The full documentation for alia can be found here.

Below is a simple tip calculator written in alia using the asm-dom wrapper. To
try it out and see more examples, click here.

.. todo: Add links to documentation and examples.

.. code-block:: C++

	// Get the state we need.
	auto bill = get_state<double>(ctx); // defaults to uninitialized
	auto tip_percentage = get_state(ctx, value(20.)); // defaults to 20%

	// Show some controls for manipulating our state.
	do_input(ctx, bill);
	do_input(ctx, tip_percentage);

	// Do some reactive calculations.
	auto tip = bill * tip_percentage / value(100.);
	auto total = bill + tip;

	// Show the results.
	do_text(ctx, printf(ctx, "tip: %.2f", tip);
	do_text(ctx, printf(ctx, "total: %.2f", total);

	// Allow the user to split the bill.
	auto n_people = get_state(ctx, value(1.));
	do_input(ctx, n_people);
	alia_if (n_people > 1)
	{
		do_text(ctx, format(ctx, "tip per person: %.2f", tip / n_people);
		do_text(ctx, format(ctx, "total per person: %.2f", total / n_people);
	}
	alia_end
