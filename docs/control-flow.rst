Control Flow
============

Control Flow Macros
-------------------

In order for its data management facilities to function properly, alia needs to know about any place where you have widgets inside if statements or loops. You mark these places by using alia macros in place of the normal control flow keywords. We've already seen a few of these, but the complete list is here.

- ``alia_if``
- ``alia_else_if``
- ``alia_else``
- ``alia_switch``
- ``alia_case``
- ``alia_for``
- ``alia_while``

All control blocks must be terminated with ``alia_end``. (As you've already seen, an ``if/else_if/else`` block only needs one ``alia_end`` to terminate the entire block.)

Note that you must not use ``goto`` in your UI code because it circumvents these mechanisms. You should also avoid using ``return`` in the middle of a function, as this will skip over part of the function without alia knowing. ``break`` and ``continue``, however, are OK.

Also note that when we check the return value of ``do_button``, we use a normal if statement. This is because that if statement represents an event handlers, not a conditional block of widgets.

Context Naming
--------------

All of the above macros assume that your UI context variable is named 'ctx'. If that's not the case, each has a corresponding form with an underscore appended to the end of the name where you can manually specify the name of the context variable. For example, ::

    alia_if_ (my_context, editing)
    {
        // ...
    }
    alia_end

Loops with Item Reordering
--------------------------

You may be wondering why we can add items to our contact list but not remove them. It's because removing them would confuse the alia_for macro. alia_for and alia_while associate state with the widgets in the loop body based on iteration count (the first iteration always gets the same data, the second iteration always gets the same data, etc.). When iterating over a list, reordering items in the list will cause them to end up associated with the wrong state.

In cases like this, you have to provide alia with a way to identify items in the list that will remain constant even as the items move around. If list items always remain at the same address (this is the case with STL lists), then you can simply use the address of an item as its ID. In other cases, you can use any info that's unique to an item (e.g., you might have assigned a numeric ID). Here's how our contact list loop looks using IDs. ::

    naming_context nc(ctx);
    for (std::list<contact>::iterator i = contacts.begin(); i != contacts.end(); ++i)
    {
        named_block nb(ctx, make_id(&*i)); // using contact address as block ID
        do_contact_ui(ctx, *i);
    }

Creating a named_block at the top of a scope creates a new data context for the scope. All widgets invoked inside that scope (or in any function called in that scope) will get their data from the block associated with the named_block's ID. Note that since the named_block is already taking care of the data management for the loop body, we can use a normal for loop.

The first line in the example defines a new naming context for the loop. A ``naming_context`` provides a context for ``named_block`` IDs. IDs used within one naming context can be reused within another without conflict. Since we only have one loop in this example, this isn't strictly necessary. However, in a real application, where you might have other loops over the same data, this is a good habit.
