Function Application
====================

<script>
    init_alia_demos(['simple-apply']);
</script>

Of course, to write real applications, we need to be able to define more
interesting data flows than operators alone will allow, so alia provides ways to
incorporate general C++ functions into your data flow.

In the following examples, we'll be working with this simple prime testing
function:

[source](application.cpp ':include :fragment=is-prime')

Notice that it's a *pure* function. Its output depends only on its input, and it
doesn't have any side effects. Other than that, it's not a very interesting
function, which is really the point: You can use normal C++ variables, normal
C++ control flow, etc. You can write and test this function like you would any
other C++ function and then insert it into your UI as part of a reactive data
flow...

apply()
-------

Now let's write a UI that allows the user to make use of our function:

[source](application.cpp ':include :fragment=simple-apply')

<div class="demo-panel">
<div id="simple-apply"></div>
</div>

`apply(ctx, is_prime, n)` is the alia equivalent of calling `is_prime(n)`, but
it has two important differences from normal C++ function application:

1. As with anything signal-related, `apply` accounts for the possibility that
   one or more of its arguments *don't have a value* and makes sure that its
   output also doesn't carry a value in those cases. &mdash; Note that
   `is_prime` itself is spared from having to worry about this possibility, as
   it should be.

2. Internally, `apply` caches the result of calling `is_prime` on `n`. As the UI
   updates, as long as the value of `n` stays the same for this particular call
   to `is_prime`, `apply` will continue using the cached result and won't bother
   to reinvoke `is_prime`. &mdash; This is the reason for the `ctx` parameter.
   It allows `apply` to take advantage of alia's data graph to maintain its own
   internal (single-entry) cache.

`apply` is variadic and can forward along any number of arguments to the
function:

<dl>

<dt>apply(ctx, f, args...)</dt><dd>

`apply(ctx, f, args...)`, where `args` are all signals, yields a signal to the
result of applying the function `f` to the values of `args`.

The resulting signal only has a value when all arguments have values.

A single-entry cache is utilized to avoid reinvoking `f` unnecessarily.

</dd>

</dl>

`apply` also has a lazy counterpart:

<dl>

<dt>lazy_apply(f, args...)</dt><dd>

`lazy_apply(f, args...)`, where `args` are all signals, yields a signal to the
result of lazily applying the function `f` to the values of `args`.

The resulting signal only has a value when all arguments have values.

No caching is performing, so every time the signal is accessed, the function is
invoked.

</dd>

</dl>

The primary advantage of this form is that it doesn't require a context
parameter (it doesn't cache and thus doesn't require any internal data). This is
handy for implementing operators and in similar situations where providing a
context parameters would be awkward. At the moment, this is only intended to be
used in those situations, so the implementation only supports functions of one
or two arguments.

lift()
------

alia also allows you to 'lift' a raw C++ function up to the level of working on
signals. Consider again the line from above where we apply `is_prime` to the
signal `n`:

```cpp
auto n_is_prime = apply(ctx, is_prime, n);
```

This could also be written like this:

```cpp
auto n_is_prime = lift(is_prime)(ctx, n);
```

`lift(is_prime)` yields a version of `is_prime` that conforms to the standard
interface conventions of reactive alia code. Specifically, it takes a context as
its first argument, its other arguments are signals, and it returns a signal.

(The lifted version of `is_prime` would call `apply` internally.)

Lifting is especially useful when dealing with higher-order function
application.

`lift` also has a lazy counterpart: `lazy_lift`. Similar to `lazy_apply`, the
lazy lifted version of a function doesn't take a context as its first argument.
