#pragma once

#include "hyper_common.hh"

namespace hyper
{
  struct colour
  {
    u8 _r;
    u8 _g;
    u8 _b;
    u8 _a;
  };

  enum colour_preset
    {
      BLACK = 0,
      WHITE,
      RED,
      GREEN,
      BLUE,
      PURPLE,
      OLIVE,

      COUNT
    };

  inline colour get_colour_from_preset (colour_preset preset)
  {
    switch (preset)
      {
      case BLACK:
        return { 0x00, 0x00, 0x00, 0xFF };
      case WHITE:
        return { 0xFF, 0xFF, 0xFF, 0xFF };
      case RED:
        return { 0xFF, 0x00, 0x00, 0xFF };
      case GREEN:
        return { 0x00, 0xFF, 0x00, 0xFF };
      case BLUE:
        return { 0x00, 0x00, 0xFF, 0xFF };
      case PURPLE:
        return { 0xA0, 0x20, 0xF0, 0xFF };
      case OLIVE:
        return { 0x80, 0x80, 0x00, 0xFF };
      default:
        return { 0x00, 0x00, 0x00, 0xFF };
      }

  }

  inline u32 get_colour_uint (colour colour)
  {
    return (u32) (colour._r) << 24 | (u32) (colour._g) << 16 | (u32) (colour._b) << 8 | (u32) colour._a;
  }
};
