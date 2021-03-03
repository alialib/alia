Elements
========

<script>
    init_alia_demos(['basic-element', 'element-attribute',
        'valueless-element-attribute', 'boolean-element-attribute',
        'simple-element-property', 'dynamic-element-property',
        'element-handler', 'element-action',
        'element-text', 'element-text-node',
        'element-content',
        'simple-html-fragment', 'dynamic-html-fragment',
        'fetched-html-fragment', 'overridden-html-fragment']);
</script>

The heart of a web app is obviously its HTML content. This section documents
the interfaces that alia/HTML provides for specifying HTML elements in your
app.

element()
---------

The most flexible (and verbose) way to specify HTML elements is to use
`html::element()`:

<dl>

<dt>element_handle<br>element(ctx, tag)</dt><dd>

Specifies the existence of an HTML element with the tag `tag`.

</dd>

</dl>

For example, the following creates an `<hr>` (horizontal rule) element.

[source](elements.cpp ':include :fragment=basic-element')

<div class="demo-panel">
<div id="basic-element"></div>
</div>

Most common HTML elements also have dedicated shortcut functions, so the above
can also be written more succinctly as:

[source](elements.cpp ':include :fragment=shorter-basic-element')

The `element_handle` returned by `element()` can be used to provide further
specifications for the element...

### Attributes

Calling `.attr()` on an `element_handle` will specify an attribute on the
element:

[source](elements.cpp ':include :fragment=element-attribute')

<div class="demo-panel">
<div id="element-attribute"></div>
</div>

As in HTML, boolean attributes can be specified with no value:

[source](elements.cpp ':include :fragment=valueless-element-attribute')

<div class="demo-panel">
<div id="valueless-element-attribute"></div>
</div>

And unlike HTML, they can also be explicitly specified with a boolean value:

[source](elements.cpp ':include :fragment=boolean-element-attribute')

<div class="demo-panel">
<div id="boolean-element-attribute"></div>
</div>

(As with most things in alia, attributes will accept either a raw boolean value
or a boolean signal.)

!> You should **NOT** use `if` statements (or even `alia_if` statements) to
   conditionally set attributes. This will confuse the implementation. Instead,
   use boolean attributes as above and use functional-style logic (e.g.,
   `alia::condition` or `alia::apply`) to compute the current value of your
   attribute. (`alia::mask` can also be useful to mask attributes that are
   conditionally present.)

#### Classes

Class attributes receive special treatment in alia/HTML because they are so
commonly used and because their native representation can be painful to work
with in C++.

Two separate interfaces are provided for specifying class tokens: one that
mimics the HTML `class` attribute and one that is more token-based and
C++-friendly:

`.classes()` takes a space-separated string of class tokens and applies them
all to the element. You should only call this once per element and the list of
tokens should be static. (No signal-based interface is provided.)

```cpp
div(ctx).classes("badge badge-primary text-wrap")
```

`.class_()` takes a single token and applies it to the element. It can be
invoked multiple times to add different tokens, and those tokens can be dynamic
signals. It will respond properly if their values change or disappear.

```cpp
div(ctx).class_("badge").class_("badge-primary").class_("text-wrap");
```

`.class_()` can optionally take a second argument (a raw boolean or boolean
signal) which masks the class token:

```cpp
// The "text-wrap" class token will only be present when `wrapped` is true.
div(ctx).class_("text-wrap", wrapped);
```

You can mix and match the two interfaces. It's recommended that you use
`classes()` for the initial, static classes associated with an element and then
use `.class_()` to dynamically add in others.

```cpp
div(ctx).classes("badge badge-primary").class_("text-wrap", wrapped);
```

!> As with attributes in general, the same incompatibility with `if/alia_if`
   statements applies to `classes()` and `class_()`.

### Properties

Element properties are specified similarly to attributes, but using `prop()`
instead of `attr()`:

[source](elements.cpp ':include :fragment=simple-element-property')

<div class="demo-panel">
<div id="simple-element-property"></div>
</div>

Of course, since properties are meant to be dynamic, you would usually use the
signal-based interface:

[source](elements.cpp ':include :fragment=dynamic-element-property')

<div class="demo-panel">
<div id="dynamic-element-property"></div>
</div>

