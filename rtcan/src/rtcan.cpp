#include <cstring>
#include <jstm/log.hpp>
#include <jstm/rtcan/rtcan.hpp>

namespace jstm::rtcan {

static constexpr u32 CAN_TQ = 18;
static constexpr u32 CAN_BS1 = CAN_BS1_13TQ;
static constexpr u32 CAN_BS2 = CAN_BS2_4TQ;
static constexpr u32 CAN_SJW = CAN_SJW_1TQ;

static u32 compute_prescaler(bitrate rate) {
  const u32 pclk1 = HAL_RCC_GetPCLK1Freq();
  const u32 apb1_div = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
  const u32 can_clk = (apb1_div >= 4) ? (pclk1 * 2) : pclk1;
  return can_clk / (static_cast<u32>(rate) * CAN_TQ);
}

service::service(const config& cfg) : cfg_{cfg} {
  init_gpio();
  init_peripheral();
  init_pools();
  configure_filters();
}

service::~service() {
  stop();

  delete tx_task_;
  delete rx_task_;
  delete tx_queue_;
  delete tx_mailbox_sem_;
  delete[] rx_pool_;
  delete rx_free_list_;
  delete rx_notify_queue_;
  delete[] map_;
  delete[] subscribers_;
}

void service::init_gpio() {
  auto enable_gpio_clock = [](GPIO_TypeDef* port) {
    if (port == GPIOA)
      __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
      __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOD)
      __HAL_RCC_GPIOD_CLK_ENABLE();
  };

  enable_gpio_clock(cfg_.tx_port);
  enable_gpio_clock(cfg_.rx_port);

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = cfg_.af;

  gpio.Pin = cfg_.tx_pin;
  HAL_GPIO_Init(cfg_.tx_port, &gpio);

  gpio.Pin = cfg_.rx_pin;
  HAL_GPIO_Init(cfg_.rx_port, &gpio);
}

void service::init_peripheral() {
  if (cfg_.instance == CAN1) {
    __HAL_RCC_CAN1_CLK_ENABLE();
  } else if (cfg_.instance == CAN2) {
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();
  }

  hcan_.Instance = cfg_.instance;
  hcan_.Init.Prescaler = compute_prescaler(cfg_.rate);
  hcan_.Init.Mode = CAN_MODE_NORMAL;

  if (cfg_.loopback && cfg_.silent)
    hcan_.Init.Mode = CAN_MODE_SILENT_LOOPBACK;
  else if (cfg_.loopback)
    hcan_.Init.Mode = CAN_MODE_LOOPBACK;
  else if (cfg_.silent)
    hcan_.Init.Mode = CAN_MODE_SILENT;

  hcan_.Init.SyncJumpWidth = CAN_SJW;
  hcan_.Init.TimeSeg1 = CAN_BS1;
  hcan_.Init.TimeSeg2 = CAN_BS2;
  hcan_.Init.TimeTriggeredMode = DISABLE;
  hcan_.Init.AutoBusOff = ENABLE;
  hcan_.Init.AutoWakeUp = DISABLE;
  hcan_.Init.AutoRetransmission = ENABLE;
  hcan_.Init.ReceiveFifoLocked = DISABLE;
  hcan_.Init.TransmitFifoPriority = DISABLE;

  if (HAL_CAN_Init(&hcan_) != HAL_OK) {
    err_ |= rtcan_error::init;
    log::error("rtcan: HAL_CAN_Init failed");
  }
}

void service::init_pools() {
  tx_queue_ = new rtos::queue<msg>(cfg_.tx_queue_depth);
  tx_mailbox_sem_ = new rtos::counting_semaphore(3, 3);

  rx_pool_ = new internal_msg[cfg_.rx_pool_size]{};

  rx_free_list_ = new rtos::queue<u16>(cfg_.rx_pool_size);
  for (u16 i = 0; i < cfg_.rx_pool_size; ++i) {
    rx_free_list_->send(i, 0);
  }

  rx_notify_queue_ = new rtos::queue<u16>(cfg_.rx_pool_size);

  map_ = new hashmap_slot[cfg_.hashmap_size]{};
  subscribers_ = new subscriber_node[cfg_.max_subscribers]{};
  next_free_subscriber_ = 0;
  subscriber_free_head_ = INVALID_INDEX;
}

