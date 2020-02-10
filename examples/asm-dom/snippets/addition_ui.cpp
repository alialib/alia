void
do_addition_ui(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    dom::do_text(ctx, "Enter two numbers to add:");

    dom::do_input(ctx, a);
    dom::do_input(ctx, b);

    dom::do_text(ctx, a + b);
}
