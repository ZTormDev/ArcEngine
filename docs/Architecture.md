# Architecture

Arc Engine is split into two main targets:

- `ArcEngine`: static engine library with SDL2 windowing and BGFX renderer lifecycle.
- `ArcGame`: game executable that depends on the public `Arc` API.

The game should include headers from `engine/include/Arc` and avoid depending on engine source files directly.
