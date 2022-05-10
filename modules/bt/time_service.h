#ifndef __TIME_SERVICE_H
#define __TIME_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <bluetooth/conn.h>

#define BT_UUID_SERVICE_TIME_VAL    BT_UUID_128_ENCODE(0xA00F0900, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_SET_TIME_VAL     BT_UUID_128_ENCODE(0xA00F0901, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERVICE_TIME       BT_UUID_DECLARE_128(BT_UUID_SERVICE_TIME_VAL)
#define BT_UUID_SET_TIME         BT_UUID_DECLARE_128(BT_UUID_SET_TIME_VAL)


typedef void (*set_time_cb_t)(const int64_t value);

struct time_service_cb {
	set_time_cb_t			set_time_cb;
};

int time_service_init(struct time_service_cb *callbacks);

#ifdef __cplusplus
}
#endif

#endif