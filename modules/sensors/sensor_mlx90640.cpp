/*
 * This code took partly the Adafruit mlx90640-library as a reference which has the below license
 * https://github.com/adafruit/Adafruit_MLX90640
 *
 *  Apache License Version 2.0, January 2004
 */

#include <zephyr.h>

#ifdef CONFIG_MEMFAULT
#include <memfault/core/trace_event.h>
#endif

#include <logging/log.h>

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "common/memory_hook.h"
#include "sensor_mlx60940.h"
LOG_MODULE_REGISTER(sensor_mlx60940, CONFIG_SENSORS_MODULE_LOG_LEVEL);

static struct mlx60940_cb mlx60940_cb;

#define MLX90640_STACK_SIZE CONFIG_MLX90640_STACK_SIZE
#define MLX90640_PRIORITY K_HIGHEST_APPLICATION_THREAD_PRIO

K_THREAD_STACK_DEFINE(mlx90640_stack_area, MLX90640_STACK_SIZE);
struct k_work_q mlx90640_work_q;

static struct k_work_delayable read_mlx90640_work;

k_timeout_t heat_map_interval = K_SECONDS(CONFIG_HEAT_MAP_CYCLE);

static paramsMLX90640 params;

static void read_heat_map(struct k_work* work) {
  uint16_t heatMap_size = 0;
  
  float emissivity = 0.95;
  float tr = 23.15;
  uint16_t mlx90640Frame[834];
  int status;

  float* heat_map =
      (float*)my_malloc(HEAT_MAP_DATA_SIZE, HEAP_MEMORY_STATISTICS_ID_SENSOR_HEATMAP_DATA);
  LOG_DBG("Allocate %d bytes %p", HEAT_MAP_DATA_SIZE, heat_map);
  if (heat_map == NULL) {
    LOG_ERR("Failed to allocate memory for the heat map!");
    goto end;
  }

  for (uint8_t page = 0; page < 2; page++) {
    status = MLX90640_GetFrameData(MLX90640_I2CADDR, mlx90640Frame);

    if (status < 0) {
      LOG_WRN("status is %d reading failed", status);

      LOG_DBG("Free %p", heat_map);
      goto end;
    }

    // For a MLX90640 in the open air the shift is -8 degC.
    tr = MLX90640_GetTa(mlx90640Frame, &params) - OPENAIR_TA_SHIFT;

    LOG_DBG("Tr = %f", tr);

    MLX90640_CalculateTo(mlx90640Frame, &params, emissivity, tr, heat_map);
  }

  // logging large data without sleep crashes the app. Somewhere out of memory ...
  // LOG_HEXDUMP_DBG(heat_map, HEAT_MAP_DATA_SIZE, "heat_map: ");
  heatMap_size = HEAT_MAP_DATA_SIZE;

end:
  if (mlx60940_cb.notify_heatMap_cb) {
    mlx60940_cb.notify_heatMap_cb(heat_map, heatMap_size);
  }

  if (heat_map) {
    LOG_DBG("Free %p", heat_map);
    my_free(heat_map);
  }
  k_work_reschedule_for_queue(&mlx90640_work_q, &read_mlx90640_work, heat_map_interval);
}

static int init_mlx60940_internal() {
  int ret;

  ret = MLX90640_I2CInit();
  if (ret) {
    LOG_ERR("Error in MLX90640_I2CInit: %d", ret);

    return ret;
  }

  uint16_t serialNumber[3];
  ret = MLX90640_I2CRead(MLX90640_I2CADDR, MLX90640_DEVICEID, 3, serialNumber);
  if (ret) {
    LOG_ERR("Error in MLX90640_I2CRead Serial: %d", ret);

    return ret;
  }
  LOG_HEXDUMP_INF(serialNumber, sizeof(uint16_t) * 3, "mlx60940 erial number: ");

  uint16_t eeMLX90640[832];
  ret = MLX90640_DumpEE(MLX90640_I2CADDR, eeMLX90640);
  if (ret) {
    LOG_ERR("Error in MLX90640_DumpEE: %d", ret);

    return ret;
  }
  LOG_HEXDUMP_DBG(eeMLX90640, sizeof(uint16_t) * 832, "eeMLX90640: ");

  ret = MLX90640_ExtractParameters(eeMLX90640, &params);
  if (ret) {
    LOG_ERR("Error in MLX90640_ExtractParameters: %d", ret);

    return ret;
  }
  // warning: conversion from 'unsigned int' to 'short unsigned int:12' changes value from '4732' to
  // '636' [-Woverflow] LOG_HEXDUMP_DBG(&params, sizeof(params), "paramsMLX90640: ");

  ret = MLX90640_SetChessMode(MLX90640_I2CADDR);
  if (ret) {
    LOG_ERR("Error in MLX90640_SetChessMode: %d", ret);

    return ret;
  }
  int mode = MLX90640_GetCurMode(MLX90640_I2CADDR);
  LOG_INF("mode = %d", mode);

  ret = MLX90640_SetResolution(MLX90640_I2CADDR, (int)MLX90640_ADC_18BIT);
  if (ret) {
    LOG_ERR("Error in MLX90640_SetResolution: %d", ret);

    return ret;
  }
  int resolution = MLX90640_GetCurResolution(MLX90640_I2CADDR);
  LOG_INF("resolution = %d", resolution);

  ret = MLX90640_SetRefreshRate(MLX90640_I2CADDR, (int)MLX90640_2_HZ);
  if (ret) {
    LOG_ERR("Error in MLX90640_SetRefreshRate: %d", ret);

    return ret;
  }
  int refreshrate = MLX90640_GetRefreshRate(MLX90640_I2CADDR);
  LOG_INF("refreshrate = %d", refreshrate);

  return 0;
}

int init_mlx60940(struct mlx60940_cb* callbacks) {
  k_work_queue_init(&mlx90640_work_q);
  k_work_queue_start(&mlx90640_work_q, mlx90640_stack_area,
                     K_THREAD_STACK_SIZEOF(mlx90640_stack_area), MLX90640_PRIORITY, NULL);

  k_work_init_delayable(&read_mlx90640_work, read_heat_map);
  k_work_reschedule_for_queue(&mlx90640_work_q, &read_mlx90640_work,
                              K_SECONDS(CONFIG_HEAT_MAP_INIT_DELAY));

  if (callbacks) {
    mlx60940_cb.notify_heatMap_cb = callbacks->notify_heatMap_cb;
  }

  return init_mlx60940_internal();
}

void set_inverval_mlx60940(uint16_t interval) {
  heat_map_interval = K_SECONDS(interval);
  k_work_reschedule_for_queue(&mlx90640_work_q, &read_mlx90640_work, K_SECONDS(0));
}
