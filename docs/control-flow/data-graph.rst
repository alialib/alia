The Data Graph
==============

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
