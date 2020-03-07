Conditionals
============

<script>
    init_alia_demos(['numerical-analysis', 'switch-example']);
</script>

alia_if/else
------------

alia provides the equivalent of `if`, `else if` and `else` as macros. Here again
is a simple example that makes use of all three:

?> The alia control flow macros come in both uppercase and lowercase forms.
   Traditionally, they have been lowercase to more closely resemble the actual
   C++ keywords that they mimic. However, the uppercase form makes it more
   obvious to readers (and clang-format) that the macros are indeed macros.
   Ultimately, it's up to you which style you prefer. If you want to be strict
   in your project, you can disable the lowercase form in [the
   configuration](configuration.md).

[source](numerical.cpp ':include :fragment=analysis')

<div class="demo-panel">
<div id="numerical-analysis"></div>
</div>

The conditions that you provide to `alia_if` (and `alia_else_if`) can be signals
or raw C++ values. If a signal is provided and the signal has no value, alia
considers the condition *neither true nor false.* As such, the code dependent on
that statement isn't executed, but any subsequent `else` blocks are *also not
considered.* A condition without a value essentially terminates the entire
sequence of `alia_if/else` statements. As you can see in the above example,
before `n` is given a value, none of the `do_text` calls are executed.

As with all alia control flow macros, `alia_if` blocks must be terminated with
`alia_end`.

### Context Naming

All alia control flow macros need access to the alia *context*, so they assume
it's available in the `ctx` variable. It will make your life much easier if you
adopt the convention of always naming your context like this. If, however, you
find yourself in a situation where that's inconvenient or impossible, the macros
are all available in an alternate form that ends with an underscore and takes
the context as a first argument. For example:

```cpp
alia_if_(my_oddly_named_context, n < 0)
{
    dom::do_text(my_oddly_named_context, "Hi!");
}
alia_end
```

(These also come in uppercase form as well.)

alia_switch
-----------

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
