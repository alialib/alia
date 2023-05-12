Getting Started
===============

alia/HTML
---------

The easiest way to get started with alia is through the alia/HTML starter
project:

https://github.com/alialib/alia-html-starter

Full instructions are included there!

The Core Library
----------------

If you're brave and want to use the core of alia separately from alia/HTML, the
following instructions apply, but beware that you'll have to spend some time
integrating it with a library that actually provides a user interface.

The core of alia should work on any C++17 compiler. It has no external
dependencies. The following compilers are officially supported:

- Visual C++ 2019 and higher
- GCC 7 and higher
- Clang 5 and higher
- Emscripten 1.39.18 and higher

### Getting It

alia is distributed as a single header (`alia.hpp`) which is attached as an
asset to every release in GitHub. You can download the latest release here:

https://github.com/alialib/alia/releases/latest/download/alia.hpp

If you want to play around with the latest version on the `main` branch, that
header is available via GitHub Pages:

https://alia.dev/alia.hpp

### Using It

alia follows the normal conventions of a header-only library that also includes
implementation (.cpp) code:

*In one and only one of your source files, you should `#define
ALIA_IMPLEMENTAITON` before including `alia.hpp`.*

In your other source files, you can simply include it directly.

There are also some [config options](configuration.md) that you may want to
set.
