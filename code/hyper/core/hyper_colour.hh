#pragma once

#include "hyper_common.hh"

namespace hyper
{
  struct Colour
  {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
  };

  enum Colour_preset
    {
      BLACK = 0,
      WHITE,
      RED,
      GREEN,
      BLUE,
      PURPLE,
      OLIVE,
      GREY,

      COUNT
    };

  inline Colour
  get_colour_from_preset (Colour_preset preset)
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
      case GREY:
        return { 0x80, 0x80, 0x80, 0xFF };
      default:
        return { 0x00, 0x00, 0x00, 0xFF };
      }
  }

  inline u32
  get_colour_uint (Colour colour)
  {
    return (u32) (colour.r) << 24 | (u32) (colour.g) << 16 | (u32) (colour.b) << 8 | (u32) colour.a;
  }
};
