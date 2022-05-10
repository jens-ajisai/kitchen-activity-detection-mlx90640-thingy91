#include "memory_hook.h"
#include "memory_statistics.h"

#include <logging/log.h>
#include <string.h>
#include <sys/slist.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(memory_hook, CONFIG_HEATY_LOG_LEVEL);

#ifdef CONFIG_HEAP_MEMORY_STATISTICS

void* my_malloc(size_t size, enum memory_hook_statistics_id id) {
  void* ptr = k_malloc(size);
  if (!ptr) LOG_ERR("Error out of memory");

  struct memory_stat_list stats = {.id = id, .len = size};
  mapPutStat(ptr, stats);

  return ptr;
}

void my_free(void* ptr) {
  if (!ptr) LOG_ERR("Free of NULL!");

  mapRemoveStat(ptr);

  k_free(ptr);
}

// https://codereview.stackexchange.com/questions/151019/implementing-realloc-in-c
void* my_realloc(void* ptr, size_t new_size, enum memory_hook_statistics_id id) {
  struct memory_stat_list stats = {0};
  mapGetStat(ptr, &stats);
  size_t originalLength = stats.len;

  if (new_size == 0) {
    my_free(ptr);
    return NULL;
  } else if (!ptr) {
    return my_malloc(new_size, id);
  } else if (new_size <= originalLength) {
    return ptr;
  } else {
    void* ptrNew = my_malloc(new_size, id);
    if (ptrNew) {
      memcpy(ptrNew, ptr, originalLength);
      my_free(ptr);
    }
    return ptrNew;
  }
}

// get also the allocations of the event manager
void* event_manager_alloc(size_t size) {
  return my_malloc(size, HEAP_MEMORY_STATISTICS_ID_EVENT_MANAGER);
}
void event_manager_free(void* addr) { my_free(addr); }

#else

size_t getMemoryPeakUsage() { return 0; }
size_t getMapPeakUsage() { return 0; }

#ifdef CONFIG_LODEPNG
#include <stdlib.h> /* allocations */
void* my_malloc(size_t size, enum memory_hook_statistics_id id) {
  ARG_UNUSED(id);
  return malloc(size);
}
void my_free(void* ptr) { free(ptr); }
void* my_realloc(void* ptr, size_t new_size, enum memory_hook_statistics_id id) {
  ARG_UNUSED(id);
  return realloc(ptr, new_size);
}
#else
void* my_malloc(size_t size, enum memory_hook_statistics_id id) {
  ARG_UNUSED(id);
  return k_malloc(size);
}
void my_free(void* ptr) { k_free(ptr); }
void* my_realloc(void* ptr, size_t new_size, enum memory_hook_statistics_id id) {
  ARG_UNUSED(id);
  LOG_ERR("realloc not defined!");
  return NULL;
}
#endif

/*
This will be in one of the next zephyr releases ...
void on_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
  LOG_INF("Memory allocated at %p, size %ld", heap_id, mem, bytes);
}

HEAP_LISTENER_ALLOC_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_alloc);

void on_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
  LOG_INF("Memory freed at %p, size %ld", heap_id, mem, bytes);
}

HEAP_LISTENER_FREE_DEFINE(my_listener, HEAP_ID_LIBC, on_heap_free);
*/

#endif