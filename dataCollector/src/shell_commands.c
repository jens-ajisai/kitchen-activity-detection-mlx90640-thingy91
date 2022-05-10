#ifdef CONFIG_SHELL

#include <logging/log.h>
#include <shell/shell.h>

#include "common/memory_hook.h"
LOG_MODULE_REGISTER(shell, CONFIG_HEATY_LOG_LEVEL);

static int echo(const struct shell *shell, size_t argc, char **argv) {
  shell_print(shell, "argc = %d", argc);
  for (size_t cnt = 0; cnt < argc; cnt++) {
    shell_print(shell, "  argv[%d] = %s", cnt, argv[cnt]);
  }

  return 0;
}

static int log_hello(const struct shell *shell, size_t argc, char **argv) {
  LOG_INF("Hello World! %s\n", CONFIG_BOARD);
  LOG_INF("build time: " __DATE__ " " __TIME__);

  return 0;
}

extern void sendReady();
static int module_state_ready(const struct shell *shell, size_t argc, char **argv) {
  sendReady();
  return 0;
}

#include <time.h>
static int test_time_functions(const struct shell *shell, size_t argc, char **argv) {
  struct tm *tm_localtime;
  struct tm *tm_gmtime;
  time_t t;
  t = k_uptime_get();
  t /= 1000;
  tm_localtime = localtime(&t);
  tm_gmtime = gmtime(&t);
  shell_print(shell, "unix time in ms %lli", t);
  shell_print(shell, "asctime(tm_localtime): %s", asctime(tm_localtime));
  shell_print(shell, "asctime(tm_gmtime)   : %s", asctime(tm_gmtime));

  shell_print(shell, "custom formatting: %04u-%02u-%02u %02u:%02u:%02u", tm_localtime->tm_year + 1900,
          tm_localtime->tm_mon + 1, tm_localtime->tm_mday, tm_localtime->tm_hour,
          tm_localtime->tm_min, tm_localtime->tm_sec);

  return 0;
}

#ifdef CONFIG_DATE_TIME
#include <date_time.h>
#include <stdlib.h>

static int date_get(const struct shell *shell, size_t argc, char **argv) {
  int64_t t = 0;
  date_time_now(&t);
  t /= 1000;

  struct tm *tm_localtime;
  tm_localtime = localtime(&t);
  shell_print(shell, "%s unix time: %lli", asctime(tm_localtime), t);

  return 0;
}

static int date_set(const struct shell *shell, size_t argc, char **argv) {
  if (argc >= 2) {
    struct tm *tm_localtime;
    char *eptr;
    time_t t = strtoll(argv[1], &eptr, strlen(argv[1]));

    tm_localtime = localtime(&t);

    date_time_set(tm_localtime);
    shell_print(shell, "Set time to %s (%lli)", asctime(tm_localtime), t);
  }
  return 0;
}
#endif

#ifdef CONFIG_CAF_LEDS
#include "utils.h"
static int test_led(const struct shell *shell, size_t argc, char **argv) {
  shell_print(shell, "Testing red led");
  send_led_event(LED_TEST_RED);
  k_sleep(K_SECONDS(1));
  shell_print(shell, "Testing green led");
  send_led_event(LED_TEST_GREEN);
  k_sleep(K_SECONDS(1));
  shell_print(shell, "Testing blue led");
  send_led_event(LED_TEST_BLUE);
  k_sleep(K_SECONDS(1));
  shell_print(shell, "Testing warning event effect");
  send_led_event(LED_EVENT_WARNING);
  return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_appCtrl, SHELL_CMD(echo, NULL, "Echo back the arguments", echo),
                               SHELL_CMD(log_hello, NULL, "Log hello", log_hello),
                               SHELL_CMD(module_state_ready, NULL, "Set module_state_ready for manual start", module_state_ready),
                               SHELL_CMD(test_time_functions, NULL, "Test time functions", test_time_functions),
#ifdef CONFIG_DATE_TIME
                               SHELL_CMD(date_get, NULL, "Get the date by asctime", date_get),
                               SHELL_CMD(date_set, NULL, "Set the date by unix time format as argv", date_set),
#endif
#ifdef CONFIG_CAF_LEDS
                               SHELL_CMD(test_led, NULL, "test CAF LED module", test_led),
#endif
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(appCtrl, &sub_appCtrl, "Control heaty datacollector", NULL);

#endif
