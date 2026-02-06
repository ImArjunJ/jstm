#include <algorithm>
#include <jstm/drivers/xpt2046.hpp>
#include <jstm/log.hpp>

namespace jstm::drivers {

result<void> xpt2046::init() {
  cs_.high();

  read_channel(xpt2046_cmd::READ_X);
  read_channel(xpt2046_cmd::READ_Y);

  log::info("xpt2046 initialized (screen %dx%d)", screen_w_, screen_h_);
  return ok();
}

u16 xpt2046::read_channel(u8 cmd) {
  u8 tx[3] = {cmd, 0x00, 0x00};
  u8 rx[3] = {};

  cs_.low();
  spi_.transfer(std::span<const u8>{tx, 3}, std::span<u8>{rx, 3});
  cs_.high();

  return static_cast<u16>(((rx[1] << 8) | rx[2]) >> 3) & 0x0FFF;
}

bool xpt2046::touched() const {
  if (irq_ && irq_->read()) {
    return false;
  }
  return const_cast<xpt2046*>(this)->pressure() > PRESSURE_THRESHOLD;
}

u16 xpt2046::pressure() { return read_channel(xpt2046_cmd::READ_Z1); }

point xpt2046::read_raw() {
  u16 x_samples[SAMPLES];
  u16 y_samples[SAMPLES];

  for (u8 i = 0; i < SAMPLES; ++i) {
    x_samples[i] = read_channel(xpt2046_cmd::READ_X);
    y_samples[i] = read_channel(xpt2046_cmd::READ_Y);
  }

  std::sort(x_samples, x_samples + SAMPLES);
  std::sort(y_samples, y_samples + SAMPLES);

  constexpr u8 skip = SAMPLES / 4;
  u32 x_sum = 0, y_sum = 0;
  constexpr u8 count = SAMPLES - 2 * skip;

  for (u8 i = skip; i < SAMPLES - skip; ++i) {
    x_sum += x_samples[i];
    y_sum += y_samples[i];
  }

  return {static_cast<i16>(x_sum / count), static_cast<i16>(y_sum / count)};
}

void xpt2046::set_rotation(u8 rotation) {
  rotation_ = rotation % 4;
  apply_rotation();
}

void xpt2046::apply_rotation() {
  switch (rotation_) {
    case 0:
      cal_.swap_xy = true;
      cal_.invert_x = true;
      cal_.invert_y = false;
      cal_.sx_raw_min = bounds_.raw_y_min;
      cal_.sx_raw_max = bounds_.raw_y_max;
      cal_.sy_raw_min = bounds_.raw_x_min;
      cal_.sy_raw_max = bounds_.raw_x_max;
      break;
    case 1:
      cal_.swap_xy = false;
      cal_.invert_x = true;
      cal_.invert_y = false;
      cal_.sx_raw_min = bounds_.raw_x_min;
      cal_.sx_raw_max = bounds_.raw_x_max;
      cal_.sy_raw_min = bounds_.raw_y_min;
      cal_.sy_raw_max = bounds_.raw_y_max;
      break;
    case 2:
      cal_.swap_xy = true;
      cal_.invert_x = false;
      cal_.invert_y = true;
      cal_.sx_raw_min = bounds_.raw_y_min;
      cal_.sx_raw_max = bounds_.raw_y_max;
      cal_.sy_raw_min = bounds_.raw_x_min;
      cal_.sy_raw_max = bounds_.raw_x_max;
      break;
    case 3:
      cal_.swap_xy = false;
      cal_.invert_x = true;
      cal_.invert_y = true;
      cal_.sx_raw_min = bounds_.raw_x_min;
      cal_.sx_raw_max = bounds_.raw_x_max;
      cal_.sy_raw_min = bounds_.raw_y_min;
      cal_.sy_raw_max = bounds_.raw_y_max;
      break;
  }
}

point xpt2046::read() {
  auto raw = read_raw();

  i16 rx = cal_.swap_xy ? raw.y : raw.x;
  i16 ry = cal_.swap_xy ? raw.x : raw.y;

  i32 sx = static_cast<i32>(rx - cal_.sx_raw_min) * (screen_w_ - 1) /
           (cal_.sx_raw_max - cal_.sx_raw_min);
  i32 sy = static_cast<i32>(ry - cal_.sy_raw_min) * (screen_h_ - 1) /
           (cal_.sy_raw_max - cal_.sy_raw_min);

  if (cal_.invert_x) sx = (screen_w_ - 1) - sx;
  if (cal_.invert_y) sy = (screen_h_ - 1) - sy;

  return {std::clamp<i16>(static_cast<i16>(sx), 0, screen_w_ - 1),
          std::clamp<i16>(static_cast<i16>(sy), 0, screen_h_ - 1)};
}

}  // namespace jstm::drivers
