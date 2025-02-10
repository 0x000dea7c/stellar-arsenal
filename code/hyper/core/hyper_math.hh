#pragma once

#include "hyper_common.hh"

#include <array>

namespace hyper
{
  template<typename T>
  struct vec2
  {
    T x;
    T y;
  };

  template<typename T>
  struct vec3
  {
    T x;
    T y;
    T z;
  };

  template<typename T>
  struct vec4
  {
    T x;
    T y;
    T z;
    T w;
  };

  struct mat4
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

  inline mat4
  identity ()
  {
    return {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f,
    };
  }

  inline mat4
  orthographic (f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far)
  {
    mat4 matrix {identity ()};
    matrix(0, 0) = 2.0f / (right - left);
    matrix(1, 1) = 2.0f / (top - bottom);
    matrix(2, 2) = -2.0f / (z_far - z_near);
    matrix(0, 3) = -(right + left) / (right - left);
    matrix(1, 3) = -(top + bottom) / (top - bottom);
    matrix(2, 3) = -(z_far + z_near) / (z_far - z_near);
    return matrix;
  }

  // TODO: operator overloading?
  inline vec4<f32>
  mat4_vec4_mul (mat4 const &matrix, vec4<f32> const& vector)
  {
    vec4<f32> result;
    result.x = matrix(0, 0) * vector.x + matrix(0, 1) * vector.y + matrix(0, 2) * vector.z + matrix(0, 3) * vector.w;
    result.y = matrix(1, 0) * vector.x + matrix(1, 1) * vector.y + matrix(1, 2) * vector.z + matrix(1, 3) * vector.w;
    result.z = matrix(2, 0) * vector.x + matrix(2, 1) * vector.y + matrix(2, 2) * vector.z + matrix(2, 3) * vector.w;
    result.w = matrix(3, 0) * vector.x + matrix(3, 1) * vector.y + matrix(3, 2) * vector.z + matrix(3, 3) * vector.w;
    return result;
  }

  template <typename T>
  inline constexpr T
  abs (T value)
  {
    return value >= 0 ? value : -value;
  }
};
