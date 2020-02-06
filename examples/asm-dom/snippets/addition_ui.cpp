void
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    do_text(ctx, "Enter two numbers to add:");

    do_input(ctx, a);
    do_input(ctx, b);

    do_text(ctx, a + b);
}
