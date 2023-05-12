Operators
=========

Basic Operators
---------------

All the basic operators work as you'd expect with signals, producing other
signals that carry the result of the operation and only have a value when both
arguments have a value.

All the basic binary operators are provided:

`+` `-` `*` `/` `%` `^` `&` `|` `<<` `>>` `==` `!=` `<` `<=` `>` `>=`

The unary operators `-`, `*` and `!` are also provided.

Logical Operators
-----------------

The logical operators (`||` and `&&`) also work as you'd expect with signals. Of
course, like any library-defined logical operators, they don't provide the same
type of short-circuiting behavior as the built-in versions, but they provide
short-circuiting in the signal space. In particular:

- When the first operand has a value and fully resolves the result of the
  operation, the second operand isn't read.

  For example, the following code is safe because although the signal
  `bad_signal > 0` is constructed, it's never actually read from, which means
  that `bad_reference` is never accessed:

  ```cpp
  auto bad_signal = direct(bad_reference);
  alia_if(value(false) && bad_signal > 0)
  {
      html::p(ctx, "Of course, this will never be displayed.");
  }
  alia_end
  ```

- When *either* operand has a value and fully resolves the result of the
  operation, the other is allowed to be empty. The result will still have a
  value.

  In other words, all of the following signals have values:

  ```cpp
  value(true) || empty<bool>()
  empty<bool>() || value(true)
  value(false) && empty<bool>()
  empty<bool>() && value(false)
  ```

The Ternary Operator
--------------------

Since the built-in ternary operator can't be overloaded in C++, alia provides
the equivalent in the form of the `conditional` function. It mimics
`std::conditional`:

<dl>

<dt>conditional(b, t, f)</dt><dd>

Yields `t` if `b` carries `true` (or a value that behaves similarly in a
conditional context) and `f` if `b` carries `false` (or a value that behaves
similarly).

</dd>

</dl>


Mutating Operators
------------------

Operators that would normally mutate one of the operands (e.g., `+=` or `--`)
instead produce *actions* in alia and are covered in [that
section](actions.md?id=actions) of the documentation.

Mixing in Raw Values
--------------------

Wherever possible, the signal operators allow you to mix signals and raw
(non-signal) C++ values. So, for example, if `n` is an integer signal, `n + 1`
is also a valid signal, as is `n > 1 && n < 4`, etc.

Subscripts
----------

The subscript operator is defined for signals that carry containers. The index
can either be another signal or a raw value.

?> alia prefers safe subscripts for signals, so for containers that offer an
   `at` indexer, the signal subscript operator will invoke that instead of
   using the actual subscript operator on the raw container.

Field Access
------------

The `->*` operator can be used for accessing fields within a structure. For
example, if `p` is a signal carrying a structure of type `point`, then
`p->*&point::x` is a signal carrying the field `x` within `p`. Since this
syntax is a little verbose, alia provides the `ALIA_FIELD` macro.
`ALIA_FIELD(p, x)` is equivalent to the above (as is `alia_field(p, x)` if you
allow alia to define lowercase macros).