void service::configure_filters() {
  CAN_FilterTypeDef filter{};
  filter.FilterActivation = ENABLE;
  filter.FilterBank = (cfg_.instance == CAN2) ? 14 : 0;
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.SlaveStartFilterBank = 14;

  if (HAL_CAN_ConfigFilter(&hcan_, &filter) != HAL_OK) {
    err_ |= rtcan_error::init;
    log::error("rtcan: HAL_CAN_ConfigFilter failed");
  }
}

result<void> service::add_filter(const filter& f) {
  if (num_user_filters_ >= MAX_USER_FILTERS) {
    return fail(error_code::out_of_memory, "rtcan: filter banks exhausted");
  }
  user_filters_[num_user_filters_++] = f;
  return ok();
}

result<void> service::add_filter(u32 std_id, u32 mask) {
  return add_filter(filter{.id = std_id, .mask = mask});
}

void service::clear_filters() { num_user_filters_ = 0; }

void service::apply_user_filters() {
  const u8 bank_offset = (cfg_.instance == CAN2) ? 14 : 0;

  for (u8 i = 0; i < num_user_filters_; ++i) {
    const filter& f = user_filters_[i];

    CAN_FilterTypeDef cf{};
    cf.FilterActivation = ENABLE;
    cf.FilterBank = bank_offset + i;
    cf.FilterFIFOAssignment =
        (f.fifo == 1) ? CAN_FILTER_FIFO1 : CAN_FILTER_FIFO0;
    cf.FilterMode = CAN_FILTERMODE_IDMASK;
    cf.FilterScale = CAN_FILTERSCALE_32BIT;
    cf.SlaveStartFilterBank = 14;

    if (f.extended) {
      cf.FilterIdHigh = static_cast<u16>((f.id << 3) >> 16);
      cf.FilterIdLow = static_cast<u16>((f.id << 3) & 0xFFFF) | (1 << 2);
      cf.FilterMaskIdHigh = static_cast<u16>((f.mask << 3) >> 16);
      cf.FilterMaskIdLow = static_cast<u16>((f.mask << 3) & 0xFFFF) | (1 << 2);
    } else {
      cf.FilterIdHigh = static_cast<u16>(f.id << 5);
      cf.FilterIdLow = 0x0000;
      cf.FilterMaskIdHigh = static_cast<u16>(f.mask << 5);
      cf.FilterMaskIdLow = 0x0000;
    }

    if (HAL_CAN_ConfigFilter(&hcan_, &cf) != HAL_OK) {
      err_ |= rtcan_error::init;
      log::error("rtcan: HAL_CAN_ConfigFilter failed for bank %d",
                 bank_offset + i);
    }
  }
}

