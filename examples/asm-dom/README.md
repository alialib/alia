Experimental asm-dom Wrapper
============================

This is an experimental wrapper for the [asm-dom
library](https://github.com/mbasso/asm-dom). It's intended to illustrate how you
might create a production wrapper for such a library and to allow you to play
around with alia. **This is not intended for real use.** You certainly might be
able to use it to create real apps, but it's not going to be maintained with
that intention.

If you're interested in seeing the actual wrapper code, check out the files
`dom.hpp` and `dom.cpp`.

Building
--------

These instructions are for Linux. (They also work in the Windows Subsystem for
Linux.) Since Emscripten runs on Windows, they should also work there, with some
obvious changes to the commands:

1. [Install Emscripten](
   https://emscripten.org/docs/getting_started/downloads.html) and activate it
   for your shell.

1. Clone this repository and change to this directory:
   ```shell
   git clone https://github.com/tmadden/alia
   cd alia/examples/asm-dom
   ```

1. Get a copy of `alia.hpp`:
   ```shell
   wget https://alia.dev/alia.hpp
   ```

1. Build the project:
   ```shell
   mkdir build
   cd build
   emcmake cmake ..
   make -j$(nproc)
   ```

The Sandbox
-----------

Once you've built the project, you can run a web server from the build directory
to serve the sandbox. One way to do this is with the Python script that alia
provides:

```shell
../../../scripts/run-web-server.py
```

If you use this script, you can navigate to
[localhost:8000](http://localhost:8000) to see (and interact with) the sandbox.

To play around with it, edit `sandbox.cpp`, rerun `make`, and refresh!
