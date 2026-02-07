# rtcan

real-time can bus service for the stm32f7. handles all the painful
parts, bit timing, interrupt routing, memory pools, and
publish/subscribe dispatch, so you can just send and receive messages.

ported from [sufst/rtcan](https://github.com/sufst/rtcan) and rewritten
in c++23 with freertos primitives.

```cmake
target_link_libraries(my_app jstm_rtcan)
```

## why

the stm32 hal can api is verbose and stateful. you need to manage tx
mailboxes, rx fifos, filter banks, interrupt callbacks, and memory
lifetimes yourself. rtcan wraps all of that into a simple pub/sub model:
publish a message by id, subscribe to messages by id, and the service
handles everything in between.

## config

```cpp
rtcan::config cfg{};
cfg.instance   = CAN1;
cfg.rate       = rtcan::bitrate::k500;
cfg.tx_port    = GPIOA;  cfg.tx_pin = GPIO_PIN_12;
cfg.rx_port    = GPIOA;  cfg.rx_pin = GPIO_PIN_11;
cfg.af         = GPIO_AF9_CAN1;
cfg.loopback   = true;   // for testing without a bus

rtcan::service svc{cfg};
```

| field           | default        | what it does                             |
| --------------- | -------------- | ---------------------------------------- |
| instance        | CAN1           | can peripheral (CAN1 or CAN2)            |
| rate            | k500           | bitrate (k125, k250, k500, k1000)        |
| tx_port/tx_pin  | GPIOA / PIN_12 | can tx gpio                              |
| rx_port/rx_pin  | GPIOA / PIN_11 | can rx gpio                              |
| af              | GPIO_AF9_CAN1  | alternate function                       |
| loopback        | false          | internal loopback mode                   |
| silent          | false          | listen-only mode                         |
| thread_priority | 3              | freertos priority for tx/rx tasks        |
| tx_queue_depth  | 16             | outgoing message buffer size             |
| rx_pool_size    | 64             | number of pre-allocated rx message slots |
| hashmap_size    | 32             | subscriber hashmap buckets               |
| max_subscribers | 64             | total subscriber slots across all ids    |

## bit timing

the driver uses 18 time quanta per bit:

- sync segment: 1 tq
- bit segment 1: 13 tq
- bit segment 2: 4 tq
- sjw: 1 tq
- sample point: 77.8%

prescaler is computed automatically from `HAL_RCC_GetPCLK1Freq()` and
the selected bitrate. on the nucleo f746zg at 216 mhz (pclk1 = 54 mhz,
can clock = 108 mhz after apb1 divider compensation), this gives:

| bitrate | prescaler |
| ------- | --------- |
| 125k    | 48        |
| 250k    | 24        |
| 500k    | 12        |
| 1000k   | 6         |

## basic usage

### transmit

```cpp
rtcan::msg m{};
m.id  = 0x100;
m.dlc = 4;
m.data[0] = 0xDE;
m.data[1] = 0xAD;

auto res = svc.transmit(m);
```

messages go into an internal queue. the tx thread picks them up,
waits for a free mailbox (3 available), and calls `HAL_CAN_AddTxMessage`.

### subscribe

```cpp
rtos::queue<const rtcan::msg*> my_queue{16};
svc.subscribe(0x100, my_queue);

// in your task:
const rtcan::msg* m = nullptr;
if (my_queue.receive(m)) {
  // use m->id, m->data, m->dlc
  svc.msg_consumed(m);  // MUST call this when done
}
```

multiple queues can subscribe to the same id. each subscriber gets a
pointer to the same message in the rx pool. the pool slot is freed
automatically when all subscribers call `msg_consumed()`.

### unsubscribe

```cpp
svc.unsubscribe(0x100, my_queue);
```

### subscriber count

```cpp
u16 n = svc.subscriber_count(0x100);
```

## message lifecycle

1. isr receives a frame -> grabs a slot from the rx pool free list
2. rx thread wakes up -> hashes the can id -> counts subscribers -> sets
   refcount on the slot -> pushes a `const msg*` to each subscriber queue
3. each subscriber calls `msg_consumed()` -> atomically decrements refcount
4. when refcount hits zero -> slot returns to the free list

if a subscriber's queue is full when the rx thread tries to push, that
subscriber's reference is silently dropped (refcount decremented) so the
pool slot isn't leaked.

## hardware filters

by default, the driver configures a single accept-all filter bank. you
can add up to 14 hardware filters before calling `start()`:

```cpp
// standard id filter: accept 0x100 with exact match
svc.add_filter(0x100, 0x7FF);

// extended id filter
svc.add_filter({.id = 0x18DAF110, .mask = 0x1FFFFFFF, .extended = true});

// when done, start applies them
svc.start();
```

`clear_filters()` removes all user filters and reverts to accept-all
on the next `start()`.

the stm32 has 28 filter banks shared between can1 (banks 0-13) and
can2 (banks 14-27). this driver uses one bank per filter.

## isr hookup

the stm32 hal uses global c callbacks for can interrupts. you need to
wire them to your service instance. this is the boilerplate. copy it
into your main.cpp:

```cpp
static rtcan::service* g_rtcan = nullptr;

extern "C" {
void CAN1_TX_IRQHandler()  { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_RX0_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_RX1_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_SCE_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_rx_isr(CAN_RX_FIFO0);
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_rx_isr(CAN_RX_FIFO1);
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_error_isr();
}
}
```

then in `main()`:

```cpp
static rtcan::service svc{cfg};
g_rtcan = &svc;
svc.start();
```

the `start()` method sets all four can interrupts to nvic priority 6
(which is >=5, the freertos threshold) so the isr handlers can safely
call `give_from_isr()`, `send_from_isr()`, and `receive_from_isr()`.

## error handling

errors are sticky bitmask flags:

| flag        | meaning                              |
| ----------- | ------------------------------------ |
| init        | `HAL_CAN_Init` or filter config fail |
| arg         | invalid argument                     |
| memory_full | tx queue, rx pool, or hashmap full   |
| tx_timeout  | no free mailbox within 500 ms        |
| hal         | `HAL_CAN_GetRxMessage` or tx fail    |
| internal    | should never happen                  |

```cpp
if (svc.error() != rtcan_error::none) {
  log::warn("rtcan errors: 0x%08lX", static_cast<u32>(svc.error()));
  svc.clear_error();
}
```

## internals

### threads

the service spawns two freertos tasks:

- tx thread: blocks on the tx queue -> takes a mailbox semaphore ->
  calls `HAL_CAN_AddTxMessage`. the semaphore (counting, max 3) tracks
  the three hardware tx mailboxes. the isr gives it back when a mailbox
  frees up.
- rx thread: blocks on the notify queue -> looks up subscribers by
  hash -> sets refcount -> distributes `const msg*` pointers.

### hashmap

subscribers are stored in an open-addressing hashmap with collision
chaining. the hash function is jenkins one-at-a-time. each slot holds
a linked list of subscriber nodes.

### memory pools

all rx messages live in a pre-allocated flat array (`rx_pool_`). free
slots are tracked with a freertos queue used as a free list. this means
allocations in the isr are just a `receive_from_isr()` (pop from queue)

- no heap, no locks.

subscriber nodes also use a free list for reuse after `unsubscribe()`.
