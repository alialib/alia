alia-docs
=========

[![Deployment](https://github.com/alialib/alia-docs/actions/workflows/docs-deployment.yml/badge.svg?branch=live)](https://github.com/alialib/alia-docs/actions/workflows/docs-deployment.yml)
[![Staging](https://github.com/alialib/alia-docs/actions/workflows/docs-staging.yml/badge.svg)](https://github.com/alialib/alia-docs/actions/workflows/docs-staging.yml)

This the documentation for alia. It's deployed here:

https://alia.dev

Building
--------

Building the docs requires Linux (or the Windows Subsystem for Linux.)

1. [Install Emscripten](
   https://emscripten.org/docs/getting_started/downloads.html) and activate it
   for your shell.

2. Build the demos:
   ```shell
   mkdir build
   cd build
   emcmake cmake ..
   make -j
   ```

3. Run `scripts/link-docs.sh`. This sets up the `www` directory with the
   documentation using symbolic links for (almost) everything, so when you edit
   the `.md` files or rebuild the demos, the effects will propagate
   immediately.

Once you've built the docs, you can run a web server from the `www` directory
to serve the docs. One way to do this is with the provided Python script.
```shell
python2 ../scripts/run-web-server.py
```

If you use this script, you can navigate to
[localhost:8006](http://localhost:8006) to see it.
