#include <jstm/core.hpp>
#include <jstm/drivers/ili9341.hpp>
#include <jstm/drivers/xpt2046.hpp>
#include <jstm/graphics/canvas.hpp>
#include <jstm/hal/fmc_lcd.hpp>
#include <jstm/hal/gpio.hpp>
#include <jstm/hal/spi_bus.hpp>
#include <jstm/time.hpp>

using namespace jstm;

int main() {
  hal::system_init();
  delay_ms(100);

  const u8 rotation = 3;

  auto lcd_addr = hal::fmc_lcd_init();

  hal::output_pin rst(GPIOB, GPIO_PIN_11, true);
  hal::output_pin bl(GPIOF, GPIO_PIN_6, false);

  drivers::ili9341_fmc_config lcd_cfg{
      .cmd_addr = lcd_addr.cmd,
      .data_addr = lcd_addr.data,
      .rst_port = GPIOB,
      .rst_pin = GPIO_PIN_11,
      .bl_port = GPIOF,
      .bl_pin = GPIO_PIN_6,
  };
  drivers::ili9341 display(lcd_cfg);

  auto r = display.init();
  if (!r) {
    log::error("display init failed: %s", r.error().message);
    while (true) delay_ms(1000);
  }

  display.set_rotation(rotation);

  graphics::canvas<drivers::ili9341> canvas(display);

  hal::spi_bus touch_spi(SPI1, {.baudrate_prescaler = SPI_BAUDRATEPRESCALER_128,
                                .cpol = SPI_POLARITY_LOW,
                                .cpha = SPI_PHASE_1EDGE,
                                .sck_port = GPIOA,
                                .sck_pin = GPIO_PIN_5,
                                .mosi_port = GPIOA,
                                .mosi_pin = GPIO_PIN_7,
                                .miso_port = GPIOA,
                                .miso_pin = GPIO_PIN_6,
                                .af = GPIO_AF5_SPI1});

  hal::output_pin touch_cs(GPIOC, GPIO_PIN_4, true);
  hal::input_pin touch_irq(GPIOC, GPIO_PIN_5, hal::pull::up);

  drivers::xpt2046 touch(touch_spi, touch_cs, &touch_irq);

  auto t = touch.init();
  if (!t) {
    log::error("touch init failed: %s", t.error().message);
    while (true) delay_ms(1000);
  }

  touch.set_screen_size(display.width(), display.height());
  touch.set_rotation(rotation);

  canvas.clear();

  constexpr u16 palette[] = {
      colors::white.raw,  colors::red.raw,    colors::green.raw,
      colors::blue.raw,   colors::cyan.raw,   colors::magenta.raw,
      colors::yellow.raw, colors::orange.raw,
  };
  constexpr u16 palette_count = sizeof(palette) / sizeof(palette[0]);
  u16 bar_w = canvas.width() / (palette_count + 1);
  u16 bar_y = canvas.height() - 20;

  auto draw_toolbar = [&]() {
    for (u16 i = 0; i < palette_count; ++i) {
      canvas.fill_rect(i * bar_w, bar_y, bar_w, 20, palette[i]);
    }
    u16 clr_x = palette_count * bar_w;
    u16 clr_w = canvas.width() - clr_x;
    canvas.fill_rect(clr_x, bar_y, clr_w, 20, colors::dark_gray.raw);
  };

  draw_toolbar();

  u16 brush_color = colors::white.raw;
  constexpr i16 brush_size = 5;

  while (true) {
    if (touch.touched()) {
      auto p = touch.read();

      if (p.x >= 0 && p.x < canvas.width() && p.y >= 0 &&
          p.y < canvas.height()) {
        if (p.y >= bar_y) {
          u16 idx = p.x / bar_w;
          if (idx < palette_count) {
            brush_color = palette[idx];
          } else {
            canvas.clear();
            draw_toolbar();
          }
        } else {
          i16 half = brush_size / 2;
          canvas.fill_circle(p.x, p.y, half, brush_color);
        }
      }
    }

    delay_ms(10);
  }
}
