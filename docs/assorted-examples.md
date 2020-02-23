Assorted Examples
=================

<script>
    init_alia_demos(['tip-calculator-demo', 'loop-macros-demo']);
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
