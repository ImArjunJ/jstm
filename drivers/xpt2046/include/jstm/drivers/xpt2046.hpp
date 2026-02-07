#pragma once

#include <jstm/core.hpp>
#include <jstm/hal/gpio.hpp>
#include <jstm/hal/spi_bus.hpp>

namespace jstm::drivers {

namespace xpt2046_cmd {
inline constexpr u8 READ_X = 0xD0;
inline constexpr u8 READ_Y = 0x90;
inline constexpr u8 READ_Z1 = 0xB0;
inline constexpr u8 READ_Z2 = 0xC0;
}  // namespace xpt2046_cmd

struct touch_bounds {
  i16 raw_x_min = 370;
  i16 raw_x_max = 3800;
  i16 raw_y_min = 200;
  i16 raw_y_max = 3800;

  constexpr touch_bounds() = default;
  constexpr touch_bounds(i16 xmin, i16 xmax, i16 ymin, i16 ymax)
      : raw_x_min{xmin}, raw_x_max{xmax}, raw_y_min{ymin}, raw_y_max{ymax} {}
};

struct touch_calibration {
  i16 sx_raw_min = 200;
  i16 sx_raw_max = 3800;
  i16 sy_raw_min = 370;
  i16 sy_raw_max = 3800;
  bool swap_xy = true;
  bool invert_x = false;
  bool invert_y = false;
};

class xpt2046 {
 public:
  static constexpr u8 SAMPLES = 16;
  static constexpr u16 PRESSURE_THRESHOLD = 200;
  static constexpr u8 DEBOUNCE_READS = 3;
  static constexpr u16 RAW_MIN = 150;
  static constexpr u16 RAW_MAX = 3900;
  static constexpr u16 MAX_SPREAD = 300;

  xpt2046(hal::spi_bus& spi, hal::output_pin& cs, hal::input_pin* irq = nullptr)
      : spi_{spi}, cs_{cs}, irq_{irq} {}

  ~xpt2046() = default;

  xpt2046(const xpt2046&) = delete;
  xpt2046& operator=(const xpt2046&) = delete;

  result<void> init();

  bool touched() const;
  point read();

  point read_raw();
  u16 pressure();

  void set_calibration(touch_calibration cal) { cal_ = cal; }
  void set_bounds(touch_bounds bounds) {
    bounds_ = bounds;
    apply_rotation();
  }
  void set_screen_size(u16 w, u16 h) {
    screen_w_ = w;
    screen_h_ = h;
  }

  void set_rotation(u8 rotation);

  const touch_calibration& calibration() const { return cal_; }
  const touch_bounds& bounds() const { return bounds_; }

  static constexpr i16 INVALID_COORD = -1;
  static constexpr point INVALID_POINT = {INVALID_COORD, INVALID_COORD};

 private:
  u16 read_channel(u8 cmd);
  u16 read_channel_burst(u8 cmd);
  void apply_rotation();

  hal::spi_bus& spi_;
  hal::output_pin& cs_;
  hal::input_pin* irq_;

  touch_bounds bounds_;
  touch_calibration cal_;
  u8 rotation_ = 1;
  u16 screen_w_ = 240;
  u16 screen_h_ = 320;
};

static_assert(touch_source<xpt2046>);

}  // namespace jstm::drivers
