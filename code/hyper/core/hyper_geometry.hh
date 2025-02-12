//
// Basic wrappers for primitives
//
#pragma once

#include "hyper_math.hh"

#include <array>

namespace hyper
{
  struct Triangle
  {
    std::array<Vec2<f32>, 3> vertices;
  };

  struct Line
  {
    Vec2<f32> start;
    Vec2<f32> end;
  };

  struct Circle
  {
    Vec2<f32> center;
    f32 radius;
  };

  struct Quad
  {
    // Bottom left
    Vec2<f32> position;
    f32 width;
    f32 height;
  };
};
