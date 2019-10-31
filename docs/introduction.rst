Introduction
============

alia is an IMGUI library, a GUI library with an immediate mode API. In traditional (retained mode) GUI libraries, the library stores a hierarchy of widget objects that represent the current on-screen GUI. The structure of the GUI remains static unless the application explicitly alters the hierarchy of objects by adding new objects, removing old objects, or updating existing objects. Manually managing widget objects like this is a burden on the application programmer, and it often discourages the creation of highly dynamic UIs.

IMGUI is designed for dynamic UIs. There is no implicit assumption that the UI remains static from frame to frame. Instead of supplying a static description of the UI, the application supplies the library with a function that dynamically defines the UI. Whenever the UI needs to be updated, the library calls this function to produce a fresh UI specification. The function can take into account dynamic application state to produce different UIs as the state changes.

Another way of saying this is that alia is a reactive GUI library.

This section is designed to introduce you to alia's way of thinking. The following examples will guide you through how some familiar UI concepts look in immediate mode. Note that the examples in this section are intentionally simple and unrealistic. More realistic examples will be presented in later sections.

Hello World
-----------

The following is a UI function that produces a simple UI with a single string of text. ::

    void do_ui(context& ctx)
    {
        do_text(ctx, "hello, world!");
    }

There is no state associated with this UI, so the function produces the same UI every time it's called (i.e., this UI is entirely static). For cases like this, there isn't much difference between the IMGUI approach and the traditional approach, but it's important to remember that the call to do_text() isn't creating a text widget. Instead, it's specifying, each frame, that there is a text widget in the UI with the value "hello, world!".

Event Handling
--------------

A common first example in GUI libraries is a button that changes the value of a text display, so let's see how that looks in alia... ::

   std::string text = "push the button!";

   void do_ui(context& ctx)
   {
       do_text(ctx, text);
       if (do_button(ctx, "push me"))
           text = "thanks!";
   }

The do_button function returns true when it's pressed, which allows you to write the event handler in the same place that you specify the widget. In alia, all code related to a widget is typically localized to one single block, not spread out over creation functions, updates, callbacks, etc. Another benefit of this approach is that the event handler has access to the stack frame of the UI specification function, which is important in more complex UIs.

Also notice that the text display doesn't have some internal label property that we change. In an IMGUI, widgets don't store their values internally. Instead, their values are passed into them each frame by the UI function. This allows their values to change in a programmatic way. In this case, the text widget always displays the current value of the variable 'text', which changes when the button is pressed.

Controls
--------

::

   bool checked = false;

   void do_ui(context& ctx)
   {
       do_check_box(ctx, inout(&checked), "check me");
       do_text(ctx, checked ? "thanks" : "please");
   }

A check box is an example of a control, a widget that manipulates a value (a boolean in this case). Like the text display, the check box doesn't have an internal value to manipulate. Instead, the UI function provides it with a reference to one. In this way, the check box can directly manipulate variables in the application. As before, we can change the value of 'checked' from within the application and the results will be immediately reflected in the UI. Similarly, we can use the value of 'checked' as a conditional in the specification of other parts of the UI, like the text display in this example.

The ``inout(&checked)`` syntax specifies that the check box will read its value from the variable ``checked`` and also write back any changes to that same variable. There are other ways of getting values in and out of controls in alia that will be explained later, but for now we'll focus on this simple method.

Synchronized Control
--------------------

Because controls manipulate variables in the application, synchronizing multiple controls is trivial in an IMGUI system. The following example shows a slider and a text control that both manipulate the same variable. Changing one changes the other.

::

    int n;

    void do_ui(context& ctx)
    {
        do_slider(ctx, inout(&n), 0, 100);
        do_text_control(ctx, inout(&n));
    }

Computed Values
---------------

Similarly, it's trivial to display values that are computed dynamically from user inputs. For example, the following UI features two numeric inputs and a text display that shows their sum. ::

   int x, y;

   void do_ui(context& ctx)
   {
       do_text_control(ctx, inout(&x));
       do_text_control(ctx, inout(&y));
       do_text(ctx, x + y);
   }

Conditional Widgets
-------------------

The true power of IMGUI really shows in GUIs where parts of the UI are added or removed depending on the state of the program. The following UI has a check box and a text display that's only present when the box is checked. ::

    bool checked = false;

    void do_ui(context& ctx)
    {
        do_check_box(ctx, inout(&checked), "show text");
        alia_if (checked)
        {
            do_text(ctx, "hello");
        }
        alia_end
    }

The ``alia_if`` macro behaves exactly like a normal if statement, but it does some magic behind the scenes to make alia's data management work. This is discussed in more detail in the Control Flow section.

Widget Lists
------------

You can even use loops to specify repeating sections of UI. This is especially useful when you have a list in your application and you want to provide a UI for each item in the list. ::

    struct contact
    {
        std::string name;
        std::string phone;
    };

    std::list<contact> contacts;

    void do_ui(context& ctx)
    {
        alia_for(std::list<contact>::iterator i = contacts.begin();
            i != contacts.end(); ++i)
        {
            do_text_control(ctx, inout(&i->name));
            do_text_control(ctx, inout(&i->phone));
            do_separator(ctx);
        }
        alia_end
        if (do_button(ctx, "add contact"))
            contacts.push_back(contact());
    }

Composition and Abstraction
---------------------------

As you've seen, alia widgets are functions, and even the entire application UI is represented as a function. It should come as no surprise that arbitrary subsections of the UI can also be represented as functions. The correspondence between functions and UI components means we can use the natural composition and abstraction mechanisms of functions to build up and break down the structure of the UI. As a simple example, the do_ui() function from the last example could be refactored as follows. ::

    void do_contact_ui(context& ctx, contact& c)
    {
        do_text_control(ctx, inout(&c.name));
        do_text_control(ctx, inout(&c.phone));
        do_separator(ctx);
    }

    void do_ui(context& ctx)
    {
        alia_for (std::list<contact>::iterator i = contacts.begin();
            i != contacts.end(); ++i)
        {
            do_contact_ui(ctx, *i);
        }
        alia_end
        if (do_button(ctx, "add contact"))
            contacts.push_back(contact());
    }

The advantage of using functions rather than objects to represent UI components is that we don't need to create and destroy object instances. Instead, we just call the functions for whichever UI components we want for that frame. Of course, objects have associated state, and you might be thinking we've lost that by using functions instead, but as you'll see the the next section, that's not the case.
