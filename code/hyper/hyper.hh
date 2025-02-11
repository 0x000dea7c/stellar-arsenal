#pragma once

#include "hyper_common.hh"
#include "hyper_math.hh"
#include <vector>

#define HYPER_UPDATE_FUNCTION_NAME "game_update"
#define HYPER_RENDER_FUNCTION_NAME "game_render"

namespace hyper
{
  enum class Shape
    {
      triangle,
      line,
      quad,
      circle
    };

  struct Framebuffer
  {
    std::pmr::vector<u32> pixels;
    size_t simd_chunks;
    i32 width;
    i32 height;
    i32 pitch;
  };

  struct Renderer_context
  {
    Framebuffer *framebuffer;
    f32 camera_x;
    f32 camera_y;
    f32 camera_zoom;
    f32 screen_width;
    f32 screen_height;
  };

  struct Frame_context
  {
    Renderer_context *renderer_context;
    u64 last_frame_time;
    f32 fixed_timestep;
    f32 physics_accumulator;
    f32 alpha_rendering;
  };
};
