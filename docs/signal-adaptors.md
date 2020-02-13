Adaptors
========

<script>
    init_alia_demos(['numeric-adaptors']);
</script>

Casts
-----

<dl>

<dt>signal_cast&lt;Value&gt;(x)</dt><dd>

Yields a signal with the value type `Value` that acts as a proxy for `x`. The
proxy applies `static_cast` to convert its own values to and from the value type
of `x`.

</dd>

</dl>

Fallbacks
---------

Sometimes it's useful to provide a fallback value for a signal that isn't always
available when we'd like it to be. This can be done with `add_fallback`:

<dl>

<dt>add_fallback(primary, fallback)</dt><dd>

`primary` and `fallback` must be signals with the same value type.

`add_fallback(primary, fallback)` yields another signal whose value is that of
`primary` if `primary` is readable and that of `fallback` otherwise.

All writes go directly to `primary`.

</dd>

</dl>

Numeric
-------

<dl>

<dt>scale(n, factor)</dt><dd>

Yields a signal that presents a view of `n` that is scaled by `factor`.

</dd>

<dt>offset(n, factor)</dt><dd>

Yields a signal that presents a view of `n` that is offset by `factor`.

</dd>

</dl>

Both of these will present bidirectional views if the underlying is
bidirectional, as demonstrated here:

[source](adaptors.cpp ':include :fragment=numeric-adaptors')

<div class="demo-panel">
<div id="numeric-adaptors"></div>
</div>
