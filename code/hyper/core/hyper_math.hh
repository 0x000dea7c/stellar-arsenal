#pragma once

#include "hyper_common.hh"

#include <array>

namespace hyper
{
  template <typename T>
  struct Vec2
  {
    T x;
    T y;
  };

  template <typename T>
  struct Vec3
  {
    T x;
    T y;
    T z;
  };

  template <typename T>
  struct Vec4
  {
    T x;
    T y;
    T z;
    T w;
  };

  struct Mat4
  {
    std::array<f32, 16> elements;

    f32 &operator()(i32 row, i32 column)
    {
      return elements[row * 4 + column];
    }

    f32 const &operator()(i32 row, i32 column) const
    {
      return elements[row * 4 + column];
    }
  };

  inline Mat4
  identity ()
  {
    return {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  template <typename T>
  inline constexpr T
  abs (T value)
  {
    return value >= 0 ? value : -value;
  }

  template <typename T>
  inline constexpr T
  max (T &&a, T &&b)
  {
    return a >= b ? a : b;
  }

  template <typename T>
  inline constexpr T
  min (T &&a, T &&b)
  {
    return a < b ? a : b;
  }

  inline constexpr f32
  sqrt (f32 value)
  {
    return std::sqrtf (value);
  }
};
