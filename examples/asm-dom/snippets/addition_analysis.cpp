void
do_addition_analysis(
    dom::context ctx, bidirectional<double> a, bidirectional<double> b)
{
    do_text(ctx, "Enter two numbers to add:");

    dom::do_input(ctx, a);
    dom::do_input(ctx, b);

    auto sum = a + b;
    alia_if(sum > 0)
    {
        dom::do_text(ctx, "The sum is positive!");
    }
    alia_else_if(sum < 0)
    {
        dom::do_text(ctx, "The sum is negative!");
    }
    alia_else
    {
        dom::do_text(ctx, "The sum is zero!");
    }
    alia_end
}
