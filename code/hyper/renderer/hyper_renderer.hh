#pragma once

#include "hyper.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

namespace hyper
{
  void set_background_colour (Renderer_context *, Colour);

  void draw (Renderer_context *);

  void draw_triangle_outline (Renderer_context *, std::array<Vec2<f32>, 3> const&, Colour);
};
