#ifndef __MY_MEMFAULT_H__
#define __MY_MEMFAULT_H__

#include <zephyr.h>

#ifdef CONFIG_MEMFAULT
size_t retrieve_memfault_data(char** memfaultData);
size_t retrieve_memfault_data_as_base64(char** memfaultData);
#endif

#endif
