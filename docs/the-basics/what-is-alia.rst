What is alia?
=============

The motivation for alia is perhaps best illustrated with an example. Imagine
that we are implementing a simple mapping application that has to display points
of interest to the user. This could be implemented very simply as follows.

.. note: For simplicity, this example uses global data and functions to access
   our application state. This isn't meant to be a guide on how to structure
   application code.

::

    void draw_map(cairo_t* cr)
    {
        draw_map_background(cr);
        for (auto const& poi : points_of_interest)
        {
            // Draw a circle at the POI's location in the POI's color.
            cairo_set_source_rgba (cr, poi.r, poi.g, poi.b, 0.6);
            cairo_set_line_width (cr, 6.0);
            cairo_arc (cr, poi.x, poi.y, 10.0, 0, 2 * pi);
            cairo_fill (cr);
        }
    }

We'll simply call this function whenever we need to draw (or redraw) our map,
and our job is done!

And of course, we can easily refactor this. ::

    void draw_poi(cairo_t* cr, point_of_interest const& poi)
    {
        // Draw a circle at the POI's location in the POI's color.
        cairo_set_source_rgba (cr, poi.r, poi.g, poi.b, 0.6);
        cairo_set_line_width (cr, 6.0);
        cairo_arc (cr, poi.x, poi.y, 10.0, 0, 2 * pi);
        cairo_fill (cr);
    }

    void draw_map(cairo_t* cr)
    {
        draw_map_background(cr);
        for (auto const& poi : points_of_interest)
        {
            draw_poi(cr, poi);
        }
    }

When our application behavior can be implemented as simple function calls, this
type of refactoring is easy, and it makes it straightforward to implement other
interesting behavior as well. ::

    void draw_map(cairo_t* cr)
    {
        draw_map_background(cr);
        for (auto const& poi : points_of_interest)
        {
            // Do some filtering.
            if (poi_matches_user_interests(poi))
                draw_poi(cr, poi);
        }
        // Dynamically add some other points that aren't in the list.
        if (location_tracking_enabled())
            draw_poi(cr, get_user_location());
        draw_poi(cr, get_user_home());
    }

An important feature of this code is that calling ``draw_poi`` is essentially
declaring "This POI is part of our map right now." This allows us to use
familiar C++ control flow mechanisms like ``if`` and ``for`` to analyze our
application state and decide which POIs to include. Our map display
automatically updates according to that logic whenever the application state
changes.

In other words, this simple implementation of a map display is *reactive*!

Now imagine that we want the user to be able to interact with our POIs. We need
to detect when the user has clicked on a POI and open a popup with information
about that POI and some options for interacting with it. Suddenly, we need a
richer interface to our graphical POIs than ``draw_poi`` provides (i.e., a way
to deliver mouse events), along with some hidden state behind that interface
(e.g., is the popup open?).

All of this of course screams "POIs are objects!" And while an object is clearly
a good fit for these new requirements, naively introducing a list of such
objects into our application unfortunately breaks the reactive property that we
had before. For example, we can no longer use a simple ``if`` statement to
decide if the user's location should be part of the map. Instead, we'll need to
detect when location tracking has been enabled or disabled and add/remove the
corresponding object accordingly. This is much more cumbersome and error-prone
code than what we had before, and it scales very poorly with application
complexity.

alia is designed to solve this problem. (There are of course other solutions to
it, but as far as I know, none are quite like alia.) alia provides the tools to
allow our new interactive application to be written like this:

::

    void do_map(app_context ctx)
    {
        do_map_background(ctx);

        alia::for_each(ctx, points_of_interest,
            [&](auto poi)
            {
                alia_if (poi_matches_user_interests(poi))
                {
                    do_poi(ctx, poi);
                }
                alia_end
            });

        alia_if (location_tracking_enabled())
        {
            do_poi(ctx, get_user_location());
        }
        alia_end

        do_poi(ctx, get_user_home());
    }

We've replaced ``draw_map`` with ``do_map``. Whereas ``draw_map`` was only
concerned with drawing, ``do_map`` is capable of handling mouse events as well.
Essentially, it decides what POIs are on the map and routes events to them (via
the ``ctx`` parameter).

Somewhere inside ``do_poi``, you'd find code that detects what event is being
processed and either draws the POI or does some mouse logic. Importantly, each
call to ``do_poi`` can also store arbitrary data within ``ctx``, which allows it
to maintain state about the user interaction or manage its own widgets. All of
this is invisible to ``do_map``. (Except that ``do_map`` must be written with
special forms of ``if`` and ``for``, which allow alia's data magic to work.)

Effectively, each call to ``do_poi`` has the capabilities of a normal C++
object: it can respond to multiple types of events, and it can maintain
arbitrary internal state. ``do_poi`` could even be a simple wrapper around a
class. However, unlike normal C++ objects, we don't have to explicitly create
and destroy calls to ``do_poi``. Whatever calls are encountered during a call to
``do_map`` uniquely specify the set of POIs that are present in the map.
``do_map`` is *reactive*!

So, alia is a library that allows you to write C++ application code reactively,
even when your application's functionality requires the power of objects, and
even when you want to utilize one or more libraries with an object-oriented
interface. The core of alia supplies the mechanics to make the above style of
programming possible, and it's intended to make the development of bindings to
other libraries fairly straightforward.
