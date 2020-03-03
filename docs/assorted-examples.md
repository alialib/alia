Assorted Examples
=================

<script>
    init_alia_demos(['tip-calculator-demo', 'loop-macros-demo',
        'for-each-map-demo', 'fetch-country', 'time-signal',
        'number-smoothing', 'color-smoothing', 'factor-tree']);
</script>

If you're more interested in code than prose, you'll like this page the best.
Many of these come from other sections of the documentation...

Tip Calculator
--------------

Here's a simple tip calculator that shows off some [actions](actions.md), as
well as how you can use alia's data graph to 'magically' manifest state when and
where you need it, even in the middle of reactive application code.

[source](numerical.cpp ':include :fragment=tip-calculator')

<div class="demo-panel">
<div id="tip-calculator-demo"></div>
</div>

Containers
----------

Here's an (admittedly contrived) example of working with containers in alia.
It uses a `std::map` to map player names to their scores.

[source](tracking.cpp ':include :fragment=for-each-map-demo')

<div class="demo-panel">
<div id="for-each-map-demo"></div>
</div>

Minimal Integration
-------------------

Although alia provides a whole suite of tools for modeling your application's
presentation logic reactively, it tries not to *force* them on you wholesale.
Here's an example of how you might use a smaller subset of alia's capabilities
to integrate alia with your application data.

[source](tracking.cpp ':include :fragment=loop-macros-demo')

<div class="demo-panel">
<div id="loop-macros-demo"></div>
</div>

Factor Trees
------------

This example displays factorization trees for numbers that you enter. It
emulates a tree view by allowing the user to show and hide the subtree
associated with each composite factor.

What's interesting about this is that there's actually no application data that
mirrors this tree view. The application 'model' consists entirely of a single
integer. The structure of the UI (and the state that allows the user to expand
and collapse nodes in the tree) is fully defined by the recursive structure of
the calls to `do_factor_tree`.

[source](numerical.cpp ':include :fragment=factor-tree')

<div class="demo-panel">
<div id="factor-tree"></div>
</div>

Asynchronous I/O
----------------

This shows an example of using `alia::async()` to integrate an asynchronous API.
In this case, we're using Emscripten's fetch API to remotely look up country
names on [REST Countries](https://restcountries.eu).

[source](fetch.cpp ':include :fragment=fetch-country')

<div class="demo-panel">
<div id="fetch-country"></div>
</div>

Timing Signals
--------------

Although all signals in alia are conceptually time-varying values, most of them
only care about the present (e.g., `a + b` is just whatever `a` is right now
plus whatever `b` is). However, some signals are more closely linked to time and
explicitly vary with it:

[source](timing.cpp ':include :fragment=time-signal')

<div class="demo-panel">
<div id="time-signal"></div>
</div>

You can use this explicit notion of time to do fun things like smooth out other
signals:

[source](timing.cpp ':include :fragment=number-smoothing')

<div class="demo-panel">
<div id="number-smoothing"></div>
</div>

You can smooth anything that provides the basic arithmetic operators. Here's a
smoothed view of a color:

[source](timing.cpp ':include :fragment=color-smoothing')

<div class="demo-panel">
<div id="color-smoothing"></div>
</div>
