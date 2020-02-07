Control Flow
============

.. digraph:: data_graph

   a -> b -> e;
   b -> c -> d;

   edge[style=invis];
   d -> e;


.. .. digraph:: data_graph

..    a -> "b[0]" -> d;
..    "b[0]" -> "b[1]" -> "b[2]";
..    "b[0]" -> "c[0]";
..    "b[1]" -> "c[1]";
..    "b[2]" -> "c[2]";

..    edge[style=invis];
..    "c[2]" -> d;

..    { rank=same "c[0]" "b[1]" }
..    { rank=same "c[1]" "b[2]" }

Loops & Conditionals
--------------------

.. todo: alia_if/else (with signals), alia_switch/alia_case, for_each

.. todo: Mention ALIA_STRICT_CONDITIONALS.

.. todo: alia_for, alia_while, alia_if/else (with booleans), alia_switch (with raw values)
