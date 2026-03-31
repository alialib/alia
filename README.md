# A Library for Interactive Applications

Alia (pronounced uh-LEE-uh) is a declarative UI library for C/C++. It aims to
achieve the polished, modern aesthetics of web-style interfaces while still
living in the low-level world of memory arenas, draw commands, and C ABIs.

Ultimately, the goal is to help address the gap that Ben Visness describes in
this post (and the talk linked at the bottom of the post):

https://bvisness.me/high-level/

In Ben's terms, Alia aims to be a "high-level" UI library, but lower in the
stack, where it's still possible to worry about cycles and bytes.

> [!WARNING]
> This code isn't currently usable. Alia is in a state of upheaval as I try to
> bring it more in line with the [Handmade](https://handmade.network/) ethos.

## Current Goals

**Modern Aesthetics** - Support animations, gradients, shadows, OKLCH, etc. out
of the box. It should be easy to match the appearance of Material, Bootstrap,
Tailwind, etc. with minimal effort. Don't require frames of lag.

**Declarative, Compositional** - Multi-pass IMGUI-style API with layout
resolved between passes. Components compose as simple function calls. External
state is polled. Internal component state is available automagically via
`use_state()`.

**Desktop/Web** - The primary target initially will be graphical desktop apps
(e.g., scientific visualization). Rendering will be via custom GPU commands,
but the library shouldn't require constant refresh. Apps should also be
deployable to the web as WASM, either using normal GPU rendering via
WebGL/WebGPU or by manipulating DOM nodes.

**C ABI, Idiomatic C++ API** - Previous iterations of Alia have shown that C++
can be quite expressive as a declarative UI language, but C++ isn't right for
everything or everyone. Layering the C++ API on top of a C ABI keeps the system
honest and leaves the door open for using Alia in other low-level languages.

**Documentation/Tooling** - Support visual illustration and inspection of
layout results, component trees, internal state, signal data flow, etc.

## Future Goals

**i18n, l10n, a11y, IME, etc.** - While there might be a prototype or two to
demonstrate what's possible, these aren't early requirements for Alia.
