Configuration
=============

alia supports the following configuration options. Just `#define` the ones you
want before including `alia.hpp`:

<dl>

<dt>ALIA_STRICT_MACROS</dt><dd>

This instructs alia to only provide uppercase macros.

By default, alia provides many of its macros in both lowercase and uppercase
form. For example, `alia_if` and `ALIA_IF` are equivalent. The lowercase forms
more closely resemble the keywords they emulate, but the uppercase forms are
more obviously macros. Which you prefer is up to you, but if you want to ensure
that your project strictly follows traditional uppercase C++ macro conventions,
use this option.

</dd>

<dt>ALIA_DYNAMIC_CONTEXT_CHECKS</dt><dd>

This switches to using dynamic context checking. (This is somewhat experimental,
but more so in terms of whether or not it's useful rather than whether or not it
works correctly.)

Normally, when you try to call a function that requires a tag/object that you're
not providing in your context, you'll get a compile-time error. This changes
that to a run-time error. This eliminates a lot of template shenanigans inside
alia (at the expense of a few more run-time checks), so it might improve your
compilation times during development. It's not recommended for production.

</dd>

</dl>
