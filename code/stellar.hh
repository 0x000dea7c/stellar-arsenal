#pragma once

#include "hyper_common.hh"

namespace stellar
{
  struct Config
  {
    f32 target_fps;
    struct
    {
      i32 width;
      i32 height;
    } resolution;
    bool vsync;
  };

  struct World
  {
    f32 width;
    f32 height;
    f32 meters_per_pixel;
  };

  struct Camera
  {
    // Center of the view in world space
    f32 x;
    f32 y;
    // Scales the world coordinates to screen coordinates
    f32 zoom;
  };

  struct State
  {
    bool running;
  };
};
