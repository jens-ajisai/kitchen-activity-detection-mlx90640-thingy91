#ifndef _UTIL_SETTINGS_H_
#define _UTIL_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <drivers/gpio.h>

#include "settings/settings.h"

void settings_util_init();
int settings_util_get(const char *name, void *dest, size_t len);
int settings_util_set(const char *name, const void *dest, size_t len);
int settings_util_del(const char *name);

void setCounter(const char *filename, int counter);
int getCounter(const char *filename);
void incrementCounter(const char *filename);
void resetCounter(const char *filename);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
