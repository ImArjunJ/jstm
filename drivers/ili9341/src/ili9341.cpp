#include <jstm/drivers/ili9341.hpp>
#include <jstm/log.hpp>
#include <jstm/time.hpp>

namespace jstm::drivers {

result<void> ili9341::init() {
  hw_reset();

  write_command(ili9341_cmd::SWRESET);
  delay_ms(50);

  write_command(0x28);

  write_command(0xCF);
  write_data_byte(0x00);
  write_data_byte(0x83);
  write_data_byte(0x30);

  write_command(0xED);
  write_data_byte(0x64);
  write_data_byte(0x03);
  write_data_byte(0x12);
  write_data_byte(0x81);

  write_command(0xE8);
  write_data_byte(0x85);
  write_data_byte(0x01);
  write_data_byte(0x79);

  write_command(0xCB);
  write_data_byte(0x39);
  write_data_byte(0x2C);
  write_data_byte(0x00);
  write_data_byte(0x34);
  write_data_byte(0x02);

  write_command(0xF7);
  write_data_byte(0x20);

  write_command(0xEA);
  write_data_byte(0x00);
  write_data_byte(0x00);

  write_command(ili9341_cmd::PWCTR1);
  write_data_byte(0x26);

  write_command(ili9341_cmd::PWCTR2);
  write_data_byte(0x11);

  write_command(ili9341_cmd::VMCTR1);
  write_data_byte(0x35);
  write_data_byte(0x3E);

  write_command(ili9341_cmd::VMCTR2);
  write_data_byte(0xBE);

  write_command(ili9341_cmd::MADCTL);
  write_data_byte(madctl::MX | madctl::BGR);

  write_command(ili9341_cmd::PIXFMT);
  write_data_byte(0x55);

  write_command(ili9341_cmd::FRMCTR1);
  write_data_byte(0x00);
  write_data_byte(0x1B);

  write_command(0xF2);
  write_data_byte(0x08);

  write_command(ili9341_cmd::GAMMASET);
  write_data_byte(0x01);

  write_command(ili9341_cmd::GMCTRP1);
  write_data_byte(0x1F);
  write_data_byte(0x1A);
  write_data_byte(0x18);
  write_data_byte(0x0A);
  write_data_byte(0x0F);
  write_data_byte(0x06);
  write_data_byte(0x45);
  write_data_byte(0x87);
  write_data_byte(0x32);
  write_data_byte(0x0A);
  write_data_byte(0x07);
  write_data_byte(0x02);
  write_data_byte(0x07);
  write_data_byte(0x05);
  write_data_byte(0x00);

  write_command(ili9341_cmd::GMCTRN1);
  write_data_byte(0x00);
  write_data_byte(0x25);
  write_data_byte(0x27);
  write_data_byte(0x05);
  write_data_byte(0x10);
  write_data_byte(0x09);
  write_data_byte(0x3A);
  write_data_byte(0x78);
  write_data_byte(0x4D);
  write_data_byte(0x05);
  write_data_byte(0x18);
  write_data_byte(0x0D);
  write_data_byte(0x38);
  write_data_byte(0x3A);
  write_data_byte(0x1F);

  write_command(ili9341_cmd::CASET);
  write_data_byte(0x00);
  write_data_byte(0x00);
  write_data_byte(0x00);
  write_data_byte(0xEF);

  write_command(ili9341_cmd::PASET);
  write_data_byte(0x00);
  write_data_byte(0x00);
  write_data_byte(0x01);
  write_data_byte(0x3F);

  write_command(0xB7);
  write_data_byte(0x07);

  write_command(ili9341_cmd::DFUNCTR);
  write_data_byte(0x0A);
  write_data_byte(0x82);
  write_data_byte(0x27);
  write_data_byte(0x00);

  write_command(ili9341_cmd::SLPOUT);
  delay_ms(100);
  write_command(ili9341_cmd::DISPON);
  delay_ms(100);
  write_command(ili9341_cmd::RAMWR);

  width_ = NATIVE_WIDTH;
  height_ = NATIVE_HEIGHT;

  backlight(true);

  log::info("ili9341 initialized (%dx%d)", width_, height_);
  return ok();
}

void ili9341::hw_reset() {
  if (!rst_port_) return;
  HAL_GPIO_WritePin(rst_port_, rst_pin_, GPIO_PIN_RESET);
  delay_ms(5);
  HAL_GPIO_WritePin(rst_port_, rst_pin_, GPIO_PIN_SET);
  delay_ms(150);
}

void ili9341::set_rotation(u8 rotation) {
  rotation_ = rotation % 4;
  u8 m;

  switch (rotation_) {
    case 0:
      m = madctl::MX | madctl::BGR;
      width_ = NATIVE_WIDTH;
      height_ = NATIVE_HEIGHT;
      break;
    case 1:
      m = madctl::MV | madctl::BGR;
      width_ = NATIVE_HEIGHT;
      height_ = NATIVE_WIDTH;
      break;
    case 2:
      m = madctl::MY | madctl::BGR;
      width_ = NATIVE_WIDTH;
      height_ = NATIVE_HEIGHT;
      break;
    case 3:
      m = madctl::MX | madctl::MY | madctl::MV | madctl::BGR;
      width_ = NATIVE_HEIGHT;
      height_ = NATIVE_WIDTH;
      break;
    default:
      m = madctl::MX | madctl::BGR;
      break;
  }

  send_command(ili9341_cmd::MADCTL, &m, 1);
  set_addr_window(0, 0, width_, height_);
}

void ili9341::fill(u16 color) {
  set_addr_window(0, 0, width_, height_);
  u32 total = static_cast<u32>(width_) * height_;
  while (total--) {
    *data_ = color;
  }
}

void ili9341::pixel(u16 x, u16 y, u16 color) {
  set_addr_window(x, y, 1, 1);
  *data_ = color;
}

void ili9341::blit(u16 x, u16 y, u16 w, u16 h, const u16* buf) {
  set_addr_window(x, y, w, h);
  u32 total = static_cast<u32>(w) * h;
  for (u32 i = 0; i < total; ++i) {
    *data_ = buf[i];
  }
}

void ili9341::backlight(bool on) {
  if (!bl_port_) return;
  HAL_GPIO_WritePin(bl_port_, bl_pin_, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void ili9341::write_command(u8 cmd) { *cmd_ = static_cast<u16>(cmd); }

void ili9341::write_data_byte(u8 val) { *data_ = static_cast<u16>(val); }

void ili9341::write_data_word(u16 val) { *data_ = val; }

void ili9341::send_command(u8 cmd, const u8* params, u8 len) {
  write_command(cmd);
  for (u8 i = 0; i < len; ++i) {
    write_data_byte(params[i]);
  }
}

void ili9341::set_addr_window(u16 x, u16 y, u16 w, u16 h) {
  u16 x_end = x + w - 1;
  u16 y_end = y + h - 1;

  write_command(ili9341_cmd::CASET);
  write_data_byte(x >> 8);
  write_data_byte(x & 0xFF);
  write_data_byte(x_end >> 8);
  write_data_byte(x_end & 0xFF);

  write_command(ili9341_cmd::PASET);
  write_data_byte(y >> 8);
  write_data_byte(y & 0xFF);
  write_data_byte(y_end >> 8);
  write_data_byte(y_end & 0xFF);

  write_command(ili9341_cmd::RAMWR);
}

}  // namespace jstm::drivers
