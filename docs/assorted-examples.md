Assorted Examples
=================

<script>
    init_alia_demos(['tip-calculator-demo', 'loop-macros-demo',
        'for-each-map-demo', 'fetch-country', 'time-signal',
        'value-smoothing']);
</script>

If you're more interested in code than prose, you'll like this page the best.
Many of these come from other sections of the documentation...

Tip Calculator
--------------

Here's a quick-and-dirty tip calculator that shows off some
[actions](actions.md), as well as how you can use alia's data graph to
'magically' manifest state when and where you need it, even in the middle of
reactive application code. Currently, this state is entirely isolated to the bit
of code that calls `get_state`, but it's on the roadmap to add proper
externalization capabilities...

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

[source](timing.cpp ':include :fragment=value-smoothing')

<div class="demo-panel">
<div id="value-smoothing"></div>
</div>
