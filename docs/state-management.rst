State Management
================

Associating State
-----------------

Imagine that we want to modify the contact UI from the last section so that it has two modes: viewing and editing. Contacts are read-only in viewing mode, but the editing UI has the same editing controls as before. We could do that like this... ::

    struct contact
    {
        contact() : editing(false) {}
        bool editing;
        std::string name;
        std::string phone;
    };

    void do_contact_ui(context& ctx, contact& c)
    {
        alia_if (c.editing)
        {
            do_text_control(ctx, inout(&c.name));
            do_text_control(ctx, inout(&c.phone));
            if (do_button(ctx, "OK"))
                c.editing = false;
        }
        alia_else
        {
            do_text(ctx, c.name);
            do_text(ctx, c.phone);
            if (do_link(ctx, "edit"))
                c.editing = true;
        }
        alia_end
        
        do_separator(ctx);
    }

The above code behaves as desired, but now we've introduced UI-specific state into our contact structure. Ideally, we'd like our model data to be independent of any UI considerations, so we'd like to store the "editing" flag somewhere UI-specific. Fortunately, alia has a facility for this called get_state. ::

    struct contact
    {
        std::string name;
        std::string phone;
    };

    void do_contact_ui(context& ctx, contact& c)
    {
        state_accessor<bool> editing = get_state(ctx, false);
        
        alia_if (get(editing))
        {
            do_text_control(ctx, inout(&c.name));
            do_text_control(ctx, inout(&c.phone));
            if (do_button(ctx, "OK"))
                set(editing, false);
        }
        alia_else
        {
            do_text(ctx, c.name);
            do_text(ctx, c.phone);
            if (do_link(ctx, "edit"))
                set(editing, true);
        }
        alia_end
        
        do_separator(ctx);
    }

get_state retrieves persistent state that's associated with the part of the UI that's currently being specified. It's a templated function, so it can retrieve any type of state. The state persists as long as the part of the UI that it's associated with.
get_state comes in two forms. In the form used above, the second argument is the default value of the state. When the state is created, it's initialized with that value. The second form is used as follows. ::

    state_accessor<bool> editing;
    if (get_state(ctx, &editing))
        set(editing, false);

With this form, the first time get_state is called for a particular piece of UI, it returns true, indicating that the state it retrieved was just created (default constructed). The calling function can use this to initialize the state.
Using get_state, you can associate arbitrary UI-specific state with the portion of the UI that needs it, without anyone outside the function ever knowing. In fact, all along, some of the widgets we've been using have been calling get_state without us knowing. For example, this is how the text controls are able to track their cursor position, text selection, etc.
