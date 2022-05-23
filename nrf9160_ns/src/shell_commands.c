#ifdef CONFIG_SHELL

#include <shell/shell.h>

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#include <memfault/http/http_client.h>
#include <memfault/metrics/metrics.h>
#include <modem/lte_lc.h>

#include "common/diag.h"
#include "memfault/ports/zephyr/root_cert_storage.h"
#endif

#include <logging/log.h>

#include "adafruit_certificates.h"
#include "common/memory_hook.h"
#include "https_client.h"
#include "mqtt/mqtt_client.h"
#include "utils.h"

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
  }
  return 0;
}

#include <sys/base64.h>
#define BASE64_ENCODE_LEN(bin_len) (4 * (((bin_len) + 2) / 3))
#define BASE64_DECODE_LEN(bin_len) (((bin_len)*3) / 4)

static int memfault_send_data_test(const struct shell *shell, size_t argc, char **argv) {
  char *memfaultData;
  while (retrieve_memfault_data_as_base64(&memfaultData)) {
    if (memfaultData) {
      uint16_t decoded_data_size = BASE64_DECODE_LEN(strlen(memfaultData));
      uint8_t *decoded_data =
          my_malloc(decoded_data_size, HEAP_MEMORY_STATISTICS_ID_SHELL_HEATMAP_DATA);
      size_t written_size = 0;

      int ret = base64_decode(decoded_data, decoded_data_size, &written_size, memfaultData,
                              strlen(memfaultData));
      if (ret) {
        LOG_ERR("base64_decode error: %d", ret);
      }

      const char *hostname = MEMFAULT_HTTP_GET_CHUNKS_API_HOST();
      const int port = MEMFAULT_HTTP_GET_CHUNKS_API_PORT();
      const char *deviceId = CONFIG_MEMFAULT_NCS_DEVICE_ID;
      const char *projectKey = CONFIG_MEMFAULT_NCS_PROJECT_KEY;

      sec_tag_t tls_sec_tag[] = {
          kMemfaultRootCert_DigicertRootCa,
          kMemfaultRootCert_DigicertRootG2,
          kMemfaultRootCert_CyberTrustRoot,
          kMemfaultRootCert_AmazonRootCa1,
      };

      // TODO test this!!!
      //  certs provisions is done by memfault_zephyr_port_install_root_certs() on startup;
      send_memfault_https(hostname, port, tls_sec_tag, sizeof(tls_sec_tag), deviceId, projectKey,
                          memfaultData, strlen(memfaultData));

      my_free(decoded_data);
    }
    my_free(memfaultData);
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

/* The mqtt client struct */
static struct mqtt_client client;

static sec_tag_t adafruit_tls_sec_tag[] = {kAdafruitRootCert_DigicertRootCa,
                                           kAdafruitRootCert_DigicertGeotrustCa,
                                           kAdafruitRootCert_Adafruit};

static struct mqtt_client_info client_info = {
    .brokerHostname = CONFIG_MQTT_BROKER_HOSTNAME,
    .brokerPort = CONFIG_MQTT_BROKER_PORT,
    .clientId = CONFIG_MQTT_CLIENT_ID,
    .userName = CONFIG_MQTT_USER_NAME,
    .password = CONFIG_MQTT_PASSWORD,
    .libtls = CONFIG_MQTT_LIB_TLS,
    .tlsSecTags = adafruit_tls_sec_tag,
    .tlsSecTagsLen = sizeof(adafruit_tls_sec_tag) / sizeof(adafruit_tls_sec_tag[0]),
    .peerVerify = CONFIG_MQTT_TLS_PEER_VERIFY,
    .sessionCaching = CONFIG_MQTT_TLS_SESSION_CACHING,
};

static void mqtt_connected_cb(const int result) { LOG_INF("mqtt_connected_cb %d", result); }

static void mqtt_disconnected_cb(const int result) { LOG_INF("mqtt_disconnected_cb %d", result); }

static void mqtt_received_cb(const int result, const uint8_t *data, const size_t len) {
  if (result != 0) {
    LOG_INF("mqtt_received_cb %d", result);
  } else {
    char buf[len + 1];
    memcpy(buf, data, len);
    buf[len] = 0;
    LOG_INF("mqtt_received_cb %d %s", result, buf);
  }
}

static struct mqtt_client_module_cb mqtt_client_module_callbacs = {
    .mqtt_connected_cb = mqtt_connected_cb,
    .mqtt_disconnected_cb = mqtt_disconnected_cb,
    .mqtt_received_cb = mqtt_received_cb,
};

static int shell_mqtt_client_init(const struct shell *shell, size_t argc, char **argv) {
  mqtt_client_module_init(&client, &client_info, &mqtt_client_module_callbacs);
  return 0;
}

static int shell_mqtt_client_connect(const struct shell *shell, size_t argc, char **argv) {
  mqtt_client_module_connect(&client);
  return 0;
}

static int shell_mqtt_client_disconnect(const struct shell *shell, size_t argc, char **argv) {
  mqtt_client_module_disconnect(&client);
  return 0;
}

static int shell_mqtt_client_subscribe(const struct shell *shell, size_t argc, char **argv) {
  const char *topic = CONFIG_MQTT_TOPIC_HUMIDITY;
  mqtt_client_module_subscribe(&client, topic);
  return 0;
}

static int shell_mqtt_client_data_publish(const struct shell *shell, size_t argc, char **argv) {
  const char *topic = CONFIG_MQTT_TOPIC_TEMPERATURE;
  const char *data = "23";
  mqtt_client_module_data_publish(&client, topic, (uint8_t *)data, strlen(data) + 1);
  return 0;
}

static int connect_lte(const struct shell *shell, size_t argc, char **argv) {
  LOG_DBG("Waiting for network.. ");
  int err = lte_lc_init_and_connect();
  if (err) {
    LOG_ERR("Failed to connect to the LTE network, err %d", err);
    return 1;
  }
  send_led_event(LED_EVENT_CONNECTED);
  return 0;
}

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

static int test_led(const struct shell *shell, size_t argc, char **argv) {
  send_led_event(LED_TEST_RED);
  k_sleep(K_SECONDS(1));
  send_led_event(LED_TEST_GREEN);
  k_sleep(K_SECONDS(1));
  send_led_event(LED_TEST_BLUE);
  k_sleep(K_SECONDS(1));
  send_led_event(LED_EVENT_WARNING);
  return 0;
}

static int clear_led(const struct shell *shell, size_t argc, char **argv) {
  send_led_event(LED_OFF);
  return 0;
}

#include "mcu_exchange/mcu_exchange_module_event.h"
static const char *mock_heatMapData =
    "{\"type\":1,\"data\":"
    "\"GAE2AUoBVAFeAXIBhgGaAbgBzAHMAcIBuAGuAZABfAFoAVQBSgFAATYBLAEsASwBLAE2ATYBLAEsARgBBAHmADYBVAFo"
    "AYYBmgHCAeoBCAImAjoCOgImAggC6gHCAZoBfAFoAVQBSgFAATYBNgE2AUABQAFAATYBNgE2ASIBBAFKAWgBhgGuAeABEg"
    "JOAoACqAK8ArIClAJsAjAC9AHCAZoBcgFeAUoBQAFAAUABQAFAAUABQAFAAUABNgEsARgBVAF8AaQB4AEcAmwCvAICAzQD"
    "UgNIAxYD2gKUAjoC9AG4AYYBaAFUAUoBQAFAAUABQAFAAUABQAFAAUABNgEsAV4BkAHCAQgCWAK8AioDhAPKA+"
    "gD3gOiA1ID7gKKAiYC4AGkAXIBXgFUAUABQAFAAUABQAFAAUABQAFAAUABNgFoAaQB4AEwAp4CFgOOA/"
    "wDVgR0BGoEJATAA0gD0AJYAv4BuAF8AWgBVAFAAUABQAFAAUABQAFAAUABQAFAATYBaAGkAfQBWALGAlID6ANgBLoE4gTO"
    "BIgEGgSOAwIDgAISAsIBkAFeAUoBQAE2ATYBQAFAAUABQAFAAUABQAE2AWgBpAH0AVgC5AJwAxAEkgT2BB4FCgW6BEIErA"
    "MgA5QCHALMAYYBXgFAATYBLAEsATYBNgFAAUABQAFAAUABNgFKAZAB4AFYAtACZgMGBJIE9gQoBRQFugRCBKwDFgOKAhwC"
    "zAGGAVQBNgEsASwBLAE2ATYBNgFAAUABQAE2ATYBIgFeAbgBHAKoAjQD1ANWBLoE4gTOBIgEEASEA/"
    "gCdgIIArgBcgFAASIBGAEYARgBIgE2ATYBNgE2ATYBNgE2AeYALAF8AeABTgLaAmYD6ANCBGoEYAQaBLYDPgO8Ak4C6gGa"
    "AV4BNgEYAQ4BGAEYASwBLAE2ATYBNgE2ATYBIgGqAOYALAF8AeABYgLaAlIDogPAA8ADmAM+A+"
    "QCdgImAswBhgFKASwBDgEOARgBIgEsATYBNgE2AUABQAE2ASwB\",\"len\":1536}";

static int test_mcuExchange_message_heatMapData(const struct shell *shell, size_t argc,
                                                char **argv) {
  struct mcu_exchange_module_event *event =
      new_mcu_exchange_module_event(strlen(mock_heatMapData) + 1);
  event->type = MCU_EXCHANGE_EVT_DATA_READY;
  memcpy(event->dyndata.data, mock_heatMapData, strlen(mock_heatMapData) + 1);
  EVENT_SUBMIT(event);
  return 0;
}

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

#include <sys/reboot.h>
static int reboot(const struct shell *shell, size_t argc, char **argv) {
  sys_reboot(SYS_REBOOT_COLD);
}

#include <arm_math.h>

#include "../../test/testHeatMapData.h"
static int test_arm_stats(const struct shell *shell, size_t argc, char **argv) {
  float32_t val;
  uint32_t index;

  uint16_t size = TESTHEATMAP_DECODED_HEIGHT * TESTHEATMAP_DECODED_WIDTH;

  arm_max_f32((float32_t *)testHeatMapData, size, &val, &index);
  shell_print(shell, "max %f at %d", val, index);

  arm_min_f32((float32_t *)testHeatMapData, size, &val, &index);
  shell_print(shell, "min %f at %d", val, index);

  arm_mean_f32((float32_t *)testHeatMapData, size, &val);
  shell_print(shell, "mean %f", val);

  arm_rms_f32((float32_t *)testHeatMapData, size, &val);
  shell_print(shell, "rms %f", val);

  arm_std_f32((float32_t *)testHeatMapData, size, &val);
  shell_print(shell, "std %f", val);

  arm_var_f32((float32_t *)testHeatMapData, size, &val);
  shell_print(shell, "var %f", val);

  int startX = 2;
  int startY = 3;
  int with = 4;
  int height = 5;

  for (int row = 0; row < 24; ++row) {
    for (int col = 0; col < 32; ++col) {
      //      shell_print(shell, "(%d, %d) = %f", row, col, testHeatMapData[row * 32 + col]);
    }
  }

  int counter = 0;
  float subArray[with * height];

  for (int row = startY; row < startY + height; ++row) {
    for (int col = startX; col < startX + with; ++col) {
      subArray[counter++] = testHeatMapData[row * 32 + col];
    }
  }

  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < with; ++col) {
      shell_print(shell, "S (%d, %d) = %f", row, col, subArray[row * with + col]);
    }
  }

  return 0;
  /*
  void arm_sub_f32	(	const float32_t * 	pSrcA,
                      const float32_t * 	pSrcB,
                      float32_t * 	pDst,
                      uint32_t 	blockSize
                    )

  float32_t arm_cityblock_distance_f32	(	const float32_t * 	pA,
                                          const float32_t * 	pB,
                                          uint32_t 	blockSize
                                          )

  float32_t arm_euclidean_distance_f32	(	const float32_t * 	pA,
                                          const float32_t * 	pB,
                                          uint32_t 	blockSize
                                          )

  void arm_correlate_f32	(	const float32_t * 	pSrcA,
                            uint32_t 	srcALen,
                            const float32_t * 	pSrcB,
                            uint32_t 	srcBLen,
                            float32_t * 	pDst
                            )

  void arm_mat_init_f32	(	arm_matrix_instance_f32 * 	S,
  uint16_t 	nRows,
  uint16_t 	nColumns,
  float32_t * 	pData
  )
  arm_status arm_mat_sub_f32	(	const arm_matrix_instance_f32 * 	pSrcA,
  const arm_matrix_instance_f32 * 	pSrcB,
  arm_matrix_instance_f32 * 	pDst
  )


  */
}

