#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <jstm/color.hpp>
#include <jstm/concepts.hpp>
#include <jstm/graphics/font.hpp>
#include <jstm/types.hpp>
#include <memory>

namespace jstm::graphics {

struct image {
  u16 w;
  u16 h;
  const u16* data;

  u16 color_key = 0;
  bool use_color_key = false;
};

template <display D>
class canvas {
 public:
  explicit canvas(D& display) : display_{display} {}

  ~canvas() = default;

  canvas(const canvas&) = delete;
  canvas& operator=(const canvas&) = delete;

  void create_framebuffer() {
    if (!framebuffer_) {
      usize size = static_cast<usize>(width()) * height();
      framebuffer_ = std::make_unique<u16[]>(size);
      clear();
    }
  }

  void destroy_framebuffer() { framebuffer_.reset(); }

  void flush() {
    if (framebuffer_ && framebuffer_dirty_) {
      display_.blit(0, 0, width(), height(), framebuffer_.get());
      framebuffer_dirty_ = false;
    }
  }

  void clear() { fill(clear_color_); }

  void fill(u16 color) {
    if (framebuffer_) {
      usize size = static_cast<usize>(width()) * height();
      for (usize i = 0; i < size; ++i) {
        framebuffer_[i] = color;
      }
      framebuffer_dirty_ = true;
    } else {
      display_.fill(color);
    }
  }

  void set_clear_color(u16 color) { clear_color_ = color; }

  void pixel(i16 x, i16 y, u16 color) {
    if (x < 0 || x >= width() || y < 0 || y >= height()) return;

    if (framebuffer_) {
      framebuffer_[y * width() + x] = color;
      framebuffer_dirty_ = true;
    } else {
      display_.pixel(static_cast<u16>(x), static_cast<u16>(y), color);
    }
  }

  void line(i16 x0, i16 y0, i16 x1, i16 y1, u16 color) {
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
      std::swap(x0, y0);
      std::swap(x1, y1);
    }
    if (x0 > x1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    i16 dx = x1 - x0, dy = std::abs(y1 - y0);
    i16 err = dx / 2, ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
      if (steep)
        pixel(y0, x0, color);
      else
        pixel(x0, y0, color);
      err -= dy;
      if (err < 0) {
        y0 += ystep;
        err += dx;
      }
    }
  }

  void hline(i16 x, i16 y, i16 length, u16 color) {
    if (y < 0 || y >= height()) return;
    i16 x_start = std::max<i16>(0, x);
    i16 x_end = std::min<i16>(width() - 1, x + length - 1);
    if (x_start > x_end) return;

    if (framebuffer_) {
      for (i16 i = x_start; i <= x_end; ++i) {
        framebuffer_[y * width() + i] = color;
      }
      framebuffer_dirty_ = true;
    } else {
      for (i16 i = x_start; i <= x_end; ++i) {
        display_.pixel(static_cast<u16>(i), static_cast<u16>(y), color);
      }
    }
  }

  void vline(i16 x, i16 y, i16 h, u16 color) {
    if (x < 0 || x >= width()) return;
    i16 y_start = std::max<i16>(0, y);
    i16 y_end = std::min<i16>(height() - 1, y + h - 1);
    if (y_start > y_end) return;

    if (framebuffer_) {
      for (i16 i = y_start; i <= y_end; ++i) {
        framebuffer_[i * width() + x] = color;
      }
      framebuffer_dirty_ = true;
    } else {
      for (i16 i = y_start; i <= y_end; ++i) {
        display_.pixel(static_cast<u16>(x), static_cast<u16>(i), color);
      }
    }
  }

  void rect(i16 x, i16 y, i16 w, i16 h, u16 color) {
    hline(x, y, w, color);
    hline(x, y + h - 1, w, color);
    vline(x, y, h, color);
    vline(x + w - 1, y, h, color);
  }

