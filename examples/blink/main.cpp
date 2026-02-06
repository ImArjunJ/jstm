#include <jstm/core.hpp>
#include <jstm/hal/hal.hpp>

int main() {
  jstm::hal::system_init();

  jstm::hal::output_pin led1(GPIOB, GPIO_PIN_0);
  jstm::hal::output_pin led2(GPIOB, GPIO_PIN_7);
  jstm::hal::output_pin led3(GPIOB, GPIO_PIN_14);

  jstm::log::info("blink example started");

  while (true) {
    led1.toggle();
    jstm::delay_ms(500);

    led2.toggle();
    jstm::delay_ms(500);

    led3.toggle();
    jstm::delay_ms(500);
  }
}