result<void> service::start() {
  if (err_ != rtcan_error::none) {
    return fail(error_code::not_initialized, "rtcan: init errors present");
  }

  if (num_user_filters_ > 0) {
    apply_user_filters();
  }

  IRQn_Type tx_irq, rx0_irq, rx1_irq, sce_irq;
  if (cfg_.instance == CAN1) {
    tx_irq = CAN1_TX_IRQn;
    rx0_irq = CAN1_RX0_IRQn;
    rx1_irq = CAN1_RX1_IRQn;
    sce_irq = CAN1_SCE_IRQn;
  } else {
    tx_irq = CAN2_TX_IRQn;
    rx0_irq = CAN2_RX0_IRQn;
    rx1_irq = CAN2_RX1_IRQn;
    sce_irq = CAN2_SCE_IRQn;
  }

  constexpr u32 can_nvic_priority = 6;
  HAL_NVIC_SetPriority(tx_irq, can_nvic_priority, 0);
  HAL_NVIC_SetPriority(rx0_irq, can_nvic_priority, 0);
  HAL_NVIC_SetPriority(rx1_irq, can_nvic_priority, 0);
  HAL_NVIC_SetPriority(sce_irq, can_nvic_priority, 0);
  HAL_NVIC_EnableIRQ(tx_irq);
  HAL_NVIC_EnableIRQ(rx0_irq);
  HAL_NVIC_EnableIRQ(rx1_irq);
  HAL_NVIC_EnableIRQ(sce_irq);

  const u32 notifs = CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING |
                     CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_ERROR |
                     CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE |
                     CAN_IT_ERROR_WARNING;

  if (HAL_CAN_ActivateNotification(&hcan_, notifs) != HAL_OK) {
    return fail(error_code::hardware_fault,
                "rtcan: failed to activate CAN notifications");
  }

  if (HAL_CAN_Start(&hcan_) != HAL_OK) {
    return fail(error_code::hardware_fault, "rtcan: HAL_CAN_Start failed");
  }

  running_.store(true);

  tx_task_ = new rtos::task("rtcan_tx", tx_thread_entry, this, 512,
                            cfg_.thread_priority);
  rx_task_ = new rtos::task("rtcan_rx", rx_thread_entry, this, 512,
                            cfg_.thread_priority);

  log::info("rtcan: started on CAN%d @ %lu bps",
            (cfg_.instance == CAN1) ? 1 : 2, static_cast<u32>(cfg_.rate));
  return ok();
}

result<void> service::stop() {
  running_.store(false);

  HAL_CAN_Stop(&hcan_);
  HAL_CAN_DeactivateNotification(
      &hcan_, CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING |
                  CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF |
                  CAN_IT_ERROR_PASSIVE | CAN_IT_ERROR_WARNING);

  delete tx_task_;
  tx_task_ = nullptr;
  delete rx_task_;
  rx_task_ = nullptr;

  return ok();
}

result<void> service::transmit(const msg& m) {
  if (!tx_queue_->send(m, 0)) {
    err_ |= rtcan_error::memory_full;
    return fail(error_code::out_of_memory, "rtcan: tx queue full");
  }
  return ok();
}

u32 service::hash(u32 key) const {
  u32 h = key;
  h += (h << 12);
  h ^= (h >> 22);
  h += (h << 4);
  h ^= (h >> 9);
  h += (h << 10);
  h ^= (h >> 2);
  h += (h << 7);
  h ^= (h >> 12);
  return h;
}

u16 service::alloc_subscriber() {
  if (subscriber_free_head_ != INVALID_INDEX) {
    u16 idx = subscriber_free_head_;
    subscriber_free_head_ = subscribers_[idx].next;
    subscribers_[idx].q = nullptr;
    subscribers_[idx].next = INVALID_INDEX;
    return idx;
  }
  if (next_free_subscriber_ >= cfg_.max_subscribers) return INVALID_INDEX;
  return next_free_subscriber_++;
}

void service::free_subscriber(u16 idx) {
  subscribers_[idx].q = nullptr;
  subscribers_[idx].next = subscriber_free_head_;
  subscriber_free_head_ = idx;
}

service::hashmap_slot* service::find_or_create_slot(u32 can_id) {
  const u32 index = hash(can_id) % cfg_.hashmap_size;
  hashmap_slot* slot = &map_[index];

  if (!slot->occupied) {
    slot->can_id = can_id;
    slot->occupied = true;
    return slot;
  }

  hashmap_slot* cur = slot;
  while (true) {
    if (cur->can_id == can_id) return cur;
    if (cur->chain_next == INVALID_INDEX) break;
    cur = &map_[cur->chain_next];
  }

  for (u16 i = 0; i < cfg_.hashmap_size; ++i) {
    if (!map_[i].occupied) {
      map_[i].can_id = can_id;
      map_[i].occupied = true;
      cur->chain_next = i;
      return &map_[i];
    }
  }

  return nullptr;
}

