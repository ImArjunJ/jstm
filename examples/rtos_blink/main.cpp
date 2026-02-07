#include <jstm/core.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/rtos/rtos.hpp>

using namespace jstm;

static void led1_task(void*) {
  hal::output_pin led(GPIOB, GPIO_PIN_0);
  while (true) {
    led.toggle();
    rtos::this_task::delay_ms(1000);
  }
}

static void led2_task(void*) {
  hal::output_pin led(GPIOB, GPIO_PIN_7);
  while (true) {
    led.toggle();
    rtos::this_task::delay_ms(1000);
  }
}

static void led3_task(void*) {
  hal::output_pin led(GPIOB, GPIO_PIN_14);
  while (true) {
    led.toggle();
    rtos::this_task::delay_ms(1000);
  }
}

int main() {
  hal::system_init();

  log::info("rtos blink example started");

  rtos::task t1("led1", led1_task, nullptr, 128, 1);
  rtos::task t2("led2", led2_task, nullptr, 128, 1);
  rtos::task t3("led3", led3_task, nullptr, 128, 1);

  rtos::start_scheduler();

  while (true) {
  }
}
