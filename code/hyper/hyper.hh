#pragma once

#include "hyper_common.hh"
#include "hyper_math.hh"
#include <vector>

#define HYPER_UPDATE_FUNCTION_NAME "game_update"
#define HYPER_RENDER_FUNCTION_NAME "game_render"

namespace hyper
{
  enum class shape
    {
      triangle,
      line,
      quad,
      circle
    };

  struct vertex_buffer
  {
    vec3<f32> _positions;
    vec2<f32> _normals;
    vec2<f32> _texture_coordinates;
    vec4<f32> _colours;
    u32 _count;
    shape _shape;
  };

  struct framebuffer
  {
    std::pmr::vector<u32> _pixels;
    size_t _simd_chunks;
    i32 _width;
    i32 _height;
    i32 _pitch;
  };

  struct renderer_context
  {
    framebuffer *_framebuffer;
    vertex_buffer *_vertex_buffer;
  };

  struct frame_context
  {
    renderer_context *_renderer_context;
    u64 _last_frame_time;
    f32 _fixed_timestep;
    f32 _physics_accumulator;
    f32 _alpha_rendering;
  };
};
