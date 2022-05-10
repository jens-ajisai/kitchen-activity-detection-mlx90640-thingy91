#include <caf/events/led_event.h>
#include <caf/led_effect.h>

#include "led_states.h"

const struct {} led_state_def_include_once;

enum led_id {
  LED_ID_1,
  LED_ID_COUNT,
};

#define COLOR_BLACK 	LED_COLOR(0,0,0)
#define COLOR_WHITE 	LED_COLOR(50,50,50)
#define COLOR_RED 		LED_COLOR(50,0,0)
#define COLOR_LIME 		LED_COLOR(0,50,0)
#define COLOR_BLUE 		LED_COLOR(0,0,50)
#define COLOR_YELLOW 	LED_COLOR(50,50,0)
#define COLOR_CYAN 		LED_COLOR(0,50,50)
#define COLOR_MAGENTA 	LED_COLOR(50,0,50)
#define COLOR_SILVER 	LED_COLOR(40,40,40)
#define COLOR_GRAY 		LED_COLOR(25,25,25)
#define COLOR_MAROON 	LED_COLOR(25,0,0)
#define COLOR_OLIVE 	LED_COLOR(25,25,0)
#define COLOR_GREEN 	LED_COLOR(0,25,0)
#define COLOR_PURPLE 	LED_COLOR(25,0,25)
#define COLOR_TEAL 		LED_COLOR(0,25,25)
#define COLOR_NAVY 		LED_COLOR(0,0,25)

#define LED_PERIOD_NORMAL	500
#define LED_PERIOD_LONG		3500
#define LED_PERIOD_RAPID	200

static const struct led_effect led_state_effects[LED_STATE_COUNT] = {
    [LED_EVENT_STARTUP]   = LED_EFFECT_LED_ON_GO_OFF(COLOR_BLUE, LED_PERIOD_NORMAL, 1000), // crashes if off delay is 0
    [LED_EVENT_CONNECTED] = LED_EFFECT_LED_ON_GO_OFF(COLOR_GREEN, LED_PERIOD_NORMAL, 1000), // crashes if off delay is 0
    [LED_EVENT_WARNING]   = LED_EFFECT_LED_ON_GO_OFF(COLOR_YELLOW, LED_PERIOD_LONG, 1000), // crashes if off delay is 0
    [LED_STATE_ERROR]     = LED_EFFECT_LED_ON(COLOR_RED),
    [LED_TEST_RED]        = LED_EFFECT_LED_ON(COLOR_RED),
    [LED_TEST_GREEN]      = LED_EFFECT_LED_ON(COLOR_GREEN),
    [LED_TEST_BLUE]       = LED_EFFECT_LED_ON(COLOR_BLUE),
    [LED_OFF]             = LED_EFFECT_LED_OFF(),
};

static const struct led_effect led_effect_off = LED_EFFECT_LED_OFF();
static const struct led_effect led_effect_on = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255));
