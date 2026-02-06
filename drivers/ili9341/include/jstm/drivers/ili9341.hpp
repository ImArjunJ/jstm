#pragma once

#include <jstm/core.hpp>
#include <jstm/hal/gpio.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::drivers {

namespace ili9341_cmd {
inline constexpr u8 NOP = 0x00;
inline constexpr u8 SWRESET = 0x01;
inline constexpr u8 SLPOUT = 0x11;
inline constexpr u8 DISPON = 0x29;
inline constexpr u8 CASET = 0x2A;
inline constexpr u8 PASET = 0x2B;
inline constexpr u8 RAMWR = 0x2C;
inline constexpr u8 MADCTL = 0x36;
inline constexpr u8 VSCRSADD = 0x37;
inline constexpr u8 PIXFMT = 0x3A;
inline constexpr u8 FRMCTR1 = 0xB1;
inline constexpr u8 DFUNCTR = 0xB6;
inline constexpr u8 PWCTR1 = 0xC0;
inline constexpr u8 PWCTR2 = 0xC1;
inline constexpr u8 VMCTR1 = 0xC5;
inline constexpr u8 VMCTR2 = 0xC7;
inline constexpr u8 GMCTRP1 = 0xE0;
inline constexpr u8 GMCTRN1 = 0xE1;
inline constexpr u8 GAMMASET = 0x26;
}  // namespace ili9341_cmd

namespace madctl {
inline constexpr u8 MY = 0x80;
inline constexpr u8 MX = 0x40;
inline constexpr u8 MV = 0x20;
inline constexpr u8 ML = 0x10;
inline constexpr u8 BGR = 0x08;
inline constexpr u8 MH = 0x04;
}  // namespace madctl

struct ili9341_fmc_config {
  volatile u16* cmd_addr = nullptr;
  volatile u16* data_addr = nullptr;
  GPIO_TypeDef* rst_port = nullptr;
  u16 rst_pin = 0;
  GPIO_TypeDef* bl_port = nullptr;
  u16 bl_pin = 0;
};

class ili9341 {
 public:
  static constexpr u16 NATIVE_WIDTH = 240;
  static constexpr u16 NATIVE_HEIGHT = 320;

  explicit ili9341(ili9341_fmc_config cfg)
      : cmd_{cfg.cmd_addr},
        data_{cfg.data_addr},
        rst_port_{cfg.rst_port},
        rst_pin_{cfg.rst_pin},
        bl_port_{cfg.bl_port},
        bl_pin_{cfg.bl_pin} {}

  ~ili9341() = default;

  ili9341(const ili9341&) = delete;
  ili9341& operator=(const ili9341&) = delete;

  result<void> init();
  void set_rotation(u8 rotation);

  u16 width() const { return width_; }
  u16 height() const { return height_; }

  void fill(u16 color);
  void pixel(u16 x, u16 y, u16 color);
  void blit(u16 x, u16 y, u16 w, u16 h, const u16* data);

  void backlight(bool on);

 private:
  void hw_reset();
  void write_command(u8 cmd);
  void write_data_byte(u8 val);
  void write_data_word(u16 val);
  void send_command(u8 cmd, const u8* params, u8 len);
  void set_addr_window(u16 x, u16 y, u16 w, u16 h);

  volatile u16* cmd_;
  volatile u16* data_;
  GPIO_TypeDef* rst_port_;
  u16 rst_pin_;
  GPIO_TypeDef* bl_port_;
  u16 bl_pin_;

  u16 width_ = NATIVE_WIDTH;
  u16 height_ = NATIVE_HEIGHT;
  u8 rotation_ = 0;
};

static_assert(display<ili9341>);

}  // namespace jstm::drivers
