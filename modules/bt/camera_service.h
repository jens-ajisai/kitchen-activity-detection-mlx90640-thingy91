#ifndef __CAMERA_SERVICE_H
#define __CAMERA_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <zephyr/types.h>
#include <bluetooth/conn.h>

#define BT_UUID_CAMERA_SERVICE_VAL 	 BT_UUID_128_ENCODE(0xCAAF0900, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_JPEG_SIZE_VAL 		 BT_UUID_128_ENCODE(0xCAAF0901, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_LIGHT_MODE_VAL 		 BT_UUID_128_ENCODE(0xCAAF0902, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_COLOR_SATURATION_VAL BT_UUID_128_ENCODE(0xCAAF0903, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_BRIGHTNESS_VAL 		 BT_UUID_128_ENCODE(0xCAAF0904, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_CONTRAST_VAL 		 BT_UUID_128_ENCODE(0xCAAF0905, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_SPECIAL_EFFECTS_VAL  BT_UUID_128_ENCODE(0xCAAF0906, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_TAKE_PICTURE_VAL 	 BT_UUID_128_ENCODE(0xCAAF0907, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_JPEG_QUALITY_VAL 	 BT_UUID_128_ENCODE(0xCAAF0909, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_CAM_INTERVAL_VAL 	 BT_UUID_128_ENCODE(0xCAAF090A, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)
#define BT_UUID_CAMERA_TX_VAL        BT_UUID_128_ENCODE(0xCAAF090B, 0xC332, 0x42A8, 0x93BD, 0x25E905756CB8)

#define BT_UUID_SERVICE_CAMERA      BT_UUID_DECLARE_128(BT_UUID_CAMERA_SERVICE_VAL)
#define BT_UUID_JPEG_SIZE			BT_UUID_DECLARE_128(BT_UUID_JPEG_SIZE_VAL)
#define BT_UUID_LIGHT_MODE			BT_UUID_DECLARE_128(BT_UUID_LIGHT_MODE_VAL)
#define BT_UUID_COLOR_SATURATION	BT_UUID_DECLARE_128(BT_UUID_COLOR_SATURATION_VAL)
#define BT_UUID_BRIGHTNESS			BT_UUID_DECLARE_128(BT_UUID_BRIGHTNESS_VAL)
#define BT_UUID_CONTRAST			BT_UUID_DECLARE_128(BT_UUID_CONTRAST_VAL)
#define BT_UUID_SPECIAL_EFFECTS		BT_UUID_DECLARE_128(BT_UUID_SPECIAL_EFFECTS_VAL)
#define BT_UUID_TAKE_PICTURE		BT_UUID_DECLARE_128(BT_UUID_TAKE_PICTURE_VAL)
#define BT_UUID_JPEG_QUALITY		BT_UUID_DECLARE_128(BT_UUID_JPEG_QUALITY_VAL)
#define BT_UUID_CAM_INTERVAL		BT_UUID_DECLARE_128(BT_UUID_CAM_INTERVAL_VAL)
#define BT_UUID_CAMERA_TX           BT_UUID_DECLARE_128(BT_UUID_CAMERA_TX_VAL)

typedef void (*jpeg_size_cb_t)(const uint8_t value);
typedef void (*light_mode_cb_t)(const uint8_t value);
typedef void (*color_saturation_cb_t)(const uint8_t value);
typedef void (*brightness_cb_t)(const uint8_t value);
typedef void (*contrast_cb_t)(const uint8_t value);
typedef void (*special_effects_cb_t)(const uint8_t value);
typedef void (*take_picture_cb_t)(const bool state);
typedef void (*jpeg_quality_cb_t)(const uint8_t value);
typedef void (*cam_interval_cb_t)(const uint8_t value);

struct camera_service_cb {
	jpeg_size_cb_t			jpeg_size_cb;
	light_mode_cb_t			light_mode_cb;
	color_saturation_cb_t	color_saturation_cb;
	brightness_cb_t			brightness_cb;
	contrast_cb_t			contrast_cb;
	special_effects_cb_t	special_effects_cb;
	take_picture_cb_t		take_picture_cb;
	jpeg_quality_cb_t		jpeg_quality_cb;
	cam_interval_cb_t		cam_interval_cb;
};

int camera_service_init(struct camera_service_cb *callbacks);

int camera_service_send_picture(struct bt_conn *conn, const uint8_t* heat_map, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* __CAMERA_SERVICE_H */
