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

<dt>fake_writability(s)</dt><dd>

This essentially casts a readable signal to a bidirectional one.

`fake_writability(s)`, where `s` is a signal, yields a wrapper for `s` that
pretends to have write capabilities. It will never actually be ready to write,
but it will type-check as a writable signal.

</dd>

</dl>

Availability
------------

The following allow you to assess the availability of a signal (in signal space,
of course):

<dl>

<dt>has_value(x)</dt><dd>

`has_value(x)` yields a signal to a boolean which indicates whether or not `x`
has a value. (The returned signal itself always has a value.)

</dd>

<dt>ready_to_write(x)</dt><dd>

`ready_to_write(x)` yields a signal to a boolean that indicates whether or not
`x` is ready to write.

</dd>

Sometimes it's useful to provide a fallback value for a signal that isn't always
available when we'd like it to be:

<dt>add_fallback(primary, fallback)</dt><dd>

`primary` and `fallback` must be signals with the same value type.

`add_fallback(primary, fallback)` yields another signal whose value is that of
`primary` if `primary` has a value and that of `fallback` otherwise.

All writes go directly to `primary`.

</dd>

On the other hand, sometimes you might want to mask a signal that's available
when you don't want it to be:

<dt>mask(signal, availability_flag)</dt><dd>

`availability_flag` must be a signal (or raw value) that can be evaluated in a
boolean (conditional) context.

`mask(signal, availability_flag)` does the equivalent of bit masking on
individual signals. If `availability_flag` evaluates to `true`, the mask
evaluates to `signal`. Otherwise, it evaluates to an empty signal of the same
type.

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
