#define MODULE utils

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

#ifdef CONFIG_CJSON_LIB
#include "cJSON.h"
#endif

#if defined CONFIG_CAF_LEDS
#include <caf/events/led_event.h>

#include "led_state_def.h"
#include "led_states.h"
#endif

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <stdio.h>
#include <sys/base64.h>
#include <zephyr.h>

#include "common/memory_hook.h"
#include "utils.h"

LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

// #include "sys/crc.h"
// crc32_ieee(msg->module.mcu_exchange.data.data.data, msg->module.mcu_exchange.data.data.len);

char* encode_base64(uint8_t* data, uint16_t len) {
  uint16_t encoded_data_size = BASE64_ENCODE_LEN(len + 1);
  char* encoded_data =
      my_malloc(encoded_data_size + 1, HEAP_MEMORY_STATISTICS_ID_UTILS_ENCODE_BASE64_MESSAGE);
  size_t written_size = 0;

  int ret = base64_encode(encoded_data, encoded_data_size + 1, &written_size, data, len);
  if (ret) {
    return NULL;
  }
  return encoded_data;
}

const char* encode_message(int type, uint8_t* data, uint16_t len) {
  if (len) {
    LOG_INF("encode message type=%d, len=%d, data=[%d,%d,%d,%d,...]", type, len, data[0], data[1],
            data[2], data[3]);
  }

  uint16_t encoded_data_size = BASE64_ENCODE_LEN(len + 1);
  char* encoded_data = my_malloc(encoded_data_size, HEAP_MEMORY_STATISTICS_ID_UTILS_ENCODE_MESSAGE);
  size_t written_size = 0;

  int ret = base64_encode(encoded_data, encoded_data_size, &written_size, data, len);
  if (ret) {
#ifdef CONFIG_MEMFAULT

#endif
    LOG_ERR("base64_encode error: %d", ret);
  }

#ifdef CONFIG_CJSON_LIB
  cJSON* json = cJSON_CreateObject();
  if (json == NULL) {
#ifdef CONFIG_MEMFAULT

#endif
    LOG_ERR("Failed to create header json");
  }

  cJSON_AddNumberToObject(json, "type", type);
  cJSON_AddNumberToObject(json, "len", len);
  cJSON_AddStringToObject(json, "data", encoded_data);
  char* jsonString = cJSON_Print(json);

  cJSON_Delete(json);
#else
  char *jsonString = my_malloc(64 + written_size, HEAP_MEMORY_STATISTICS_ID_CJSON_PREALLOC_STRING);
  LOG_DBG("Allocate %d bytes %p", 64 + written_size, jsonString);
  if (jsonString) {
    memset(jsonString, 0, 64 + written_size);
    sprintf(jsonString, "{ \"type\":%d, \"data\":\"%s\", \"len\":%d }", type, encoded_data, len);
  }
#endif
  my_free(encoded_data);
  return jsonString;
}

void parse_message(const char* message, int* type, uint8_t** decoded_data, uint16_t* len) {
  LOG_INF("parse_message: %s", message);

#ifdef CONFIG_CJSON_LIB
  cJSON* jsonRoot = cJSON_Parse(message);
  cJSON* jsonType = cJSON_GetObjectItemCaseSensitive(jsonRoot, "type");
  cJSON* jsonData = cJSON_GetObjectItemCaseSensitive(jsonRoot, "data");
  cJSON* jsonLen = cJSON_GetObjectItemCaseSensitive(jsonRoot, "len");
  *type = jsonType->valueint;
  *len = jsonLen->valueint;
  const char* data = jsonData->valuestring;

  uint16_t decoded_data_size = BASE64_DECODE_LEN(strlen(data));
  *decoded_data = my_malloc(decoded_data_size, HEAP_MEMORY_STATISTICS_ID_UTILS_DECODED_BASE64);
  size_t written_size = 0;

  int ret = base64_decode(*decoded_data, decoded_data_size, &written_size, data, strlen(data));
  if (ret) {
#ifdef CONFIG_MEMFAULT

#endif
    LOG_ERR("base64_decode error: %d", ret);
  }

  LOG_INF("parsed message type=%d, len=%d, data=[%d,%d,%d,%d,...]", *type, *len, (*decoded_data)[0],
          (*decoded_data)[1], (*decoded_data)[2], (*decoded_data)[3]);

  // do something
  cJSON_Delete(jsonRoot);
#endif
}

#if defined CONFIG_CAF_LEDS
void send_led_event(enum led_states led_effect) {
  if (led_effect < LED_STATE_COUNT) {
    struct led_event* event = new_led_event();

    event->led_id = LED_ID_1;
    event->led_effect = &(led_state_effects[led_effect]);
    EVENT_SUBMIT(event);
  } else {
    LOG_ERR("Unknown led effect or null");
  }
}
#endif

