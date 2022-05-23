#include "analysis_impl.h"

// region of interest
static struct roi gas_stove = {3, 5, 4, 4};
static struct roi microwave_oven = {3, 5, 4, 4};
static struct roi dis_washer = {3, 5, 4, 4};
static struct roi rice_cooker = {3, 5, 4, 4};
static struct roi heater = {3, 5, 4, 4};

RING_BUF_DECLARE(gas_stove_stats_history, HISTORY_SIZE * sizeof(struct stats));
RING_BUF_DECLARE(microwave_oven_stats_history, HISTORY_SIZE * sizeof(struct stats));
RING_BUF_DECLARE(dis_washer_stats_history, HISTORY_SIZE * sizeof(struct stats));
RING_BUF_DECLARE(rice_cooker_stats_history, HISTORY_SIZE * sizeof(struct stats));
RING_BUF_DECLARE(heater_stats_history, HISTORY_SIZE * sizeof(struct stats));

struct ring_buf *getHistory_gas_stove() {
  return gas_stove_stats_history;
}
struct ring_buf *getHistory_microwave_oven() {
  return microwave_oven_stats_history;
}
struct ring_buf *getHistory_dis_washer() {
  return dis_washer_stats_history;
}
struct ring_buf *getHistory_rice_cooker() {
  return rice_cooker_stats_history;
}
struct ring_buf *getHistory_heater() {
  return heater_stats_history;
}

static float *get_roi(struct roi *thisRoi, float *heatMap) {
  int counter = 0;
  float *out =
      my_malloc(thisRoi->w * thisRoi->h * sizeof(float), HEAP_MEMORY_STATISTICS_ID_ANALYSIS_ROI);

  for (int row = thisRoi->y; row < thisRoi->y + thisRoi->h; ++row) {
    for (int col = thisRoi->x; col < thisRoi->x + thisRoi->w; ++col) {
      out[counter++] = heatMap[row * HEATMAP_WIDTH + col];
    }
  }
  return out;
}

static struct stats *get_stats(float *area, uint16_t entriesCount) {
  struct stats *thisStats =
      my_malloc(sizeof(struct stats), HEAP_MEMORY_STATISTICS_ID_ANALYSIS_STATS);
  uint32_t index;
  arm_max_f32(area, entriesCount, &thisStats->max, &index);
  arm_min_f32(area, entriesCount, &thisStats->min, &index);
  arm_mean_f32(area, entriesCount, &thisStats->mean);
  arm_var_f32(area, entriesCount, &thisStats->var);
  return thisStats;
}

// TODO decide on the data structure and pass as parameter for re-use
static void appendToHistory(struct ring_buf *history, struct stats *stat) {
  if (ring_buf_space_get(history) == 0) {
    uint8_t *unused;
    ring_buf_get_claim(history, &unused, sizeof(struct stats));
    ring_buf_get_finish(history, sizeof(struct stats));
  }
  ring_buf_put(history, (uint8_t *)stat, sizeof(struct stats));
}

void analyze_roi(float *heatMap, struct roi *thisRoi, struct ring_buf *thisHistory) {
  float *roi_area = get_roi(thisRoi, heatMap);
  struct stats *roi_stats = get_stats(roi_area, thisRoi->w * thisRoi->h);
  appendToHistory(thisHistory, roi_stats);
  my_free(roi_area);
  my_free(roi_stats);
}

void analyze_heatmap(float *heatMap) {
  analyze_roi(heatMap, &gas_stove, &gas_stove_stats_history);
  analyze_roi(heatMap, &microwave_oven, &microwave_oven_stats_history);
  analyze_roi(heatMap, &dis_washer, &dis_washer_stats_history);
  analyze_roi(heatMap, &rice_cooker, &rice_cooker_stats_history);
  analyze_roi(heatMap, &heater, &heater_stats_history);
}
