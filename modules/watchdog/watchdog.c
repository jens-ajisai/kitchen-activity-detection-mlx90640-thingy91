/*
 * This code is based on the task_wdt sample which has the below license
 * zephyr/samples/subsys/task_wdt
 * 
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MODULE watchdog

#include <event_manager.h>
#include <logging/log.h>

#include "mcu_exchange/mcu_exchange_module_event.h"
LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

#include <device.h>
#include <drivers/watchdog.h>
#include <sys/reboot.h>
#include <task_wdt/task_wdt.h>

static int task_wdt_id_main;
static int task_wdt_id_mcu_comm;

static void task_wdt_callback(int channel_id, void *user_data) {
  LOG_INF("Task watchdog channel %d callback, thread: %s", channel_id, (char*) user_data);

  /*
   * If the issue could be resolved, call task_wdt_feed(channel_id) here
   * to continue operation.
   *
   * Otherwise we can perform some cleanup and reset the device.
   */

  LOG_INF("Resetting device... (not active for now)");

  // should not be active for now. Only PoC.
  task_wdt_feed(channel_id);
  //	sys_reboot(SYS_REBOOT_COLD);
}

const char *main_channel_name = "wdt_main_channel";
const char *mcu_comm_channel_name = "wdt_mcu_comm_channel";

void init_and_start_watchdog() {
  const struct device *hw_wdt_dev = device_get_binding(DT_LABEL(DT_NODELABEL(wdt)));

  LOG_INF("Init and start watchdog.");

  if (!device_is_ready(hw_wdt_dev)) {
    LOG_INF("Hardware watchdog %s is not ready; ignoring it.", hw_wdt_dev->name);
    hw_wdt_dev = NULL;
  }

  task_wdt_init(hw_wdt_dev);

  /* passing NULL instead of callback to trigger system reset */
  task_wdt_id_main = task_wdt_add(CONFIG_WATCHDOG_TIMER_MAIN_CHANNEL, task_wdt_callback,
                                  (void *)main_channel_name);
  task_wdt_id_mcu_comm = task_wdt_add(CONFIG_WATCHDOG_TIMER_MCU_COMM_CHANNEL, task_wdt_callback,
                                      (void *)mcu_comm_channel_name);
}

/*
 * This high-priority thread needs a tight timing
 */
void control_thread(void) {
  LOG_INF("Control thread started.");

  while (true) {
    task_wdt_feed(task_wdt_id_main);
    k_sleep(K_MSEC(1000));
  }
}

K_THREAD_DEFINE(control, 1024, control_thread, NULL, NULL, NULL, -1, 0, 1000);

static bool event_handler(const struct event_header *eh) {
  LOG_DBG("event_handler");

  if (is_mcu_exchange_module_event(eh)) {
    struct mcu_exchange_module_event *event = cast_mcu_exchange_module_event(eh);
    if (event->type == MCU_EXCHANGE_EVT_HEAT_MAP_DATA_READY) {
      LOG_INF("Feed wdt on channel MCU Comm.");
      task_wdt_feed(task_wdt_id_mcu_comm);
    }
  }
  return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, mcu_exchange_module_event);