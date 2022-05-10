#ifndef _LED_STATES_H_
#define _LED_STATES_H_

enum led_states {
  LED_EVENT_STARTUP,
  LED_EVENT_CONNECTED,
  LED_EVENT_WARNING,               // on recoverable error
  LED_STATE_ERROR,                 // on error

  LED_TEST_RED,    // test
  LED_TEST_GREEN,  // test
  LED_TEST_BLUE,   // test

  LED_OFF,
  LED_STATE_COUNT
};

#endif /* _LED_STATES_H_ */

