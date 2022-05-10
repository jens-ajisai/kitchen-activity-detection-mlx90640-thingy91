/*
 * This code is based on the settings sample which has the below license
 * zephyr/samples/subsys/settings
 * 
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MODULE util_settings

#include <logging/log.h>
#include <zephyr.h>

#include "settings/settings.h"
LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

struct direct_immediate_value {
  size_t len;
  void *dest;
  uint8_t fetched;
};

static int direct_loader_immediate_value(const char *name, size_t len, settings_read_cb read_cb,
                                         void *cb_arg, void *param) {
  const char *next;
  size_t name_len;
  int rc;
  struct direct_immediate_value *one_value = (struct direct_immediate_value *)param;

  name_len = settings_name_next(name, &next);

  if (name_len == 0) {
    if (len == one_value->len) {
      rc = read_cb(cb_arg, one_value->dest, len);
      if (rc >= 0) {
        one_value->fetched = 1;
        return 0;
      }

      LOG_ERR("failed reason:%d", rc);
      return rc;
    }
    return -EINVAL;
  }

  /* other keys aren't served by the calback
   * Return success in order to skip them
   * and keep storage processing.
   */
  return 0;
}

int settings_util_get(const char *name, void *dest, size_t len) {
  int rc;
  struct direct_immediate_value dov;

  dov.fetched = 0;
  dov.len = len;
  dov.dest = dest;

  rc = settings_load_subtree_direct(name, direct_loader_immediate_value, (void *)&dov);
  if (rc == 0) {
    if (!dov.fetched) {
      rc = -ENOENT;
    }
  }

  return rc;
}

int settings_util_set(const char *name, const void *dest, size_t len) {
  return settings_save_one(name, dest, len);
}

int settings_util_del(const char *name) { return settings_delete(name); }

int settings_util_init() {
  int ret;

  ret = settings_subsys_init();
  if (ret) {
    LOG_ERR("settings subsys initialization: fail (err %d)", ret);
    return ret;
  }
  LOG_DBG("settings subsys initialization: OK.");
  return 0;
}

void setCounter(const char *filename, int counter) {
  settings_util_set(filename, (const void *)&counter, sizeof(counter));
  LOG_DBG("setCounter(%s,%d)", log_strdup(filename), counter);
}

int getCounter(const char *filename) {
  int counter = 0;
  settings_util_get(filename, &counter, sizeof(counter));
  LOG_DBG("getCounter(%s) -> %d", log_strdup(filename), counter);
  return counter;
}

void resetCounter(const char *filename) {
  LOG_DBG("resetCounter");
  setCounter(filename, 1);
}

void incrementCounter(const char *filename) {
  LOG_DBG("incrementCounter");
  setCounter(filename, getCounter(filename) + 1);
}
