#pragma once
#include <jstm/types.hpp>

namespace jstm::graphics {

struct glyph {
  u16 bitmap_offset;
  u8 width;
  u8 height;
  u8 x_advance;
  i8 x_offset;
  i8 y_offset;
};

struct font {
  const u8* bitmap;
  const glyph* glyphs;
  u8 first;
  u8 last;
  u8 y_advance;
};

}  // namespace jstm::graphics