#include <math.h>
void convertAndScaleHeatMap(int16_t heatMap[], uint8_t* data, uint16_t entriesCount) {
  float* dataptr = (float*)data;
  for (uint16_t i = 0; i < entriesCount; i++) {
    static int scaling = 10;
    heatMap[i] = scaling * fmin(fmax(dataptr[i], SHRT_MIN / scaling), SHRT_MAX / scaling);
  }
}

void revertHeatMapToFloat(float heatMap[], uint8_t* data, uint16_t entriesCount) {
  int16_t* dataptr = (int16_t*)data;
  for (uint16_t i = 0; i < entriesCount; i++) {
    static float scaling = 10.0;
    heatMap[i] = dataptr[i] / scaling;
  }
}

void revertConvertAndScaleHeatMap(int8_t heatMap[], uint8_t* data, uint16_t entriesCount) {
  int16_t* dataptr = (int16_t*)data;
  for (uint16_t i = 0; i < entriesCount; i++) {
    static int scaling = 10;
    heatMap[i] = fmin(fmax(dataptr[i], 0 * scaling), UCHAR_MAX * scaling) / scaling;
  }
}

#ifdef CONFIG_LZ4
#include <lz4.h>

uint16_t compressData(uint8_t** dst, const uint8_t* src, const uint16_t src_size) {
  const int max_dst_size = LZ4_compressBound(src_size);

  *dst = my_malloc((size_t)max_dst_size, HEAP_MEMORY_STATISTICS_ID_UTILS_COMPRESS_DATA);
  LOG_DBG("Allocated %d bytes to %p", max_dst_size, *dst);
  if (*dst == NULL) {
    LOG_ERR("Failed to allocate memory for compressed data");
    return 0;
  }

  const int compressed_data_size = LZ4_compress_default(src, *dst, src_size, max_dst_size);
  if (compressed_data_size <= 0) {
    my_free(*dst);
    LOG_ERR("Failed to compress the data");

    return 0;
  }

  LOG_DBG("Original Data size: %d", src_size);
  LOG_DBG("Compressed Data size : %d", compressed_data_size);

  return max_dst_size;
}
#else
uint16_t compressData(uint8_t **dst, const uint8_t *src, const uint16_t src_size) {
  *dst = my_malloc((size_t)src_size, HEAP_MEMORY_STATISTICS_ID_UTILS_COMPRESS_DATA);
  if (*dst == NULL) {
    LOG_ERR("Failed to allocate memory for compressed data");

    return 0;
  }
  memcpy(*dst, src, src_size);
  return src_size;
}
#endif

#include "lodepng/lodepng.h"
char* HeatmapToBase64Image(uint8_t* data, size_t len) {
  char* tmp = NULL;
  uint16_t entriesCount = len / sizeof(int16_t);
  int8_t* heatMap =
      my_malloc(entriesCount * sizeof(int8_t), HEAP_MEMORY_STATISTICS_ID_UTILS_HEATMAP_DATA);
  revertConvertAndScaleHeatMap(heatMap, data, entriesCount);

#ifdef CONFIG_BOARD_THINGY91_NRF9160_NS
  if (heatMap[200] == 0) {
    send_led_event(LED_EVENT_WARNING);
  }
#endif

  unsigned char* encodedGrey;
  size_t encodedGreySize;
  unsigned err =
      lodepng_encode_memory(&encodedGrey, &encodedGreySize, heatMap, 32, 24, LCT_GREY, 8);
  if (err == 0) {
    tmp = encode_base64(encodedGrey, encodedGreySize);
  } else {
    LOG_ERR("lodepng_encode_memory: encoding FAIL, %d", err);
  }
  my_free(heatMap);
  if (encodedGrey) my_free(encodedGrey);
  LOG_DBG("HeatmapToBase64Image return");
  return tmp;
}

#ifdef CONFIG_CJSON_LIB
static cJSON_Hooks _cjson_hooks;

static void* malloc_fn_hook(size_t sz) { return my_malloc(sz, HEAP_MEMORY_STATISTICS_ID_CJSON); }

/**@brief Initialize cJSON by assigning function hooks. */
static void cJSON_Init() {
  _cjson_hooks.malloc_fn = malloc_fn_hook;
  _cjson_hooks.free_fn = my_free;

  cJSON_InitHooks(&_cjson_hooks);
}
#endif

void init_utils() {
#ifdef CONFIG_CJSON_LIB
  cJSON_Init();
#endif
}

#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
#define LED0 ""
#define PIN 0
#define FLAGS 0
#endif

void blink(int time) {
  const struct device* dev;
  bool led_is_on = true;
  int ret;

  dev = device_get_binding(LED0);
  if (dev == NULL) {
    return;
  }

  ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
  if (ret < 0) {
    return;
  }

  gpio_pin_set(dev, PIN, (int)1);
  led_is_on = !led_is_on;
  k_msleep(time);
  gpio_pin_set(dev, PIN, (int)0);
  k_msleep(time);
}
