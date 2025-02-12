#pragma once

#include "hyper_common.hh"
#include "hyper_geometry.hh"
#include "hyper_colour.hh"

#include <array>

namespace stellar
{
  struct Ship
  {
    struct Thrusters
    {
      std::array<hyper::Quad, 2> data;
      hyper::Colour colour;
      f32 width;
      f32 height;
    } thrusters;
    struct Wings
    {
      hyper::Triangle left;
      hyper::Triangle right;
      hyper::Colour colour;
    } wings;
    struct Body
    {
      hyper::Triangle data;
      hyper::Colour colour;
      f32 width;
      f32 height;
    } body;
    struct Cockpit
    {
      hyper::Triangle data;
      hyper::Colour colour;
      f32 width;
      f32 height;
    } cockpit;
  };

  struct Star
  {
    hyper::Circle body;
    hyper::Colour colour;
  };

  struct Game_data
  {
    std::array<Star, 1024> stars;
    Ship ship;
  };

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
