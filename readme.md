# jstm

modular c++23 toolkit for the stm32 nucleo f746zg. link only what you need.

## target

- **mcu**: stm32f746zgt6 (cortex-m7, 216 mhz, 1 mb flash, 320 kb sram)
- **board**: nucleo-144
- **toolchain**: arm-none-eabi-gcc 14+, cmake >= 3.22
- **standard**: c++23, c11

## modules

| target          | what it is                                        |
| --------------- | ------------------------------------------------- |
| `jstm_core`     | type aliases, `result<T>`, colors, logging        |
| `jstm_hal`      | gpio, uart, spi, i2c, dma, fmc, system init       |
| `jstm_rtos`     | freertos c++ wrappers (task, queue, semaphore)    |
| `jstm_graphics` | `canvas<D>` draw primitives + text on any display |
| `jstm_ili9341`  | ili9341 tft driver over fmc                       |
| `jstm_xpt2046`  | xpt2046 resistive touch over spi                  |
| `jstm_rtcan`    | real-time can bus service with pub/sub            |

## using it

add jstm as a subdirectory and link the modules you want:

```cmake
add_subdirectory(jstm)
target_link_libraries(my_app jstm_hal jstm_rtos)
```

modules pull in their own dependencies, so linking `jstm_rtcan` gets you
`jstm_rtos`, `jstm_core`, and `stm32_hal` automatically.

## building

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=stm32f746zg.cmake -DJSTM_ENABLE_EXAMPLES=ON
cmake --build build
```

## flashing

```
openocd -f board/st_nucleo_f7.cfg -c "program build/examples/blink/example_blink.elf verify reset exit"
```

or via st-link:

```
st-flash write build/examples/blink/example_blink.bin 0x08000000
```

## quick taste

```cpp
#include <jstm/core.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/rtos/rtos.hpp>

using namespace jstm;

static void blink(void*) {
  hal::output_pin led(GPIOB, GPIO_PIN_0);
  while (true) {
    led.toggle();
    rtos::this_task::delay_ms(500);
  }
}

int main() {
  hal::system_init();
  rtos::task t("blink", blink, nullptr, 128, 1);
  rtos::start_scheduler();
}
```

## cmake options

| option                 | default | what it does           |
| ---------------------- | ------- | ---------------------- |
| `JSTM_ENABLE_EXAMPLES` | `OFF`   | build example programs |

## docs

everything is documented in `docs/`:

- [core](docs/core.md) - types, result, color, logging, platform, time, concepts
- [hal](docs/hal.md) - gpio, uart, spi, i2c, dma, fmc, system init
- [rtos](docs/rtos.md) - freertos wrappers
- [graphics](docs/graphics.md) - canvas and font rendering
- [drivers](docs/drivers.md) - ili9341, xpt2046
- [rtcan](docs/rtcan.md) - real-time can bus service
- [examples](docs/examples.md) - walkthrough of each example

## license

do whatever you want with it.