  void fill_rect(i16 x, i16 y, i16 w, i16 h, u16 color) {
    i16 x_start = std::max<i16>(0, x);
    i16 y_start = std::max<i16>(0, y);
    i16 x_end = std::min<i16>(width() - 1, x + w - 1);
    i16 y_end = std::min<i16>(height() - 1, y + h - 1);
    if (x_start > x_end || y_start > y_end) return;

    if (framebuffer_) {
      for (i16 j = y_start; j <= y_end; ++j)
        for (i16 i = x_start; i <= x_end; ++i)
          framebuffer_[j * width() + i] = color;
      framebuffer_dirty_ = true;
    } else {
      for (i16 j = y_start; j <= y_end; ++j)
        for (i16 i = x_start; i <= x_end; ++i)
          display_.pixel(static_cast<u16>(i), static_cast<u16>(j), color);
    }
  }

  void circle(i16 x0, i16 y0, i16 r, u16 color) {
    i16 f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    i16 x = 0, y = r;

    pixel(x0, y0 + r, color);
    pixel(x0, y0 - r, color);
    pixel(x0 + r, y0, color);
    pixel(x0 - r, y0, color);

    while (x < y) {
      if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;
      pixel(x0 + x, y0 + y, color);
      pixel(x0 - x, y0 + y, color);
      pixel(x0 + x, y0 - y, color);
      pixel(x0 - x, y0 - y, color);
      pixel(x0 + y, y0 + x, color);
      pixel(x0 - y, y0 + x, color);
      pixel(x0 + y, y0 - x, color);
      pixel(x0 - y, y0 - x, color);
    }
  }

  void fill_circle(i16 x0, i16 y0, i16 r, u16 color) {
    vline(x0, y0 - r, 2 * r + 1, color);
    i16 f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    i16 x = 0, y = r;

    while (x < y) {
      if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;
      vline(x0 + x, y0 - y, 2 * y + 1, color);
      vline(x0 - x, y0 - y, 2 * y + 1, color);
      vline(x0 + y, y0 - x, 2 * x + 1, color);
      vline(x0 - y, y0 - x, 2 * x + 1, color);
    }
  }

  void draw_image(i16 x, i16 y, u16 img_w, u16 img_h, const u16* data) {
    i16 sx = 0, sy = 0;
    i16 dx = x, dy = y;
    i16 w = static_cast<i16>(img_w);
    i16 h = static_cast<i16>(img_h);

    if (dx < 0) {
      sx = -dx;
      w += dx;
      dx = 0;
    }
    if (dy < 0) {
      sy = -dy;
      h += dy;
      dy = 0;
    }
    if (dx + w > width()) w = width() - dx;
    if (dy + h > height()) h = height() - dy;
    if (w <= 0 || h <= 0) return;

    if (framebuffer_) {
      for (i16 row = 0; row < h; ++row) {
        const u16* src = &data[(sy + row) * img_w + sx];
        u16* dst = &framebuffer_[(dy + row) * width() + dx];
        std::memcpy(dst, src, static_cast<usize>(w) * sizeof(u16));
      }
      framebuffer_dirty_ = true;
    } else {
      for (i16 row = 0; row < h; ++row) {
        display_.blit(static_cast<u16>(dx), static_cast<u16>(dy + row),
                      static_cast<u16>(w), 1, &data[(sy + row) * img_w + sx]);
      }
    }
  }

  void draw_image(i16 x, i16 y, const image& img) {
    if (img.use_color_key) {
      draw_image_keyed(x, y, img);
    } else {
      draw_image(x, y, img.w, img.h, img.data);
    }
  }

  void draw_image_scaled(i16 x, i16 y, u16 dst_w, u16 dst_h, u16 img_w,
                         u16 img_h, const u16* data) {
    for (i16 dy = 0; dy < static_cast<i16>(dst_h); ++dy) {
      i16 screen_y = y + dy;
      if (screen_y < 0 || screen_y >= height()) continue;
      u16 src_y = static_cast<u16>(static_cast<u32>(dy) * img_h / dst_h);
      for (i16 dx = 0; dx < static_cast<i16>(dst_w); ++dx) {
        i16 screen_x = x + dx;
        if (screen_x < 0 || screen_x >= width()) continue;
        u16 src_x = static_cast<u16>(static_cast<u32>(dx) * img_w / dst_w);
        pixel(screen_x, screen_y, data[src_y * img_w + src_x]);
      }
    }
  }

