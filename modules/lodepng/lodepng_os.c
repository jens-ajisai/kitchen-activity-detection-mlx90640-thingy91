#include <string.h>

#include "common/memory_hook.h"

void* lodepng_malloc(size_t size) { return my_malloc(size, HEAP_MEMORY_STATISTICS_ID_LODEPNG); }
void lodepng_free(void* ptr) { my_free(ptr); }
void* lodepng_realloc(void* ptr, size_t new_size) { return my_realloc(ptr, new_size, HEAP_MEMORY_STATISTICS_ID_LODEPNG); }
