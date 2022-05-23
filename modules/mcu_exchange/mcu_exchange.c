/*
 * The module architecture (using states, message queue in a separate thread, using events for
 * communication) is based on the Application Event Manager as used in the Asset Tracker v2
 * Application which has the below license.
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/others/app_event_manager.html#app-event-manager
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/asset_tracker_v2/README.html
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE mcu_exchange_module

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

#include <caf/events/module_state_event.h>
#include <ctype.h>
#include <drivers/uart.h>
#include <event_manager.h>
#include <init.h>
#include <logging/log.h>
#include <string.h>
#include <zephyr.h>

#include "common/memory_hook.h"
#include "common/modules_common.h"
#include "mcu_exchange_module_event.h"
#include "utils.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_MCU_EXCHANGE_MODULE_LOG_LEVEL);

struct mcu_exchange_msg_data {
  union {
    struct module_state_event module_state;
    struct mcu_exchange_module_event mcu_exchange;
  } module;
  struct event_dyndata* dyndata;
};

/* Sensor module message queue. */
#define MCU_EXCHANGE_QUEUE_ENTRY_COUNT 10
#define MCU_EXCHANGE_QUEUE_BYTE_ALIGNMENT 4

K_MSGQ_DEFINE(msgq_mcu_exchange, sizeof(struct mcu_exchange_msg_data),
              MCU_EXCHANGE_QUEUE_ENTRY_COUNT, MCU_EXCHANGE_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
    .name = "data bridge module",
    .msg_q = &msgq_mcu_exchange,
};

static bool event_handler(const struct event_header* eh) {
  struct mcu_exchange_msg_data msg = {0};
  bool enqueue_msg = false;

  TRANSLATE_EVENT_TO_MSG(mcu_exchange_module, mcu_exchange)

  if (is_mcu_exchange_module_event(eh)) {
    struct mcu_exchange_module_event* event = cast_mcu_exchange_module_event(eh);
    msg.dyndata = my_malloc(sizeof(struct event_dyndata) + event->dyndata.size,
                            HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA);
    memcpy(msg.dyndata, &event->dyndata, sizeof(struct event_dyndata) + event->dyndata.size);
  }

  if (is_module_state_event(eh)) {
    struct module_state_event* event = cast_module_state_event(eh);
    msg.module.module_state = *event;
    enqueue_msg = true;
  }

  if (enqueue_msg) {
    int err = module_enqueue_msg(&self, &msg);
    if (err) {
      LOG_ERR("Message could not be enqueued");
    }
  }

  return false;
}

static const struct device* uart_dev;
static char uart_buf[CONFIG_MCU_EXCHANGE_UART_BUF_SIZE];

static void send_message(struct mcu_exchange_msg_data* msg) {
  if (msg->module.mcu_exchange.received) return;

  const char* jsonString =
      encode_message(msg->module.mcu_exchange.type, msg->dyndata->data, msg->dyndata->size);

  if (jsonString) {
    for (size_t i = 0; i <= strlen(jsonString); i++) {
      uart_poll_out(uart_dev, jsonString[i]);
    }
    my_free((char*)jsonString);
  }
}

static void handle_message(const char* message) {
  uint16_t len = 0;
  int type = 0;

  uint8_t* decoded_data;
  parse_message(message, &type, &decoded_data, &len);
  struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(len);

  switch (type) {
    case MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY:
      event->type = MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY;
      memcpy(event->dyndata.data, decoded_data, len);
      event->dyndata.size = len;
      break;
    case MCU_EXCHANGE_EVT_BUTTON:
    case MCU_EXCHANGE_EVT_DATA_READY:
      event->type = type;
      break;
    case MCU_EXCHANGE_EVT_ERROR:
    default:
      event->type = MCU_EXCHANGE_EVT_ERROR;
      break;
  }
  event->received = true;
  EVENT_SUBMIT(event);
  my_free(decoded_data);
}

static int uart_rx_handler(uint8_t character) {
  static size_t buf_pos;

  uart_buf[buf_pos] = character;

  switch (character) {
    case '\0':
      goto process;
      break;
    default:
      buf_pos++;
      break;
  }

  if (buf_pos > sizeof(uart_buf) - 1) {
    LOG_HEXDUMP_ERR(uart_buf, buf_pos, "RX");
    LOG_ERR("Buffer overflow. Discard all data.");
#ifdef CONFIG_MEMFAULT
    MEMFAULT_TRACE_EVENT(McuExchange_BufferOverflow);
#endif
    buf_pos = 0;
  }

  return 0;

process:

  if (buf_pos) {
    LOG_INF("buf_pos.%d", buf_pos);

    struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(buf_pos + 1);
    event->type = MCU_EXCHANGE_EVT_DATA_READY;
    event->dyndata.size = buf_pos + 1;
    memcpy(event->dyndata.data, uart_buf, buf_pos + 1);
    EVENT_SUBMIT(event);
  }

  /* Reset UART handler state */
  buf_pos = 0;

  return 0;
}

