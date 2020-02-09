Operators and Casts
===================

Basic Operators
---------------

All the basic operators work as you'd expect with signals, producing
signals that carry the result of the operation and are only readable
when both arguments are readable.

All the basic binary operators are provided:

`+` `-` `*` `/` `%` `^` `&` `|` `<<` `>>` `==` `!=` `<` `<=` `>` `>=`

The unary operators `-` and `!` are also provided.

Logical Operators
-----------------

The logical operators (`||` and `&&`) also work as you'd expect them to
work with signals. Of course, like any library-defined logical
operators, they don't provide the same type of short-circuiting behavior
as the built-in versions, but they provide short-circuiting in the
signal space. In particular:

- When the first operand is readable and fully resolves the result of the
  operation, the second operand isn't read.
- When *either* operand is readable and fully resolves the result of the
  operation, the other is allowed to be unreadable. The result will still be
  readable.

The Ternary Operator
--------------------

Since the built-in ternary operator can't be overloaded in C++, alia provides
the equivalent in the form of the `conditional` function. It mimics
`std::conditional`:

Mutating Operators
------------------

Operators that would normally mutate one of the operands (i.e., the compound
assignment operators and the increment/decrement operators) instead produce
*actions* in alia and are covered in that section of the documentation.

Operators and Raw Values
------------------------

Casts
-----

`signal_cast<Value>(signal)`

Subscripts and Field Access
---------------------------

The subscript operator is defined for signals.
