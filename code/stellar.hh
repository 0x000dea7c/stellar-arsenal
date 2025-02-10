#pragma once

#include "hyper_common.hh"

namespace stellar
{
  struct config
  {
    f32 _target_fps;
    struct
    {
      i32 _width;
      i32 _height;
    } resolution;
    bool _vsync;
  };

  // NOTE: everything is on meters
  struct world
  {
    f32 _width;
    f32 _height;
    f32 _meters_per_pixel;
  };

  struct camera
  {
    struct
    {
      f32 _width;
      f32 _height;
    } fov;
    struct
    {
      f32 _x;
      f32 _y;
    } position;
  };

  struct state
  {
    bool _running;
  };
};
