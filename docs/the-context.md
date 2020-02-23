The Context
===========

<script>
    init_alia_demos(['custom-components', 'multiple-custom-components']);
</script>

Here's an example of how you might use a custom context to establish a username
as a globally accessible property in your app.

[source](context.cpp ':include :fragment=custom-components')

<div class="demo-panel">
<div id="custom-components"></div>
</div>

The call to `copy_context` ensures that your new context has its own underlying
storage for its components. This isn't always strictly necessary (and it's
unnecessary in this example), but it a safe practice. Without it, you could run
into problems where you reuse the same tag with different values in different
parts of your app. (This might happen if you add additional components at lower
levels of your app.)

You can also, of course, add multiple components at once...

[source](context.cpp ':include :fragment=multiple-custom-components')

<div class="demo-panel">
<div id="multiple-custom-components"></div>
</div>
