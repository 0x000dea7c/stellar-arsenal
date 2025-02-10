#pragma once

#include "hyper.hh"

// NOTE: this file will be compiled into a shared library and will be
// hot reloaded! Careful with what I put here, especially state
// related stuff.

// Symbols are public by default on GNU/Linux systems if I don't specify
// anything at compile time, but it's a good practice to compile shared
// libraries with hidden visiblity and then being explicit about which
// functions should be visible. What do I know anyway

// Function names should be game_update and game_render for hyper to
// pick them up

#define STELLAR_API __attribute__((visibility ("default")))

extern "C"
{
  STELLAR_API void game_update (hyper::frame_context &);

  STELLAR_API void game_render (hyper::frame_context &);
}