result<void> service::subscribe(u32 can_id, rtos::queue<const msg*>& q) {
  hashmap_slot* slot = find_or_create_slot(can_id);
  if (!slot) {
    err_ |= rtcan_error::memory_full;
    return fail(error_code::out_of_memory, "rtcan: subscriber hashmap full");
  }

  u16 si = alloc_subscriber();
  if (si == INVALID_INDEX) {
    err_ |= rtcan_error::memory_full;
    return fail(error_code::out_of_memory, "rtcan: subscriber pool full");
  }

  subscribers_[si].q = &q;
  subscribers_[si].next = INVALID_INDEX;

  if (slot->first_subscriber == INVALID_INDEX) {
    slot->first_subscriber = si;
  } else {
    u16 cur = slot->first_subscriber;
    while (subscribers_[cur].next != INVALID_INDEX) {
      cur = subscribers_[cur].next;
    }
    subscribers_[cur].next = si;
  }

  return ok();
}

const service::hashmap_slot* service::find_slot(u32 can_id) const {
  const u32 index = hash(can_id) % cfg_.hashmap_size;
  const hashmap_slot* slot = &map_[index];

  if (!slot->occupied) return nullptr;

  const hashmap_slot* cur = slot;
  while (cur) {
    if (cur->can_id == can_id) return cur;
    if (cur->chain_next == INVALID_INDEX) break;
    cur = &map_[cur->chain_next];
  }
  return nullptr;
}

u16 service::subscriber_count(u32 can_id) const {
  const hashmap_slot* slot = find_slot(can_id);
  if (!slot) return 0;

  u16 count = 0;
  u16 si = slot->first_subscriber;
  while (si != INVALID_INDEX) {
    ++count;
    si = subscribers_[si].next;
  }
  return count;
}

result<void> service::unsubscribe(u32 can_id, rtos::queue<const msg*>& q) {
  const u32 index = hash(can_id) % cfg_.hashmap_size;
  hashmap_slot* slot = &map_[index];

  if (!slot->occupied)
    return fail(error_code::not_found, "rtcan: no subscribers for this CAN ID");

  hashmap_slot* found = nullptr;
  hashmap_slot* cur = slot;
  while (cur) {
    if (cur->can_id == can_id) {
      found = cur;
      break;
    }
    if (cur->chain_next == INVALID_INDEX) break;
    cur = &map_[cur->chain_next];
  }

  if (!found)
    return fail(error_code::not_found, "rtcan: no subscribers for this CAN ID");

  u16 prev = INVALID_INDEX;
  u16 si = found->first_subscriber;
  while (si != INVALID_INDEX) {
    if (subscribers_[si].q == &q) {
      if (prev == INVALID_INDEX) {
        found->first_subscriber = subscribers_[si].next;
      } else {
        subscribers_[prev].next = subscribers_[si].next;
      }
      free_subscriber(si);
      return ok();
    }
    prev = si;
    si = subscribers_[si].next;
  }

  return fail(error_code::not_found, "rtcan: queue not subscribed to this ID");
}

void service::msg_consumed(const msg* m) {
  auto* im = reinterpret_cast<internal_msg*>(reinterpret_cast<uintptr_t>(m) -
                                             offsetof(internal_msg, payload));

  u16 prev = im->refcount.fetch_sub(1, std::memory_order_acq_rel);
  if (prev == 1) {
    u16 slot_index = static_cast<u16>(im - rx_pool_);
    rx_free_list_->send(slot_index, 0);
  }
}

void service::handle_tx_complete_isr() { tx_mailbox_sem_->give_from_isr(); }

