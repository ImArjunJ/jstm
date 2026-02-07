# drivers

display and touch drivers for the stm32f746. each is a separate static
library so you only link what you need. you can create your own concepts and respective drivers relatively trivially.

## ili9341

240x320 tft display driver over the fmc parallel interface. satisfies `jstm::display`.

```cmake
target_link_libraries(my_app jstm_ili9341)
```

### wiring

the fmc data bus uses fixed pins on ports D and E. the address line A0
on PF0 selects command vs data.

| signal | pin                                                |
| ------ | -------------------------------------------------- |
| D0-D15 | PD14, PD15, PD0, PD1, PE7-PE15, PD4, PD5, PD8-PD10 |
| A0     | PF0                                                |
| NE1    | PD7                                                |
| NOE    | PD4                                                |
| NWE    | PD5                                                |
| RST    | PB11 (or any gpio)                                 |
| BL     | PF6 (or any gpio)                                  |

### usage

```cpp
auto addr = hal::fmc_lcd_init();

drivers::ili9341_fmc_config cfg{
  .cmd_addr  = addr.cmd,
  .data_addr = addr.data,
  .rst_port  = GPIOB, .rst_pin = GPIO_PIN_11,
  .bl_port   = GPIOF, .bl_pin  = GPIO_PIN_6,
};

drivers::ili9341 display(cfg);
display.init();
display.set_rotation(1);

display.fill(0x0000); // clear to black
display.pixel(10, 20, 0xF800); // red pixel
display.blit(0, 0, w, h, framebuf); // block transfer
display.backlight(true);
```

## xpt2046

resistive touch controller over spi. satisfies `jstm::touch_source`.

```cmake
target_link_libraries(my_app jstm_xpt2046)
```

### wiring

| signal   | pin            |
| -------- | -------------- |
| SPI SCK  | PA5            |
| SPI MOSI | PA7            |
| SPI MISO | PA6            |
| CS       | PC4            |
| IRQ      | PC5 (optional) |

the xpt2046 shares a spi bus but needs a slow clock - the driver uses
`SPI_BAUDRATEPRESCALER_128` which gives ~422 kHz from the 54 mhz apb1
clock (spi1 is on apb2 at 108 mhz, so actually ~844 kHz).

### usage

```cpp
hal::spi_bus spi(SPI1, { /* see touch_paint example */ });
hal::output_pin cs(GPIOC, GPIO_PIN_4, true);
hal::input_pin irq(GPIOC, GPIO_PIN_5, hal::pull::up);

drivers::xpt2046 touch(spi, cs, &irq);
touch.init();
touch.set_screen_size(320, 240);
touch.set_rotation(1);

if (touch.touched()) {
  auto p = touch.read();   // calibrated screen coordinates
  // p.x, p.y
}
```

### calibration

the driver reads 16 samples per axis, sorts them, and averages the
middle 8 (trimmed mean) to reject noise.

raw adc values are mapped to screen coordinates using a calibration
struct. the defaults work for most ili9341 modules but you can tune them:

```cpp
touch.set_bounds({
  .raw_x_min = 370, .raw_x_max = 3800,
  .raw_y_min = 200, .raw_y_max = 3800,
});
```

the rotation logic automatically swaps and inverts axes to match the
display rotation, so you only need to calibrate once and then
`set_rotation()` handles the rest.

### raw access

```cpp
point raw = touch.read_raw();  // uncalibrated 12-bit adc values
u16 pressure = touch.pressure();
```
