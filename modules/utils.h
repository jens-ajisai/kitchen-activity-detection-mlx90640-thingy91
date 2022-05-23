#ifndef _UTILS_H_
#define _UTILS_H_

#include <zephyr.h>
#include <sys/base64.h>

#include "mcu_exchange/mcu_exchange_module_event.h"

#define BASE64_ENCODE_LEN(bin_len) (4 * (((bin_len) + 2) / 3))
#define BASE64_DECODE_LEN(bin_len) (((bin_len)*3) / 4)

void init_utils();
char *encode_base64(uint8_t *data, uint16_t len);
const char* encode_message(int type, uint8_t* data, uint16_t len);
void parse_message(const char* message, int* type, uint8_t** data, uint16_t* len);

void convertAndScaleHeatMap(int16_t heatMap[], uint8_t *data, uint16_t entriesCount);
void revertHeatMapToFloat(float heatMap[], uint8_t* data, uint16_t entriesCount);
uint16_t compressData(uint8_t **dst, const uint8_t *src, const uint16_t src_size);
char* HeatmapToBase64Image(uint8_t* data, size_t len);

#if defined CONFIG_CAF_LEDS

#include <caf/events/led_event.h>
#include "led_states.h"
void send_led_event(enum led_states led_effect);
#endif

void blink(int time);

#endif /* _UTILS_H_ */
