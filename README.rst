alia
====

.. image:: https://travis-ci.org/tmadden/alia.svg?branch=master
    :target: https://travis-ci.org/tmadden/alia

.. image:: https://ci.appveyor.com/api/projects/status/nxtmxag4oph0a0ie/branch/master?svg=true
    :target: https://ci.appveyor.com/project/tmadden/alia/branch/master

.. image:: https://codecov.io/gh/tmadden/alia/branch/master/graph/badge.svg
    :target: https://codecov.io/gh/tmadden/alia

|

alia (pronounced uh-LEE-uh) is a modern C++ library for developing reactive applications. It provides a core of generic facilities for reactive programming (data flow modeling, event processing, etc.) as well as interfaces to some existing C++ libraries, allowing those libraries to be used reactively from within alia applications.

The full documentation for alia can be found here.

Below is a simple tip calculator written in alia using the asm-dom wrapper. To try it out and see more examples, click here.

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
	do_text(ctx, format(ctx, "tip: %.2f", tip);
	do_text(ctx, format(ctx, "total: %.2f", total);

	// Allow the user to split the bill.
	auto n_people = get_state(ctx, value(1.));
	do_input(ctx, n_people);
	alia_if (n_people > 1)
	{
		do_text(ctx, format(ctx, "tip per person: %.2f", tip / n_people);
		do_text(ctx, format(ctx, "total per person: %.2f", total / n_people);
	}
	alia_end