void service::handle_rx_isr(u32 fifo) {
  u16 slot_index;
  if (!rx_free_list_->receive_from_isr(slot_index)) {
    CAN_RxHeaderTypeDef hdr;
    u8 discard[8];
    HAL_CAN_GetRxMessage(&hcan_, fifo, &hdr, discard);
    err_ |= rtcan_error::memory_full;
    return;
  }

  internal_msg& im = rx_pool_[slot_index];

  CAN_RxHeaderTypeDef hdr;
  if (HAL_CAN_GetRxMessage(&hcan_, fifo, &hdr, im.payload.data) != HAL_OK) {
    rx_free_list_->send_from_isr(slot_index);
    err_ |= rtcan_error::hal;
    return;
  }

  const u8 dlc = static_cast<u8>(hdr.DLC);
  for (u8 i = dlc; i < 8; ++i) im.payload.data[i] = 0;

  if (hdr.IDE == CAN_ID_EXT) {
    im.payload.id = hdr.ExtId;
    im.payload.extended = true;
  } else {
    im.payload.id = hdr.StdId;
    im.payload.extended = false;
  }
  im.payload.dlc = dlc;
  im.payload.rtr = (hdr.RTR == CAN_RTR_REMOTE);
  im.refcount.store(0, std::memory_order_relaxed);

  rx_notify_queue_->send_from_isr(slot_index);
}

void service::handle_error_isr() {
  tx_mailbox_sem_->give_from_isr();
  HAL_CAN_ResetError(&hcan_);
  err_ |= rtcan_error::hal;
}

void service::tx_thread_entry(void* arg) {
  auto* self = static_cast<service*>(arg);

  while (self->running_.load()) {
    msg m;
    if (!self->tx_queue_->receive(m, pdMS_TO_TICKS(100))) {
      continue;
    }

    if (!self->tx_mailbox_sem_->take(pdMS_TO_TICKS(500))) {
      self->err_ |= rtcan_error::tx_timeout;
      continue;
    }

    CAN_TxHeaderTypeDef hdr{};
    if (m.extended) {
      hdr.IDE = CAN_ID_EXT;
      hdr.ExtId = m.id;
    } else {
      hdr.IDE = CAN_ID_STD;
      hdr.StdId = m.id;
    }
    hdr.RTR = m.rtr ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    hdr.DLC = m.dlc;

    u32 mailbox;
    if (HAL_CAN_AddTxMessage(&self->hcan_, &hdr, m.data, &mailbox) != HAL_OK) {
      self->err_ |= rtcan_error::hal;
      self->tx_mailbox_sem_->give();
    }
  }
}

void service::rx_thread_entry(void* arg) {
  auto* self = static_cast<service*>(arg);

  while (self->running_.load()) {
    u16 slot_index;
    if (!self->rx_notify_queue_->receive(slot_index, pdMS_TO_TICKS(100))) {
      continue;
    }

    internal_msg& im = self->rx_pool_[slot_index];

    const u32 index = self->hash(im.payload.id) % self->cfg_.hashmap_size;
    hashmap_slot* slot = &self->map_[index];

    if (!slot->occupied) {
      self->rx_free_list_->send(slot_index, 0);
      continue;
    }

    hashmap_slot* found = nullptr;
    hashmap_slot* cur = slot;
    while (cur) {
      if (cur->can_id == im.payload.id) {
        found = cur;
        break;
      }
      if (cur->chain_next == INVALID_INDEX) break;
      cur = &self->map_[cur->chain_next];
    }

    if (!found || found->first_subscriber == INVALID_INDEX) {
      self->rx_free_list_->send(slot_index, 0);
      continue;
    }

    u16 count = 0;
    u16 si = found->first_subscriber;
    while (si != INVALID_INDEX) {
      ++count;
      si = self->subscribers_[si].next;
    }

    im.refcount.store(count, std::memory_order_relaxed);

    const msg* payload_ptr = &im.payload;
    si = found->first_subscriber;
    while (si != INVALID_INDEX) {
      if (!self->subscribers_[si].q->send(payload_ptr, 0)) {
        u16 prev = im.refcount.fetch_sub(1, std::memory_order_acq_rel);
        if (prev == 1) {
          self->rx_free_list_->send(slot_index, 0);
        }
      }
      si = self->subscribers_[si].next;
    }

    if (im.refcount.load(std::memory_order_acquire) == 0 && count > 0) {
    }
  }
}

}  // namespace jstm::rtcan
