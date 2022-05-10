#ifndef _MEMORY_HOOK_H_
#define _MEMORY_HOOK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <zephyr.h>

#include "memory_statistics.h"

void* my_malloc(size_t size, enum memory_hook_statistics_id id);
void my_free(void* ptr);
void* my_realloc(void* ptr, size_t new_size, enum memory_hook_statistics_id id);

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_HOOK_H_ */
