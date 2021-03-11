Assorted Examples
=================

<script>
    init_alia_demos(['for-each-map-demo', 'time-signal',
        'number-smoothing', 'color-smoothing', 'factor-tree']);
</script>

If you're more interested in code than prose, you'll like this page the best.
Many of these come from other sections of the documentation.

You may also want to check out the [alia/HTML demos](https://html.alia.dev/).

Containers
----------

Here's an (admittedly contrived) example of working with containers in alia.
It uses a `std::map` to map player names to their scores.

Like the tip calculator on the landing page, it also shows off some of the more
basic features of alia, like [actions](actions.md) and how you can use alia's
data graph to 'magically' manifest state when and where you need it, even in
the middle of declarative component code.

[source](tracking.cpp ':include :fragment=for-each-map-demo')

<div class="demo-panel">
<div id="for-each-map-demo"></div>
</div>

Factor Trees
------------

This example displays factorization trees for numbers that you enter. (It
implements a poor man's tree view by allowing the user to show and hide the
subtree associated with each composite factor.)

What's interesting about this is that there's actually no application data that
mirrors this tree view. The application 'model' consists entirely of a single
integer. The structure of the UI (and the state that allows the user to expand
and collapse nodes in the tree) is fully defined by the recursive structure of
the calls to `factor_tree`.

[source](numerical.cpp ':include :fragment=factor-tree')

<div class="demo-panel">
<div id="factor-tree"></div>
</div>

Timing Signals
--------------

Although all signals in alia are conceptually time-varying values, most of them
only care about the present (e.g., `a + b` is just whatever `a` is right now
plus whatever `b` is). However, some signals are more closely linked to time
and explicitly vary with it:

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
smoothed view of a color with a custom transition:

[source](timing.cpp ':include :fragment=color-smoothing')

<div class="demo-panel">
<div id="color-smoothing"></div>
</div>
