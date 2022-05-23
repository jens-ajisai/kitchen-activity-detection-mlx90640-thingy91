#define MODULE control_module

#include <event_manager.h>
#include <logging/log.h>
#ifdef CONFIG_CAF_BUTTONS
#include <caf/events/button_event.h>
#endif
#include "mcu_exchange/mcu_exchange_module_event.h"
#include "utils.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

static bool handle_mcu_exchange_module_event(const struct mcu_exchange_module_event* event) {
  if (event->type == MCU_EXCHANGE_EVT_ERROR) {
    send_led_event(LED_STATE_ERROR);
  }
  return false;
}

#ifdef CONFIG_CAF_BUTTONS
enum button_id { BUTTON_ID_0, BUTTON_ID_COUNT };

static bool handle_button_event(const struct button_event* evt) {
  LOG_DBG("button_event pressed:%d, key_id:%d", evt->pressed, evt->key_id);
  if (evt->pressed) {
    switch (evt->key_id) {
      case BUTTON_ID_0:
        send_led_event(LED_TEST_BLUE);
        struct mcu_exchange_module_event* event = new_mcu_exchange_module_event(0);
        event->type = MCU_EXCHANGE_EVT_BUTTON;
        EVENT_SUBMIT(event);
        break;
      default:
        break;
    }
  } else {
    switch (evt->key_id) {
      case BUTTON_ID_0:
        send_led_event(LED_OFF);
        break;
      default:
        break;
    }
  }

  return false;
}
#endif

static bool event_handler(const struct event_header* eh) {
  if (is_mcu_exchange_module_event(eh)) {
    return handle_mcu_exchange_module_event(cast_mcu_exchange_module_event(eh));
  }
#ifdef CONFIG_CAF_BUTTONS
  if (is_button_event(eh)) {
    return handle_button_event(cast_button_event(eh));
  }
#endif
  LOG_WRN("unhandled event.");
  return false;
}

EVENT_LISTENER(MODULE, event_handler);
#ifdef CONFIG_CAF_BUTTONS
EVENT_SUBSCRIBE(MODULE, button_event);
#endif
EVENT_SUBSCRIBE(MODULE, mcu_exchange_module_event);