#include "analysis/analysis_impl.h"
static void printHistory(const struct shell *shell) {
  struct ring_buf* gas_stove_stats_history = getHistory_gas_stove();

  struct stats * allStats =  my_malloc(HISTORY_SIZE * sizeof(struct stats), HEAP_MEMORY_STATISTICS_ID_UTILS_HEATMAP_DATA);
  uint32_t size = ring_buf_peek(&gas_stove_stats_history, (uint8_t*) allStats, HISTORY_SIZE * sizeof(struct stats));
  uint32_t entriesCount = size / sizeof(struct stats);
  
  shell_print(shell, "entriesCount=%d", entriesCount);
  for(uint32_t i = 0; i < entriesCount; i++) {
    shell_print(shell, "min[%d]=%f, max[%d]=%f, mean[%d]=%f, var[%d]=%f", i, allStats[i].min, i, allStats[i].max, i, allStats[i].mean, i, allStats[i].var);
  }
  my_free(allStats);
}

static int test_analyze_heatmap(const struct shell *shell, size_t argc, char **argv) {
  uint16_t entriesCount = TESTHEATMAP_DECODED_HEIGHT * TESTHEATMAP_DECODED_WIDTH;
  float *heatMap1 =
      my_malloc(entriesCount * sizeof(float), HEAP_MEMORY_STATISTICS_ID_UTILS_HEATMAP_DATA);
  float *heatMap2 =
      my_malloc(entriesCount * sizeof(float), HEAP_MEMORY_STATISTICS_ID_UTILS_HEATMAP_DATA);
  memcpy(heatMap1, testHeatMapData, entriesCount * sizeof(float));

  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  arm_offset_f32(heatMap1, 5.0, heatMap2, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap2);
  arm_offset_f32(heatMap2, 5.0, heatMap1, entriesCount);
  printHistory(shell);
  analyze_heatmap(heatMap1);
  printHistory(shell);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_appCtrl, SHELL_CMD(echo, NULL, "Echo back the arguments", echo),
    SHELL_CMD(reboot, NULL, "System cold reboot", reboot),
    SHELL_CMD(test_arm_stats, NULL, "System cold reboot", test_arm_stats),
    SHELL_CMD(test_analyze_heatmap, NULL, "System cold reboot", test_analyze_heatmap),
#ifdef CONFIG_MEMFAULT
    SHELL_CMD(memfault_send_data_test, NULL, "Retrieve and send memfault data",
              memfault_send_data_test),
    SHELL_CMD(memfault_retrieve_data, NULL, "Retrieve and log the memfault data",
              memfault_retrieve_data),
    SHELL_CMD(memfault_some_tests, NULL, "Retrieve and log the memfault data", memfault_some_tests),
#endif
    SHELL_CMD(connect_lte, NULL, "connect to lte", connect_lte),
    SHELL_CMD(shell_mqtt_client_init, NULL, "Log hello", shell_mqtt_client_init),
    SHELL_CMD(shell_mqtt_client_connect, NULL, "Log hello", shell_mqtt_client_connect),
    SHELL_CMD(shell_mqtt_client_disconnect, NULL, "Log hello", shell_mqtt_client_disconnect),
    SHELL_CMD(shell_mqtt_client_subscribe, NULL, "Log hello", shell_mqtt_client_subscribe),
    SHELL_CMD(shell_mqtt_client_data_publish, NULL, "Log hello", shell_mqtt_client_data_publish),
    SHELL_CMD(log_hello, NULL, "Log hello", log_hello),
    SHELL_CMD(mock_heatMap_data, NULL, "Log hello", test_mcuExchange_message_heatMapData),
    SHELL_CMD(get_memory_stats, NULL, "Get allocation leftovers", get_memory_stats),
    SHELL_CMD(test_led, NULL, "test led", test_led),
    SHELL_CMD(clear_led, NULL, "clear led", clear_led), SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(appCtrl, &sub_appCtrl, "Control heaty nrf9160", NULL);

#endif