Note that this does **not** create a two-way binding between the property and
the signal (as you can probably see if you switch between toggling the checkbox
directly and toggling it via the button). To do this, you need to register an
event handler. Read on.

### Event Handling

#### handler() - The Low-Level Interface

`.handler()` allows you to attach a handler for a Javascript event on your
element. This allows us to fix our checkbox demo from above:

[source](elements.cpp ':include :fragment=element-handler')

<div class="demo-panel">
<div id="element-handler"></div>
</div>

alia/HTML currently runs only within Emscripten, so the event is passed as an
`emscripten::val`, and you can use [its
interface](https://emscripten.org/docs/api_reference/val.h.html) to inspect it.
(It's possible this will change at some point in alia/HTML's evolution.)

#### on() - The Action-Based Interface

When you don't care about the actual Javascript event object, you can use
`on()`, which directly works with actions. Since we can guess the new state of
our checkbox, we can also implement the demo like this:

[source](elements.cpp ':include :fragment=element-action')

<div class="demo-panel">
<div id="element-action"></div>
</div>

### Text

`.text()` specifies the text of an element:

[source](elements.cpp ':include :fragment=element-text')

<div class="demo-panel">
<div id="element-text"></div>
</div>

Elements that are normally used for text have shortcuts to accept the text
directly, so the above could also be written as:

```cpp
p(ctx, "This is a paragraph of text.");
```

The free function `text()` also exists to specify an independent HTML text
node:

[source](elements.cpp ':include :fragment=element-text-node')

<div class="demo-panel">
<div id="element-text-node"></div>
</div>

### Nesting

`.content()` allows you to specify nested content within an element:

[source](elements.cpp ':include :fragment=element-content')

<div class="demo-panel">
<div id="element-content"></div>
</div>

HTML elements that normally serve as containers also have shortcuts for
including content. The above could also be written as follows:

```cpp
span(ctx, [&] {
    text(ctx, "This is some normal text. ");
    b(ctx, "This is some bold text.");
});
```

Since it's common in such cases to apply one or more classes to the container,
the shortcuts also have an option for that, e.g.:

```cpp
span(ctx, "text-monospace", [&] {
    text(ctx, "This is some normal text. ");
    b(ctx, "This is some bold text.");
});
```

or:

```cpp
span(ctx, "text-monospace").content([&] {
    text(ctx, "This is some normal text. ");
    b(ctx, "This is some bold text.");
});
```

Rooting Your Content
--------------------

In order to actually integrate your alia/HTML elements into a web app, you need
to root them somewhere in the HTML document. The usual way to do this is to
create a placeholder element in the document and use `placeholder_root` in your
app to root your content at that element:

```cpp
// Any elements specified within `my_app_content` will appear at the HTML
// element with the ID "app-content".
placeholder_root(ctx, "app-content", [&] {
    my_app_content(ctx);
});
```

Raw HTML Fragments
------------------

Often, we only need very limited programmatic control over our HTML content,
and in those cases, transliterating the HTML to C++ can be unnecessarily
painful. We can often solve this by providing the skeleton of our app in
regular HTML and then selectively rooting our C++ content at particular
placeholders in the HTML tree. However, what if we want to invert that
relationship and invoke raw HTML content from within C++?

To address this need, alia provides `html_fragment`, which takes a string of
raw HTML and incorporates it into the UI:

[source](elements.cpp ':include :fragment=simple-html-fragment')

<div class="demo-panel">
<div id="simple-html-fragment"></div>
</div>

Of course, `html_fragment` works with signals and can handle dynamic content:

[source](elements.cpp ':include :fragment=dynamic-html-fragment')

<div class="demo-panel">
<div id="dynamic-html-fragment"></div>
</div>

The function `fetch_text` is designed to work together with `html_fragment` to
incorporate external HTML files into your app.

[source](elements.cpp ':include :fragment=fetched-html-fragment')

<div class="demo-panel">
<div id="fetched-html-fragment"></div>
</div>

Like `element`, `html_fragment` returns a handle. This handle can be used to
override elements within the fragment with alia-driven content:

[source](elements.cpp ':include :fragment=overridden-html-fragment')

<div class="demo-panel">
<div id="overridden-html-fragment"></div>
</div>

This allows you to endlessly nest alia-driven content within HTML content and
vice versa.
