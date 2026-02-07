# hal

wraps the st hal in raii c++ classes so peripherals get initialized in constructors and cleaned up in
destructors. link `jstm_hal` for everything.

```cmake
target_link_libraries(my_app jstm_hal)
```

`jstm_hal` pulls in `jstm_core`, `stm32_hal` (the vendor hal), and the
startup files (vector table, `SystemInit`, syscalls).

## system init

call this first thing in `main()`. it configures:

- hse bypass (8 mhz from st-link) -> pll -> 216 mhz sysclk
- ahb /1, apb1 /4 (54 mhz), apb2 /2 (108 mhz)
- flash latency 7 (for 216 mhz)
- dwt cycle counter (for `delay_us`)
- gpio clocks A-G
- uart4 logging backend (PA0 tx, PA1 rx, 115200 baud)

```cpp
jstm::hal::system_init();
```

after this call, `log::info()` works and the system clock is stable.

## gpio

### output_pin

```cpp
hal::output_pin led(GPIOB, GPIO_PIN_0);       // default low
hal::output_pin cs(GPIOC, GPIO_PIN_4, true);   // start high

led.high();
led.low();
led.toggle();
led.set(value);
```

configures push-pull, no pull, low speed. non-copyable.

### input_pin

```cpp
hal::input_pin btn(GPIOC, GPIO_PIN_13, hal::pull::up);

if (btn.read()) { ... }
```

pull options: `pull::none`, `pull::up`, `pull::down`.

### cs_guard

raii chip-select. pulls cs low on construction, high on destruction:

```cpp
{
  hal::cs_guard g(cs_pin);
  spi.write(data);
}  // cs goes high here
```

## uart

```cpp
hal::uart_port serial(USART3, {
  .baudrate = 115200,
  .tx_port = GPIOD, .tx_pin = GPIO_PIN_8,
  .rx_port = GPIOD, .rx_pin = GPIO_PIN_9,
  .af = GPIO_AF7_USART3,
});

u8 buf[] = {0x01, 0x02};
auto r = serial.write(std::span{buf});
```

returns `result<usize>` for both `write()` and `read()`. configures 8n1
by default. the gpio alternate function, clock enable, and `HAL_UART_Init`
are all done in the constructor.

## spi

```cpp
hal::spi_bus spi(SPI1, {
  .baudrate_prescaler = SPI_BAUDRATEPRESCALER_16,
  .cpol = SPI_POLARITY_LOW,
  .cpha = SPI_PHASE_1EDGE,
  .sck_port = GPIOA, .sck_pin = GPIO_PIN_5,
  .mosi_port = GPIOA, .mosi_pin = GPIO_PIN_7,
  .miso_port = GPIOA, .miso_pin = GPIO_PIN_6,
  .af = GPIO_AF5_SPI1,
});

spi.write(data); // tx only
spi.transfer(tx_span, rx_span); // full duplex
```

movable, non-copyable. destructor calls `HAL_SPI_DeInit`.

## i2c

```cpp
hal::i2c_bus i2c(I2C1, {
  .timing = 0x40912732,    // 400 kHz for 54 mhz pclk1
  .sda_port = GPIOB, .sda_pin = GPIO_PIN_9,
  .scl_port = GPIOB, .scl_pin = GPIO_PIN_8,
  .af = GPIO_AF4_I2C1,
});

i2c.write(0x3C, data);
i2c.read(0x3C, buf);
i2c.write_read(0x3C, cmd, response);
```

the timing register value depends on the i2c speed and pclk1 frequency.
use st's cubemx or the reference manual calculator to get the right value.

gpio is configured as open-drain with pull-up (required for i2c).

## dma

```cpp
hal::dma_channel dma(DMA2_Stream0, DMA_CHANNEL_0);

dma.start(src, dst, length);
dma.wait(); // blocking poll
dma.busy(); // non-blocking check
```

currently memory-to-peripheral, normal mode, byte-aligned. the constructor
calls `HAL_DMA_Init`. movable, non-copyable.

## fmc lcd

sets up the flexible memory controller for a 16-bit parallel lcd on norsram
bank 1. used by the ili9341 driver.

```cpp
auto addr = hal::fmc_lcd_init();
// addr.cmd  - write commands here
// addr.data - write data here
```

configures fmc with bank1, 16-bit bus, sram mode. the address line A0
distinguishes command vs data: writing to `addr.cmd` drives A0 low (command),
writing to `addr.data` drives A0 high (data).

gpio pins for the data bus and control lines are hardcoded for the nucleo
f746zg layout (ports D, E, F).

## log backend

initializes uart4 on PA0/PA1 at 115200 baud. provides the
`log_uart_transmit()` function that `log.hpp` calls when logging.

this is a weak symbol - if you don't link `jstm_hal`, logging falls back
to `std::printf` (which goes nowhere on bare metal unless you implement
`_write`).

## clock tree

```
hse 8 mhz (bypass from st-link)
  └─ pll: M=8, N=432, P=2, Q=9
       └─ sysclk = 216 mhz
            ├─ ahb  /1  = 216 mhz (hclk)
            ├─ apb1 /4  = 54 mhz  (pclk1 - uart, i2c, can, tim2-7)
            └─ apb2 /2  = 108 mhz (pclk2 - spi1, usart1/6)
```

timer clocks are 2x their apb when the apb prescaler isn't 1, so:

- tim2-7, tim12-14: 108 mhz
- tim1, tim8-11: 216 mhz
