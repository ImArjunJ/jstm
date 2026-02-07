# examples

seven examples that build when you pass `-DJSTM_ENABLE_EXAMPLES=ON`.
each one is a standalone firmware image you can flash to the nucleo
f746zg.

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=stm32f746zg.cmake -DJSTM_ENABLE_EXAMPLES=ON
cmake --build build
```

| example           | what it shows                          |
| ----------------- | -------------------------------------- |
| blink             | bare-metal gpio                        |
| rtos_blink        | freertos tasks                         |
| touch_paint       | fmc display + spi touch + graphics     |
| rtcan_loopback    | can bus pub/sub in internal loopback   |
| can_send_test     | can2 hardware tx through a transceiver |
| can_recv_test     | can2 hardware rx with subscribe_all    |
| can_parallel_test | 5 concurrent tx tasks over can2        |

flash any of them with:

```
openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
  -c "program build/examples/<name>/<name>.elf verify reset exit"
```

---

## blink

no rtos, no drivers, just core + hal.

toggles the three onboard leds (PB0 green, PB7 blue, PB14 red) in
sequence with 500 ms delays using the dwt cycle counter.

```cpp
hal::output_pin led1(GPIOB, GPIO_PIN_0);
hal::output_pin led2(GPIOB, GPIO_PIN_7);
hal::output_pin led3(GPIOB, GPIO_PIN_14);

while (true) {
  led1.toggle();  delay_ms(500);
  led2.toggle();  delay_ms(500);
  led3.toggle();  delay_ms(500);
}
```

---

## rtos_blink

same three leds, but each one runs in its own freertos task at priority
1 with a 128-word stack.

```cpp
rtos::task t1("led1", led1_task, nullptr, 128, 1);
rtos::task t2("led2", led2_task, nullptr, 128, 1);
rtos::task t3("led3", led3_task, nullptr, 128, 1);

rtos::start_scheduler();
```

---

## touch_paint

uses the ili9341 display over fmc and the xpt2046 touch controller over spi.

the display and touch are both set to the same rotation so coordinates
match. touching the palette bar at the bottom selects a brush color.
touching the rightmost bar section clears the screen.

```cpp
drivers::ili9341 display(lcd_cfg);
display.init();
display.set_rotation(3);

graphics::canvas<drivers::ili9341> canvas(display);

drivers::xpt2046 touch(touch_spi, touch_cs, &touch_irq);
touch.init();
touch.set_screen_size(display.width(), display.height());
touch.set_rotation(3);
```

### wiring

| signal       | pin            |
| ------------ | -------------- |
| LCD RST      | PB11           |
| LCD BL       | PF6            |
| FMC data/ctl | (see hal docs) |
| Touch SCK    | PA5            |
| Touch MOSI   | PA7            |
| Touch MISO   | PA6            |
| Touch CS     | PC4            |
| Touch IRQ    | PC5            |

---

## rtcan_loopback

a can bus example using internal loopback mode (no transceiver needed).
a producer task transmits a counter message on id 0x100 every 500 ms.
a subscriber task receives it and logs the data. a heartbeat task
blinks the green led.

```cpp
rtcan::config cfg{};
cfg.loopback = true;
cfg.rate     = rtcan::bitrate::k500;

static rtcan::service svc{cfg};
g_rtcan = &svc;

static rtos::queue<const rtcan::msg*> rx_queue{16};
svc.subscribe(0x100, rx_queue);
svc.start();
```

the example includes all the isr boilerplate needed to wire the stm32
hal can callbacks to the rtcan service instance.

### isr boilerplate

the example shows the required interrupt wiring pattern. the key points:

- store a global pointer to your service instance
- forward all four can irq handlers through `HAL_CAN_IRQHandler`
- route the six hal callbacks to the service's `handle_*_isr()` methods

see [rtcan docs](rtcan.md) for a full explanation of why this is needed.

---

## can_send_test

sends a counter message on can id 0x100 every 500 ms over real hardware.
uses can2 on pb6 (tx) / pb5 (rx) with an external transceiver
(max3051esa+ or similar). requires a second can node on the bus to
provide ack (e.g. a candapter).

```cpp
rtcan::config cfg{};
cfg.instance = CAN2;
cfg.rate     = rtcan::bitrate::k500;
cfg.tx_port  = GPIOB;  cfg.tx_pin = GPIO_PIN_6;
cfg.rx_port  = GPIOB;  cfg.rx_pin = GPIO_PIN_5;
cfg.af       = GPIO_AF9_CAN2;
```

### wiring

| signal | pin |
| ------ | --- |
| CAN TX | PB6 |
| CAN RX | PB5 |

connect pb6/pb5 to the transceiver's txd/rxd. canh/canl go to the bus.
120 ohm termination is preferred at each end of the bus.

---

## can_recv_test

listens for all incoming can frames using `subscribe_all()` and prints
each one to uart. uses the same can2 hardware setup as can_send_test.

```cpp
static rtos::queue<const rtcan::msg*> rx_q{32};
svc.subscribe_all(rx_q);
svc.start();
```

---

## can_parallel_test

stress test: 5 concurrent freertos tasks each transmit 200 messages
(1000 total) at staggered intervals. a monitor task prints progress
and a final summary. uses can2 with `tx_queue_depth = 32` to handle
the burst.
