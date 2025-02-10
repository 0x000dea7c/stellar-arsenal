#pragma once

#include "hyper_common.hh"

namespace stellar
{
  struct config
  {
    f32 _target_fps;
    i32 _width;
    i32 _height;
    bool _vsync;
  };

  struct state
  {
    bool _running;
  };
};
