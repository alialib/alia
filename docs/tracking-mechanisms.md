Tracking Mechanisms
===================

<script>
    init_alia_demos(['numerical-analysis', 'switch-example', 'transform-demo',
        'metered-transform-demo', 'metered-direct-counting']);
</script>

This page documents all the various mechanisms you can use to allow alia to
observe the flow of your application and properly maintain the data graph...

Conditionals
------------

### alia_if/else

alia provides the equivalent of `if`, `else if` and `else` as macros. Here again
is a simple example that makes use of all three:

?> The alia control flow macros come in both uppercase and lowercase forms.
   Traditionally, they have been lowercase to more closely resemble the actual
   C++ keywords that they mimic. However, the uppercase form makes it more
   obvious to readers (and clang-format) that the macros are indeed macros.
   Ultimately, it's up to you which style you prefer. If you want to be strict
   in your project, you can disable the lowercase form in the configuration.

[source](numerical.cpp ':include :fragment=analysis')

<div class="demo-panel">
<div id="numerical-analysis"></div>
</div>

As with all alia control flow macros, `alia_if` blocks must be terminated with
`alia_end`.

The conditions that you provide to `alia_if` (and `alia_else_if`) statements can
be signals or raw C++ values. If a signal is provided and the signal has no
value, alia considers the condition *neither true nor false.* As such, the code
dependent on that statement isn't executed, but any subsequent `else` blocks are
*also not considered.* A condition without a value essentially terminates the
entire sequence of `alia_if/else` statements. As you can see in the above
example, before `n` is given a value, none of the `do_text` calls are executed.

### alia_switch

alia provides a similar set of macros that mirror the behavior of `switch`,
`case`, and `default`. Note that all the normal semantics of `switch` statements
work as expected, including fallthrough:

[source](tracking.cpp ':include :fragment=switch-example')

<div class="demo-panel">
<div id="switch-example"></div>
</div>

The value passed to `alia_switch` is a signal, and just like `alia_if`
conditionals, if that signal doesn't have a value, none of the cases will match
(including the default, if any).

Looping Constructs
------------------

### for_each

### transform

When working with container signals, it's common to apply some function
individually to each item in the container. Just like `std::transform` allows
you to do this on regular C++ containers, `alia::transform` allows you to do
this on container signals:

<dl>

<dt>transform(ctx, container, f)</dt><dd>

Coming soon...

</dd>

</dl>

[source](application.cpp ':include :fragment=transform-demo')

<div class="demo-panel">
<div id="transform-demo"></div>
</div>

Again, we also could've just counted the number of primes directly with a single
call to `apply`, like this:

[source](application.cpp ':include :fragment=direct-counting')

However, there's one very important difference between the two implementations:
In the `transform` version, the call to `apply(ctx, is_prime, n)` is
*individually caching* the result of `is_prime` for each input number, so as we
update them, `is_prime` is only reinvoked on the individual numbers that have
changed. This is in contrast to the 'single apply' version where any change to
any part of the vector causes the entire `std::count_if` calculator to be
redone.

Below are the two versions with counters to show how many times each has called
`is_prime`.

First the `transform` version:

<div class="demo-panel">
<div id="metered-transform-demo"></div>
</div>

And now the 'single apply' version:

<div class="demo-panel">
<div id="metered-direct-counting"></div>
</div>

Of course, given that this is a contrived example and `is_prime` is a fairly
cheap function to call, it doesn't make much difference which way we do it, but
in real world applications where you might be retrieving data from remote
sources or doing computationally-intensive background calculations, the
difference can be significant.

### alia_for

### alia_while

Custom Flow
-----------

### named_block

### scoped_block

### alia_untracked_if
