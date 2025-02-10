#pragma once

#include "hyper.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

namespace hyper
{
  // TODO: this will most likely change
  void renderer_init (f32 width, f32 height);

  void set_background_colour (renderer_context *, colour);

  void draw (renderer_context *);

  void draw_triangle_outline (renderer_context *, std::array<vec2<f32>, 3> const&, colour);
};