  void draw_image_scaled(i16 x, i16 y, u16 dst_w, u16 dst_h, const image& img) {
    draw_image_scaled(x, y, dst_w, dst_h, img.w, img.h, img.data);
  }

  void set_cursor(i16 x, i16 y) {
    cursor_x_ = x;
    cursor_y_ = y;
  }
  void set_text_color(u16 color) { text_color_ = color; }
  void set_text_background(u16 color) { text_bg_color_ = color; }
  void set_text_size(u8 s) {
    text_size_x_ = s;
    text_size_y_ = s;
  }
  void set_text_size(u8 sx, u8 sy) {
    text_size_x_ = sx;
    text_size_y_ = sy;
  }
  void set_text_wrap(bool wrap) { text_wrap_ = wrap; }
  void set_font(const font* f) { font_ = f; }

  void draw_char(i16 x, i16 y, char c, u16 fg, u16 bg, u8 sx = 1, u8 sy = 1) {
    if (font_) {
      draw_char_font(x, y, c, fg, sx, sy);
    } else {
      draw_char_builtin(x, y, c, fg, bg, sx, sy);
    }
  }

  void write_char(char c) {
    if (c == '\n') {
      cursor_x_ = 0;
      cursor_y_ += (font_ ? font_->y_advance : 8) * text_size_y_;
    } else if (c == '\r') {
      cursor_x_ = 0;
    } else {
      u8 cw = font_ ? font_->glyphs[c - font_->first].x_advance : 6;
      if (text_wrap_ && (cursor_x_ + cw * text_size_x_ > width())) {
        cursor_x_ = 0;
        cursor_y_ += (font_ ? font_->y_advance : 8) * text_size_y_;
      }
      draw_char(cursor_x_, cursor_y_, c, text_color_, text_bg_color_,
                text_size_x_, text_size_y_);
      cursor_x_ += cw * text_size_x_;
    }
  }

  void print(const char* str) {
    while (*str) write_char(*str++);
  }

