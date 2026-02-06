#pragma once

#include <jstm/types.hpp>

namespace jstm {

constexpr u16 rgb565(u8 r, u8 g, u8 b) {
  return static_cast<u16>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr u8 rgb565_r(u16 c) {
  return static_cast<u8>(((c >> 11) & 0x1F) * 255 / 31);
}

constexpr u8 rgb565_g(u16 c) {
  return static_cast<u8>(((c >> 5) & 0x3F) * 255 / 63);
}

constexpr u8 rgb565_b(u16 c) { return static_cast<u8>((c & 0x1F) * 255 / 31); }

struct color {
  u16 raw = 0;

  constexpr color() = default;
  constexpr explicit color(u16 val) : raw{val} {}
  constexpr color(u8 r, u8 g, u8 b) : raw{rgb565(r, g, b)} {}

  constexpr bool operator==(const color&) const = default;
  constexpr explicit operator u16() const { return raw; }
};

namespace colors {

inline constexpr color black{0x0000};
inline constexpr color white{0xFFFF};
inline constexpr color red{0xF800};
inline constexpr color green{0x07E0};
inline constexpr color blue{0x001F};
inline constexpr color cyan{0x07FF};
inline constexpr color magenta{0xF81F};
inline constexpr color yellow{0xFFE0};
inline constexpr color orange{0xFD20};
inline constexpr color purple{0x8010};
inline constexpr color gray{0x8410};
inline constexpr color dark_gray{0x4208};
inline constexpr color light_gray{0xC618};

}  // namespace colors

}  // namespace jstm
