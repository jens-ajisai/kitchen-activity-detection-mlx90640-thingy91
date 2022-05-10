#include "diag.h"

#include <logging/log.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>
#include <memfault_platform_config.h>
#include <sys/base64.h>
#include <zephyr.h>

#include "utils.h"
#include "common/memory_hook.h"

LOG_MODULE_REGISTER(diag, CONFIG_DIAG_LOG_LEVEL);

size_t retrieve_memfault_data_as_base64(char** memfaultData_base64) {
  uint8_t memfaultData[MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN];
  size_t buf_len = sizeof(memfaultData);

  bool data_available = memfault_packetizer_get_chunk(memfaultData, &buf_len);
  if (!data_available) {
    return 0;  // no more data to send
  }
  LOG_INF("memfault data available: %d bytes", buf_len);

  *memfaultData_base64 = my_malloc(BASE64_ENCODE_LEN(buf_len) + 1, HEAP_MEMORY_STATISTICS_ID_DIAG_MEMFAULT_BASE64_DATA);
  if (*memfaultData_base64 == NULL) {
    LOG_ERR("Failed to allocate memory for the base64 encoded memfault data!");

    return 0;
  }

  size_t base64_chunk_len = 0;
  int err = base64_encode(*memfaultData_base64, BASE64_ENCODE_LEN(buf_len) + 1, &base64_chunk_len,
                          memfaultData, buf_len);
  if (err) {
    LOG_ERR("error encoding base64: %d", err);


    return 0;
  }

  LOG_INF("memfault data as base64: %d bytes", base64_chunk_len);

  (*memfaultData_base64)[base64_chunk_len + 1] = '\0';

  return base64_chunk_len;
}

size_t retrieve_memfault_data(char** memfaultData) {
  *memfaultData = my_malloc(MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN, HEAP_MEMORY_STATISTICS_ID_DIAG_MEMFAULT_DATA);
  if (*memfaultData == NULL) {
    LOG_ERR("Failed to allocate memory for the memfault data!");


    return 0;
  }

  size_t buf_len = MEMFAULT_DATA_EXPORT_CHUNK_MAX_LEN;

  bool data_available = memfault_packetizer_get_chunk(*memfaultData, &buf_len);
  if (!data_available) {
    return 0;  // no more data to send
  }

  return buf_len;
}

#ifdef CONFIG_MEMFAULT_HTTP_ENABLE
#include "memfault/http/http_client.h"
#include "memfault/nrfconnect_port/http.h"

void some_periodic_task(void) {
  memfault_nrfconnect_port_post_data();
}
#endif
