#pragma once

#include <cstdint>

namespace jstm {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = std::size_t;

struct point {
  i16 x = 0;
  i16 y = 0;

  constexpr bool operator==(const point&) const = default;

  constexpr point operator+(point rhs) const {
    return {static_cast<i16>(x + rhs.x), static_cast<i16>(y + rhs.y)};
  }

  constexpr point operator-(point rhs) const {
    return {static_cast<i16>(x - rhs.x), static_cast<i16>(y - rhs.y)};
  }
};

struct size {
  u16 w = 0;
  u16 h = 0;

  constexpr bool operator==(const size&) const = default;

  constexpr u32 area() const { return static_cast<u32>(w) * h; }
};

struct rect {
  point origin;
  size dims;

  constexpr bool operator==(const rect&) const = default;

  constexpr i16 x() const { return origin.x; }
  constexpr i16 y() const { return origin.y; }
  constexpr u16 width() const { return dims.w; }
  constexpr u16 height() const { return dims.h; }

  constexpr i16 right() const { return static_cast<i16>(origin.x + dims.w); }
  constexpr i16 bottom() const { return static_cast<i16>(origin.y + dims.h); }

  constexpr bool contains(point p) const {
    return p.x >= origin.x && p.x < right() && p.y >= origin.y &&
           p.y < bottom();
  }
};

}  // namespace jstm
