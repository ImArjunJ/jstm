# core

the core module is header-only. it gives you the basic vocabulary types that
the rest of the library builds on. link `jstm_core` and you get everything.

```cmake
target_link_libraries(my_app jstm_core)
```

## types

```
u8  u16  u32  u64
i8  i16  i32  i64
usize
```

also has `point`, `size`, and `rect` for 2d geometry:

```cpp
jstm::point p{10, 20};
jstm::size  s{320, 240};
jstm::rect  r{p, s};

r.contains({15, 25});  // true
r.right();             // 330
r.area();              // only on size 76800
```

`point` supports `+` and `-`. everything is `constexpr`.

## result

thin wrapper around `std::expected<T, error>`. every fallible function in
jstm returns `result<T>` instead of throwing.

```cpp
jstm::result<usize> n = uart.write(data);
if (!n) {
  log::error("write failed: %s", n.error().message);
  return;
}
// use *n
```

error codes:

```
ok, timeout, hardware_fault, invalid_argument, not_initialized,
not_found, connection_failed, out_of_memory, io_error
```

helpers:

```cpp
return jstm::ok();            // result<void> success
return jstm::ok(42);          // result<int> success
return jstm::fail(error_code::timeout, "spi timed out");
```

## color

rgb565 color type for 16-bit displays.

```cpp
jstm::color c(255, 0, 128);   // from r, g, b
jstm::color c(0xF800);        // from raw u16

jstm::colors::red;
jstm::colors::cyan;
jstm::colors::dark_gray;
// ... etc
```

conversion helpers: `rgb565(r, g, b)`, `rgb565_r(c)`, `rgb565_g(c)`, `rgb565_b(c)`.

predefined colors: black, white, red, green, blue, cyan, magenta, yellow,
orange, purple, gray, dark_gray, light_gray.

## log

printf-style logging with severity levels. output goes to uart4 by default
(configured in `hal/src/log_backend.cpp`). if no uart backend is linked,
falls back to `std::printf`.

```cpp
jstm::log::info("hello %s", "world");
jstm::log::warn("temperature: %d°C", temp);
jstm::log::error("init failed");
```

levels: `trace`, `debug`, `info`, `warn`, `error`, `off`.

compile-time filtering via `JSTM_LOG_LEVEL` (default 2 = info). messages
below the threshold are compiled out completely.

output format: `[INF] hello world\n`

## concepts

c++20 concepts that define the interfaces drivers must satisfy:

- `display` - anything with `width()`, `height()`, `fill()`, `pixel()`, `blit()`
- `spi_device` - has `select()` / `deselect()`
- `i2c_device` - has `address()`
- `input_source` - has `poll()` / `pressed()`
- `touch_source` - has `touched()` / `read()`

the canvas template is constrained with `display`, and the driver headers
have `static_assert` checks so you get clear errors if a driver doesn't
satisfy its concept.

## time

blocking delay and timestamp utilities:

```cpp
jstm::delay_ms(100);       // HAL_Delay
jstm::delay_us(50);        // DWT cycle counter (sub-microsecond accurate)
u32 t = jstm::millis();    // HAL_GetTick
```

`delay_us` uses the cortex-m7 DWT cycle counter which is enabled during
`system_init()`. it's accurate down to ~5 ns at 216 mhz.
