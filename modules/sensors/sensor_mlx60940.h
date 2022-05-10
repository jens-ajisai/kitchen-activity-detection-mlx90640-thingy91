/*
 * This code took partly the Adafruit mlx90640-library as a reference which has the below license
 * https://github.com/adafruit/Adafruit_MLX90640
 * 
 *  Apache License Version 2.0, January 2004
 */

#ifndef _SENSOR_MLX60940_H_
#define _SENSOR_MLX60940_H_

#define HEAT_MAP_DATA_SIZE (sizeof(float) * 24 * 32)

#define MLX90640_I2CADDR DT_PROP(DT_NODELABEL(mlx90640), reg)
#define MLX90640_DEVICEID 0x2407

#define OPENAIR_TA_SHIFT 8  ///< Default 8 degree offset from ambient air

/** Mode to read pixel frames (two per image) */
enum mlx90640_mode {
  MLX90640_INTERLEAVED,
  MLX90640_CHESS,
};

/** Internal ADC resolution for pixel calculation */
enum mlx90640_res {
  MLX90640_ADC_16BIT,
  MLX90640_ADC_17BIT,
  MLX90640_ADC_18BIT,
  MLX90640_ADC_19BIT,
};

/** How many PAGES we will read per second (2 pages per frame) */
enum mlx90640_refreshrate {
  MLX90640_0_5_HZ,
  MLX90640_1_HZ,
  MLX90640_2_HZ,
  MLX90640_4_HZ,
  MLX90640_8_HZ,
  MLX90640_16_HZ,
  MLX90640_32_HZ,
  MLX90640_64_HZ,
};

typedef void (*notify_heatMap_cb_t)(float* heat_map, uint16_t size);

struct mlx60940_cb {
  notify_heatMap_cb_t notify_heatMap_cb;
};

#ifdef __cplusplus
extern "C" {
#endif

int init_mlx60940(struct mlx60940_cb* callbacks);
void set_inverval_mlx60940(uint16_t interval);

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_MLX60940_H_ */
