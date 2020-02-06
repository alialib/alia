void
do_greeting_ui(dom::context ctx, bidirectional<string> name)
{
    do_text(ctx, "What's your name?");

    // Allow the user to input their name.
    do_input(ctx, name);

    // Greet the user.
    alia_if(name != "")
    {
        do_text(ctx, "Hello, " + name + "!");
    }
    alia_end
}
