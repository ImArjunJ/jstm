# rtos

c++ wrappers around freertos v11.1.0. link `jstm_rtos` and you get the full
kernel plus typed c++ primitives.

```cmake
target_link_libraries(my_app jstm_rtos)
```

## why

the raw freertos api uses void pointers, untyped handles, and c naming
conventions. these wrappers give you:

- raii lifetime (tasks/queues/semaphores are deleted in destructors)
- typed queues (`queue<msg>` instead of `QueueHandle_t` + `sizeof`)
- move semantics where it makes sense
- `this_task` namespace for delay/yield/suspend

there is little to no overhead.

## configuration

freertos is configured in `rtos/config/FreeRTOSConfig.h`:

| setting              | value     | why                                  |
| -------------------- | --------- | ------------------------------------ |
| tick rate            | 1000 hz   | 1 ms resolution                      |
| heap size            | 32 kb     | heap_4 allocator                     |
| max priorities       | 8         | 0 (idle) through 7                   |
| minimal stack        | 128 words | 512 bytes per task minimum           |
| static allocation    | on        | idle + timer tasks use static memory |
| max syscall priority | 5         | isr priority ceiling for FromISR     |

the hal tick is driven by tim6 (not systick) so that freertos owns systick
exclusively. see `rtos/src/hal_timebase_tim.c`.

## task

```cpp
rtos::task t("blink", my_func, nullptr, 256, 2);

t.suspend();
t.resume();
t.priority(); // read
t.set_priority(3); // write
t.stack_high_water_mark(); // debug: minimum free stack ever
```

the function signature is `void (*)(void*)`. stack depth is in words
(multiply by 4 for bytes). priority 0 is idle, higher is more urgent.

tasks are deleted when the `task` object is destroyed.

## mutex / lock_guard

```cpp
rtos::mutex m;
{
  rtos::lock_guard lock(m);
  // critical section
}
```

standard mutex with priority inheritance (freertos default). `lock()` takes
an optional timeout in ticks (default: wait forever).

## binary_semaphore

```cpp
rtos::binary_semaphore sem;

// in isr:
sem.give_from_isr();

// in task:
sem.take();
```

isr-safe signaling. `give_from_isr()` handles the `portYIELD_FROM_ISR`
call internally.

## counting_semaphore

```cpp
rtos::counting_semaphore sem(3, 3);  // max 3, initial 3

sem.take();   // decrement
sem.give();   // increment
sem.count();  // current value
```

used by rtcan to track the 3 hardware can mailboxes.

## queue<T>

```cpp
rtos::queue<msg> q(16);  // 16-element queue

q.send(m);                           // block until space
q.send(m, pdMS_TO_TICKS(100));       // timeout
q.send(m, 0);                        // non-blocking

msg out;
q.receive(out);                      // block until available
q.receive(out, pdMS_TO_TICKS(50));   // timeout

q.send_from_isr(m);     // isr-safe send
q.receive_from_isr(out); // isr-safe receive

q.peek(out);             // read without removing
q.count();               // messages waiting
q.spaces();              // free slots
q.reset();               // discard all messages
```

the template parameter determines the element type. items are copied by
value (memcpy internally), so keep them small or use pointers.

## software_timer

```cpp
rtos::software_timer heartbeat("hb", pdMS_TO_TICKS(1000), true, [](TimerHandle_t) {
  led.toggle();
});

heartbeat.start();
heartbeat.stop();
heartbeat.set_period(pdMS_TO_TICKS(500));
heartbeat.is_active();
```

callbacks run in the timer daemon task (priority 2, stack 256 words).
keep them short.

## this_task

convenience namespace for the calling task:

```cpp
rtos::this_task::delay_ms(500);
rtos::this_task::yield();
rtos::this_task::suspend();

TaskHandle_t h = rtos::this_task::handle();
```

## free functions

```cpp
rtos::start_scheduler(); // never returns
rtos::tick_count(); // current tick
rtos::delay(ticks); // raw tick delay
rtos::delay_ms(ms); // millisecond delay
```

## hooks

`rtos/src/rtos_hooks.cpp` provides the mandatory freertos hooks:

- `vApplicationMallocFailedHook` - infinite loop (fatal)
- `vApplicationStackOverflowHook` - infinite loop (fatal)
- `vApplicationGetIdleTaskMemory` - static allocation
- `vApplicationGetTimerTaskMemory` - static allocation
- `SysTick_Handler` - routes to freertos tick handler

these are force-linked with `--undefined` so the linker doesn't discard
them from the static library.

## nvic priority rules

any isr that calls a freertos `FromISR` function must have a numerical
priority >= `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (5). lower
numbers = higher priority on arm, so 5 is the ceiling. the rtcan module
uses priority 6 for can interrupts.
