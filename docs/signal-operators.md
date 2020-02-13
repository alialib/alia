Operators
=========

Basic Operators
---------------

All the basic operators work as you'd expect with signals, producing other
signals that carry the result of the operation and are only readable when both
arguments are readable.

All the basic binary operators are provided:

`+` `-` `*` `/` `%` `^` `&` `|` `<<` `>>` `==` `!=` `<` `<=` `>` `>=`

The unary operators `-` and `!` are also provided.

Logical Operators
-----------------

The logical operators (`||` and `&&`) also work as you'd expect with signals. Of
course, like any library-defined logical operators, they don't provide the same
type of short-circuiting behavior as the built-in versions, but they provide
short-circuiting in the signal space. In particular:

- When the first operand is readable and fully resolves the result of the
  operation, the second operand isn't read.

  For example, the following code is safe because `null_signal` is never read:

  ```cpp
  auto null_signal =
      lamda_reader(always_readable, []() { return *(bool*)nullptr; });
  alia_if(value(false) && null_signal)
  {
      do_text(ctx, "This will never be displayed.");
  }
  alia_end
  ```

- When *either* operand is readable and fully resolves the result of the
  operation, the other is allowed to be unreadable. The result will still be
  readable.

  In other words, all of the following are readable signals:

  ```cpp
  value(true) || empty<bool>()
  value(false) && empty<bool>()
  empty<bool>() || value(true)
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

Field Access
------------

The `->*` operator can be used for accessing fields within a structure. For
example, if `p` is a signal carrying a structure of type `point`, then
`p->*&point::x` is a signal carrying the field `x` within `p`. Since this syntax
is a little verbose, alia provides the `ALIA_FIELD` macro. `ALIA_FIELD(p, x)` is
equivalent to the above (as is `alia_field(p, x)` if you allow alia to define
lowercase macros).