static void isr(const struct device* dev, void* user_data) {
  ARG_UNUSED(user_data);

  uint8_t character;

  uart_irq_update(dev);

  if (!uart_irq_rx_ready(dev)) {
    return;
  }

  /*
   * Check that we are not sending data (buffer must be preserved then),
   * and that a new character is available before handling each character
   */

  while (uart_fifo_read(dev, &character, 1)) {
    uart_rx_handler(character);
  }
}

static int uart_line_init(char* uart_dev_name) {
  int err;
  uint8_t dummy;
  uart_dev = device_get_binding(uart_dev_name);
  if (uart_dev == NULL) {
    LOG_ERR("Cannot bind %s\n", uart_dev_name);
#ifdef CONFIG_MEMFAULT
    MEMFAULT_TRACE_EVENT(McuExchange_UartDevNotFound);
#endif
    return -EINVAL;
  }

  uint32_t start_time = k_uptime_get_32();

  /* Wait for the UART line to become valid */
  do {
    err = uart_err_check(uart_dev);
    if (err) {
      if (k_uptime_get_32() - start_time > CONFIG_MCU_EXCHANGE_UART_INIT_TIMEOUT) {
        LOG_ERR(
            "UART check failed: %d. "
            "UART initialization timed out.",
            err);
#ifdef CONFIG_MEMFAULT
        MEMFAULT_TRACE_EVENT_WITH_STATUS(McuExchange_UartInitTimeoutError, err);
#endif
        return -EIO;
      }

      LOG_WRN(
          "UART check failed: %d. "
          "Dropping buffer and retrying.",
          err);
#ifdef CONFIG_MEMFAULT
      MEMFAULT_TRACE_EVENT_WITH_STATUS(McuExchange_AtUartInitError, err);
#endif
      while (uart_fifo_read(uart_dev, &dummy, 1)) {
        /* Do nothing with the data */
      }
      k_sleep(K_MSEC(10));
    }
  } while (err);

  uart_irq_callback_set(uart_dev, isr);
  return err;
}

static int init(void) {
  char* uart_dev_name = CONFIG_MCU_EXCHANGE_UART;  // UART_2
  int err;

  /* Initialize the UART module */
  err = uart_line_init(uart_dev_name);
  if (err) {
    LOG_ERR("UART could not be initialized: %d", err);
#ifdef CONFIG_MEMFAULT
    MEMFAULT_TRACE_EVENT_WITH_STATUS(McuExchange_SetupInitCannotInit, err);
#endif
    return -EFAULT;
  }

  uart_irq_rx_enable(uart_dev);

  return err;
}

static void handle_msg(struct mcu_exchange_msg_data* msg) {
  if (msg->module.module_state.module_id == MODULE_ID(main) &&
      msg->module.module_state.state == MODULE_STATE_READY) {
    if (init()) {
      struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(0);
      event->type = MCU_EXCHANGE_EVT_ERROR;
      event->received = false;
      EVENT_SUBMIT(event);
    } else {
      struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(0);
      event->type = MCU_EXCHANGE_EVT_READY;
      event->received = false;
      EVENT_SUBMIT(event);
    }
  }

  else if ((IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY)) ||
           (IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_BUTTON)) ||
           (IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_ERROR))) {
    send_message(msg);
  }

  else if (IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_DATA_READY)) {
    handle_message(msg->dyndata->data);
  } else if (IS_EVENT(msg, mcu_exchange, MCU_EXCHANGE_EVT_ERROR)) {
    LOG_ERR("MCU_EXCHANGE_EVT_ERROR");
  }

  if (msg->dyndata) {
    my_free(msg->dyndata);
  }
}

static void module_thread_fn(void) {
  struct mcu_exchange_msg_data msg;

  self.thread_id = k_current_get();

  module_start(&self);

  while (true) {
    module_get_next_msg(&self, &msg);
    handle_msg(&msg);
  }
}

K_THREAD_DEFINE(mcu_exchange_module_thread, CONFIG_MCU_EXCHANGE_THREAD_STACK_SIZE, module_thread_fn,
                NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_FINAL(MODULE, mcu_exchange_module_event);
