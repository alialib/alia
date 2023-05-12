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

</dl>
