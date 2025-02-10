#pragma once

#include <cstdint>
#include <utility>
#include <cmath>

using f32 = float;
using f64 = double;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using u8  = std::uint8_t;

namespace hyper
{
  template <typename T>
  inline constexpr bool
  is_power_of_two (T a)
  {
    return ((a) != 0) && ((a) & ((a) - 1)) == 0;
  }

  inline constexpr uint64_t
  kilobytes (int32_t n)
  {
    return n * 1024ull;
  }

  inline constexpr uint64_t
  megabytes (int32_t n)
  {
    return kilobytes (n) * 1024ull;
  }

  template <typename T>
  inline constexpr void
  swap (T &a, T &b)
  {
    return std::swap (a, b);
  }

  template <typename T>
  inline constexpr T
  floor (T a)
  {
    return std::floor (a);
  }
};
