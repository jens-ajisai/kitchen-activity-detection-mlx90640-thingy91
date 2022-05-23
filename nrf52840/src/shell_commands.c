#ifdef CONFIG_SHELL

#include <shell/shell.h>

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#include <memfault/metrics/metrics.h>

#include "common/diag.h"
#include "utils.h"
#endif

#include <logging/log.h>

#include "common/memory_hook.h"
LOG_MODULE_REGISTER(shell, CONFIG_HEATY_LOG_LEVEL);

#ifdef CONFIG_MEMFAULT
static int memfault_retrieve_data(const struct shell *shell, size_t argc, char **argv) {
  char *memfaultData;
  while (retrieve_memfault_data_as_base64(&memfaultData)) {
    if (memfaultData) {
      shell_print(
          shell,
          "memfault --project-key CONFIG_MEMFAULT_NCS_PROJECT_KEY post-chunk --encoding base64 %s",
          memfaultData);
    }
    my_free(memfaultData);
    LOG_DBG("Free %p", memfaultData);
  }
  return 0;
}

static int memfault_some_tests(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  memfault_metrics_heartbeat_set_unsigned(MEMFAULT_METRICS_KEY(testMetric), 462);
  memfault_metrics_heartbeat_debug_trigger();

  return 0;
}
#endif

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

#include "../../test/testHeatMapData.h"
#include "sensors/sensors_ids.h"
#include "sensors/sensors_module_event.h"

static int sensors_mockReading_mlx90640(const struct shell *shell, size_t argc, char **argv) {
  uint16_t size = TESTHEATMAP_DECODED_HEIGHT * TESTHEATMAP_DECODED_WIDTH * sizeof(float);

  struct sensors_data_module_event *event = new_sensors_data_module_event(size);
  event->type = SENSORS_EVT_HEAT_MAP_DATA_READY;
  event->timestamp = k_uptime_get();
  event->sensorsId = SENSOR_HEAT_MAP;
  memcpy(event->dyndata.data, testHeatMapData, size);
  event->dyndata.size = size;

  EVENT_SUBMIT(event);
  return 0;
}

#ifdef CONFIG_HEAP_MEMORY_STATISTICS
static int get_memory_stats(const struct shell *shell, size_t argc, char **argv) {
  struct memory_stat_list *stats;
  uint16_t size = getMemoryStats(&stats);
  for (uint16_t i = 0; i < size; i++) {
    shell_print(shell, "%s: %d", get_memory_stat_str(stats[i].id), stats[i].len);
  }
  // exception here not to use my_free
  k_free(stats);
  return 0;
}

static int test_memory_stats(const struct shell *shell, size_t argc, char **argv) {
  int key = 10;
  int key2 = 11;
  int key3 = 12;
  struct memory_stat_list stats = {.id = 0, .len = 20};
  struct memory_stat_list stats2 = {.id = 1, .len = 21};
  struct memory_stat_list stats3 = {.id = 2, .len = 22};
  mapPutStat(&key, stats);
  mapPutStat(&key2, stats2);
  mapPutStat(&key3, stats3);

  struct memory_stat_list ret = {0};
  mapGetStat(&key2, &ret);
  int storedLen2 = ret.len;
  if (storedLen2 == stats2.len) {
    shell_print(shell, "test_memory_stats: 2 OK");
  } else {
    shell_print(shell, "test_memory_stats: comparison 2 FAIL");
  }
  mapRemoveStat(&key2);
  mapGetStat(&key, &ret);
  int storedLen = ret.len;
  if (storedLen == stats.len) {
    shell_print(shell, "test_memory_stats: 1 OK");
  } else {
    shell_print(shell, "test_memory_stats: comparison 1 FAIL");
  }
  mapGetStat(&key2, &ret);
  storedLen2 = ret.len;
  if (storedLen2 == 0) {
    shell_print(shell, "test_memory_stats: 2 OK");
  } else {
    shell_print(shell, "test_memory_stats: comparison 2 FAIL");
  }
  mapGetStat(&key3, &ret);
  int storedLen3 = ret.len;
  if (storedLen3 == stats3.len) {
    shell_print(shell, "test_memory_stats: 3 OK");
  } else {
    shell_print(shell, "test_memory_stats: comparison 3 FAIL");
  }
  return 0;
}
#endif

#include <sys/reboot.h>
static int reboot(const struct shell *shell, size_t argc, char **argv) {
  sys_reboot(SYS_REBOOT_COLD);
}

#include "mcu_exchange/mcu_exchange_module_event.h"
static int mock_error(const struct shell *shell, size_t argc, char **argv) {
  struct mcu_exchange_module_event *event = new_mcu_exchange_module_event(0);
  event->type = MCU_EXCHANGE_EVT_ERROR;
  event->received = false;
  EVENT_SUBMIT(event);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_appCtrl, SHELL_CMD(echo, NULL, "Echo back t+he arguments", echo),
    SHELL_CMD(reboot, NULL, "System cold reboot", reboot),
    SHELL_CMD(mock_error, NULL, "Send MCU_EXCHANGE_EVT_ERROR", mock_error),
#ifdef CONFIG_MEMFAULT
    SHELL_CMD(memfault_retrieve_data, NULL, "Retrieve and log the memfault data",
              memfault_retrieve_data),
    SHELL_CMD(memfault_some_tests, NULL, "Retrieve and log the memfault data", memfault_some_tests),
#endif
    SHELL_CMD(log_hello, NULL, "Log hello", log_hello),
    SHELL_CMD(module_state_ready, NULL, "Log hello", module_state_ready),
    SHELL_CMD(sensors_mockReading_mlx90640, NULL, "Send mock reading",
              sensors_mockReading_mlx90640),
#ifdef CONFIG_HEAP_MEMORY_STATISTICS
    SHELL_CMD(get_memory_stats, NULL, "Get allocation leftovers", get_memory_stats),
    SHELL_CMD(test_memory_stats, NULL, "Get allocation leftovers", test_memory_stats),
#endif
    SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(appCtrl, &sub_appCtrl, "Control heaty nrf52840", NULL);

#endif