  void printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    print(buffer);
  }

  void scroll_up(i16 pixels) {
    if (!framebuffer_) return;
    u16 w = width(), h = height();

    for (i16 y = 0; y < h - pixels; y++) {
      std::memcpy(&framebuffer_[y * w], &framebuffer_[(y + pixels) * w],
                  w * sizeof(u16));
    }
    for (i16 y = h - pixels; y < h; y++) {
      for (i16 x = 0; x < w; x++) {
        framebuffer_[y * w + x] = clear_color_;
      }
    }
    framebuffer_dirty_ = true;
  }

  u16 width() const { return display_.width(); }
  u16 height() const { return display_.height(); }
  i16 cursor_x() const { return cursor_x_; }
  i16 cursor_y() const { return cursor_y_; }

 private:
  // clang-format off
  static constexpr u8 font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00,
    0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
    0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62,
    0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00,
    0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00,
    0x08,0x2A,0x1C,0x2A,0x08, 0x08,0x08,0x3E,0x08,0x08,
    0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08,
    0x00,0x60,0x60,0x00,0x00, 0x20,0x10,0x08,0x04,0x02,
    0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00,
    0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31,
    0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39,
    0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
    0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E,
    0x00,0x36,0x36,0x00,0x00, 0x00,0x56,0x36,0x00,0x00,
    0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14,
    0x41,0x22,0x14,0x08,0x00, 0x02,0x01,0x51,0x09,0x06,
    0x32,0x49,0x79,0x41,0x3E, 0x7E,0x11,0x11,0x11,0x7E,
    0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
    0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41,
    0x7F,0x09,0x09,0x01,0x01, 0x3E,0x41,0x41,0x51,0x32,
    0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00,
    0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
    0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x04,0x02,0x7F,
    0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
    0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E,
    0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31,
    0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F,
    0x1F,0x20,0x40,0x20,0x1F, 0x7F,0x20,0x18,0x20,0x7F,
    0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03,
    0x61,0x51,0x49,0x45,0x43, 0x00,0x00,0x7F,0x41,0x41,
    0x02,0x04,0x08,0x10,0x20, 0x41,0x41,0x7F,0x00,0x00,
    0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
    0x00,0x01,0x02,0x04,0x00, 0x20,0x54,0x54,0x54,0x78,
    0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20,
    0x38,0x44,0x44,0x48,0x7F, 0x38,0x54,0x54,0x54,0x18,
    0x08,0x7E,0x09,0x01,0x02, 0x08,0x14,0x54,0x54,0x3C,
    0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00,
    0x20,0x40,0x44,0x3D,0x00, 0x00,0x7F,0x10,0x28,0x44,
    0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78,
    0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
    0x7C,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7C,
    0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20,
    0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C,
    0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
    0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C,
    0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
    0x00,0x00,0x7F,0x00,0x00, 0x00,0x41,0x36,0x08,0x00,
    0x08,0x08,0x2A,0x1C,0x08,
  };
  // clang-format on

  void draw_char_builtin(i16 x, i16 y, char c, u16 fg, u16 bg, u8 sx, u8 sy) {
    if (c < 32 || c > 126) return;
    for (u8 i = 0; i < 5; i++) {
      u8 line = font5x7[(c - 32) * 5 + i];
      for (u8 j = 0; j < 8; j++) {
        if (line & 0x01) {
          if (sx == 1 && sy == 1)
            pixel(x + i, y + j, fg);
          else
            fill_rect(x + i * sx, y + j * sy, sx, sy, fg);
        } else if (bg != fg) {
          if (sx == 1 && sy == 1)
            pixel(x + i, y + j, bg);
          else
            fill_rect(x + i * sx, y + j * sy, sx, sy, bg);
        }
        line >>= 1;
      }
    }
  }

  void draw_char_font(i16 x, i16 y, char c, u16 fg, u8 sx, u8 sy) {
    if (c < font_->first || c > font_->last) return;
    const glyph* g = &font_->glyphs[c - font_->first];
    const u8* bmp = font_->bitmap;
    u16 bo = g->bitmap_offset;
    u8 bit = 0, bits = 0;

    for (u8 yy = 0; yy < g->height; yy++) {
      for (u8 xx = 0; xx < g->width; xx++) {
        if (!(bit++ & 7)) bits = bmp[bo++];
        if (bits & 0x80) {
          if (sx == 1 && sy == 1)
            pixel(x + g->x_offset + xx, y + g->y_offset + yy, fg);
          else
            fill_rect(x + (g->x_offset + xx) * sx, y + (g->y_offset + yy) * sy,
                      sx, sy, fg);
        }
        bits <<= 1;
      }
    }
  }

  void draw_image_keyed(i16 x, i16 y, const image& img) {
    for (i16 row = 0; row < static_cast<i16>(img.h); ++row) {
      i16 screen_y = y + row;
      if (screen_y < 0 || screen_y >= height()) continue;
      for (i16 col = 0; col < static_cast<i16>(img.w); ++col) {
        i16 screen_x = x + col;
        if (screen_x < 0 || screen_x >= width()) continue;
        u16 px = img.data[row * img.w + col];
        if (px != img.color_key) pixel(screen_x, screen_y, px);
      }
    }
  }

  D& display_;
  std::unique_ptr<u16[]> framebuffer_;
  bool framebuffer_dirty_ = false;

  i16 cursor_x_ = 0;
  i16 cursor_y_ = 0;
  u8 text_size_x_ = 1;
  u8 text_size_y_ = 1;
  u16 text_color_ = 0xFFFF;
  u16 text_bg_color_ = 0x0000;
  u16 clear_color_ = 0x0000;
  bool text_wrap_ = true;
  const font* font_ = nullptr;
};

}  // namespace jstm::graphics
