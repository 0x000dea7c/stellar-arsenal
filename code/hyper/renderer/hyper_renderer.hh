#pragma once

#include "hyper.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

namespace hyper
{
  void set_background_colour (Renderer_context *, Colour);

  void draw_triangle_outline (Renderer_context *, std::array<Vec2<f32>, 3> const &, Colour);

  void draw_triangle_filled (Renderer_context *, std::array<Vec2<f32>, 3> const &, Colour);

  void draw_circle_outline (Renderer_context *, f32, f32, f32, Colour);

  void draw_circle_filled (Renderer_context *, f32, f32, f32, Colour);

  void draw_line (Renderer_context *, Vec2<f32> const &, Vec2<f32> const&, Colour);

  void draw_quad_filled (Renderer_context *, Vec2<f32> const &, f32, f32, Colour);
};
