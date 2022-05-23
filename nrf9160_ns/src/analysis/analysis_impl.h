#ifndef _ANALYSIS_IMPL_H_
#define _ANALYSIS_IMPL_H_

#include <zephyr.h>

#define HEATMAP_WIDTH 32
#define HEATMAP_HEIGHT 24

#ifdef __cplusplus
extern "C" {
#endif

struct roi {
  int x;
  int y;
  int h;
  int w;
};

struct stats {
  float mean;
  float min;
  float max;
  float var;
};

void analyze_heatmap(float *heatMap);
struct ring_buf* getHistory_gas_stove();
struct ring_buf* getHistory_microwave_oven();
struct ring_buf* getHistory_dis_washer();
struct ring_buf* getHistory_rice_cooker();
struct ring_buf* getHistory_heater();
#ifdef __cplusplus
}
#endif

#endif
