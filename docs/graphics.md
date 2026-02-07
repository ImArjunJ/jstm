# graphics

framebuffered or direct-draw 2d canvas for any display that satisfies the
`jstm::display` concept. header-only.

```cmake
target_link_libraries(my_app jstm_graphics)
```

## canvas

```cpp
drivers::ili9341 display(cfg);
display.init();

graphics::canvas<drivers::ili9341> canvas(display);
canvas.create_framebuffer();  // optional - allocates width*height*2 bytes
```

### framebuffer mode vs direct mode

if you call `create_framebuffer()`, all drawing goes to a ram buffer.
call `flush()` to push the whole buffer to the display at once. this
avoids flicker and is much faster for complex scenes.

without a framebuffer, every draw call goes directly to the display
hardware. fine for simple things but slow for lots of small updates.

a 320×240 framebuffer uses 153,600 bytes. the f746 has 320 kb sram,
so this is feasible but significant.

### drawing primitives

```cpp
canvas.clear();
canvas.fill(colors::black.raw);

canvas.pixel(10, 20, colors::white.raw);
canvas.line(0, 0, 100, 100, colors::red.raw);
canvas.hline(0, 50, 200, colors::green.raw);
canvas.vline(100, 0, 150, colors::blue.raw);

canvas.rect(10, 10, 100, 50, colors::cyan.raw);
canvas.fill_rect(10, 10, 100, 50, colors::cyan.raw);

canvas.circle(160, 120, 40, colors::yellow.raw);
canvas.fill_circle(160, 120, 40, colors::yellow.raw);
```

lines use bresenham's algorithm. circles use the midpoint algorithm.
all coordinates are `i16` and clip to screen bounds automatically.

### images

```cpp
canvas.draw_image(x, y, img_w, img_h, pixel_data);
canvas.draw_image_scaled(x, y, dst_w, dst_h, src_w, src_h, data);
```

there's also an `image` struct with optional color-key transparency:

```cpp
graphics::image img{
  .w = 32, .h = 32,
  .data = sprite_data,
  .color_key = 0x0000,
  .use_color_key = true,
};
canvas.draw_image(10, 10, img);
```

scaling uses nearest-neighbor interpolation.

### text

```cpp
canvas.set_text_color(colors::white.raw);
canvas.set_text_background(colors::black.raw);
canvas.set_cursor(0, 0);

canvas.print("hello, world!");
canvas.printf("count: %d\n", n);
canvas.write_char('A');
```

the default font is a built-in 5×7 pixel font (ascii 32-126). each
character occupies 6×8 pixels (5 wide + 1 spacing, 7 tall + 1 spacing).

text wraps automatically at the screen edge. disable with
`canvas.set_text_wrap(false)`.

scaling: `canvas.set_text_size(2)` doubles both axes.
`canvas.set_text_size(2, 3)` for independent x/y scaling.

### custom fonts

```cpp
canvas.set_font(&my_font);
```

fonts use the `graphics::font` struct with a bitmap + glyph table. the
format is a packed 1-bit-per-pixel bitmap with per-glyph metrics (offset,
width, height, advance). compatible with adafruit gfx font format.

pass `nullptr` to go back to the built-in font.

### scrolling

```cpp
canvas.scroll_up(16);  // scroll framebuffer up by 16 pixels
```

only works in framebuffer mode. the vacated rows are filled with the
clear color.

### accessors

```cpp
canvas.width();
canvas.height();
canvas.cursor_x();
canvas.cursor_y();
```

## font struct

```cpp
struct glyph {
  u16 bitmap_offset;
  u8 width, height;
  u8 x_advance;
  i8 x_offset, y_offset;
};

struct font {
  const u8* bitmap;
  const glyph* glyphs;
  u8 first, last;    // ascii range
  u8 y_advance;      // line height
};
```
