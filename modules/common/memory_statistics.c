#include <logging/log.h>
#include <string.h>
#include <sys/slist.h>
#include <zephyr.h>

#include "memory_statistics.h"

LOG_MODULE_REGISTER(memory_statistics, CONFIG_MEMORY_STAT_LOG_LEVEL);

#ifdef CONFIG_HEAP_MEMORY_STATISTICS

static size_t inUse = 0;
size_t inUsePeak = 0;

static sys_slist_t memory_stats = SYS_SLIST_STATIC_INIT(&memory_stats);
static K_MUTEX_DEFINE(memory_stats_lock);

size_t getMemoryPeakUsage() { return inUsePeak; }

char* get_memory_stat_str(enum memory_hook_statistics_id id) {
  switch (id) {
    case HEAP_MEMORY_STATISTICS_ID_ANALYSIS_VAL_STRING:
      return "ANALYSIS_VAL_STRING";
    case HEAP_MEMORY_STATISTICS_ID_CJSON:
      return "CJSON";
    case HEAP_MEMORY_STATISTICS_ID_CJSON_PREALLOC_STRING:
      return "CJSON_PREALLOC_STRING";
    case HEAP_MEMORY_STATISTICS_ID_COMM_CTRL_DATA_TO_NRF9160:
      return "COMM_CTRL_DATA_TO_NRF9160";
    case HEAP_MEMORY_STATISTICS_ID_COMM_CTRL_HEADER_STRING:
      return "COMM_CTRL_HEADER_STRING";
    case HEAP_MEMORY_STATISTICS_ID_COMM_CTRL_HEATMAP_DATA:
      return "COMM_CTRL_HEATMAP_DATA";
    case HEAP_MEMORY_STATISTICS_ID_COMM_CTRL_NULL_STRING:
      return "COMM_CTRL_NULL_STRING";
    case HEAP_MEMORY_STATISTICS_ID_DIAG_MEMFAULT_BASE64_DATA:
      return "DIAG_MEMFAULT_BASE64_DATA";
    case HEAP_MEMORY_STATISTICS_ID_DIAG_MEMFAULT_DATA:
      return "DIAG_MEMFAULT_DATA";
    case HEAP_MEMORY_STATISTICS_ID_HTTPS_CLIENT_HEADER:
      return "HTTPS_CLIENT_HEADER";
    case HEAP_MEMORY_STATISTICS_ID_LODEPNG:
      return "LODEPNG";
    case HEAP_MEMORY_STATISTICS_ID_MCU_EXCHANGE_UART_RX:
      return "MCU_EXCHANGE_UART_RX";
    case HEAP_MEMORY_STATISTICS_ID_SENSOR_HEATMAP_DATA:
      return "SENSOR_HEATMAP_DATA";
    case HEAP_MEMORY_STATISTICS_ID_SHELL_HEATMAP_DATA:
      return "SHELL_HEATMAP_DATA";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_COMPRESS_DATA:
      return "UTILS_COMPRESS_DATA";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_DECODED_BASE64:
      return "UTILS_DECODED_BASE64";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_DYN_DATA:
      return "UTILS_DYN_DATA";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_ENCODE_BASE64_MESSAGE:
      return "UTILS_ENCODE_BASE64_MESSAGE";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_ENCODE_MESSAGE:
      return "UTILS_ENCODE_MESSAGE";
    case HEAP_MEMORY_STATISTICS_ID_UTILS_HEATMAP_DATA:
      return "UTILS_HEATMAP_DATA";
    case HEAP_MEMORY_STATISTICS_ID_EVENT_MANAGER:
      return "EVENT_MANAGER";
    case HEAP_MEMORY_STATISTICS_ID_MSG_DYNDATA:
      return "MSG_DYNDATA";
    case HEAP_MEMORY_STATISTICS_ID_FS_READ_BUFFER:
      return "FS_READ_BUFFER";
    case HEAP_MEMORY_STATISTICS_ID_BLE_DATA:
      return "HEAP_MEMORY_STATISTICS_ID_BLE_DATA";
    default:
      return "Unknown id";
  }
}

uint16_t mapGetSize() {
  uint16_t size = 0;
  sys_snode_t* node = NULL;

  k_mutex_lock(&memory_stats_lock, K_FOREVER);
  SYS_SLIST_FOR_EACH_NODE(&memory_stats, node) { size++; }
  k_mutex_unlock(&memory_stats_lock);
  return size;
}

void mapPutStat(void* key, struct memory_stat_list stats) {
  struct memory_stat_list* statsPtr = k_malloc(sizeof(struct memory_stat_list));
  memcpy(statsPtr, &stats, sizeof(struct memory_stat_list));
  statsPtr->key = key;

  inUse += stats.len;
  inUsePeak = MAX(inUsePeak, inUse);
  LOG_DBG("+%d -> %d (%s)", stats.len, inUse, get_memory_stat_str(stats.id));    

  k_mutex_lock(&memory_stats_lock, K_FOREVER);
  sys_slist_append(&memory_stats, &statsPtr->node);
  k_mutex_unlock(&memory_stats_lock);
}

void mapGetStat(void* key, struct memory_stat_list* stats) {
  struct memory_stat_list* statsInList = NULL;
  
  k_mutex_lock(&memory_stats_lock, K_FOREVER);
  SYS_SLIST_FOR_EACH_CONTAINER(&memory_stats, statsInList, node) {
    if (statsInList->key == key) {
      memcpy(stats, statsInList, sizeof(struct memory_stat_list));
      k_mutex_unlock(&memory_stats_lock);
      return;
    }
  }
  k_mutex_unlock(&memory_stats_lock);
  memset(stats, 0, sizeof(struct memory_stat_list));
  LOG_WRN("Warn key not in map. Realloc on new allocation?");
  return;
}

void mapRemoveStat(void* key) {
  struct memory_stat_list* stats = NULL;
  struct memory_stat_list* save_stats = NULL;

  k_mutex_lock(&memory_stats_lock, K_FOREVER);
  SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&memory_stats, stats, save_stats, node) {
    if (stats->key == key) {
      inUse -= stats->len;
      inUsePeak = MAX(inUsePeak, inUse);
      LOG_DBG("-%d -> %d (%s)", stats->len, inUse, get_memory_stat_str(stats->id));          
      sys_slist_find_and_remove(&memory_stats, &stats->node);
      k_free(stats);
      k_mutex_unlock(&memory_stats_lock);
      return;
    }
  }
  k_mutex_unlock(&memory_stats_lock);
  LOG_ERR("Double free or statistics map full? No entry found for %p!", key);
  return;
}


uint16_t getMemoryStats(struct memory_stat_list** result_stats) {
  uint16_t size = mapGetSize();
  uint16_t count = 0;
  *result_stats = k_malloc(size * sizeof(struct memory_stat_list));
  
  struct memory_stat_list* stats= NULL;
  
  k_mutex_lock(&memory_stats_lock, K_FOREVER);
  SYS_SLIST_FOR_EACH_CONTAINER(&memory_stats, stats, node) {
    memcpy(*result_stats + count, stats, sizeof(struct memory_stat_list));
    count++;
  }
  k_mutex_unlock(&memory_stats_lock);
  return size;
}

#endif