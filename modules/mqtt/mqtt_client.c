/*
 * This code is based on the nRF9160: Simple MQTT sample which has the below license
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/mqtt_simple/README.html
 * 
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE mqtt_client_module
#include "mqtt_client.h"

#include <logging/log.h>
#include <memfault/core/trace_event.h>
#include <memfault/metrics/metrics.h>
#include <net/mqtt.h>
#include <net/socket.h>
#include <random/rand32.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_MQTT_MODULE_LOG_LEVEL);

static struct mqtt_client_module_cb mqtt_client_module_cb;

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];

/* MQTT Broker details. */
static struct sockaddr_storage broker;

#define MQTT_POLL_STACK_SIZE 4096
#define MQTT_POLL_PRIORITY 5

K_THREAD_STACK_DEFINE(mqtt_poll_stack_area, MQTT_POLL_STACK_SIZE);

struct k_thread mqtt_poll_thread_data;

static struct pollfd fds;

void mqtt_poll(void *arg1, void *unused, void *unused2) {
  (void)unused;
  (void)unused2;
  struct mqtt_client *client = (struct mqtt_client *)arg1;
  while (1) {
    LOG_DBG("mqtt_poll loop enter");
    int err;
    err = poll(&fds, 1, mqtt_keepalive_time_left(client));

    if (err < 0) {
      LOG_ERR("poll: %d", errno);

      break;
    }
    LOG_DBG("mqtt_live call before");
    err = mqtt_live(client);
    LOG_DBG("mqtt_live call after");
    if ((err != 0) && (err != -EAGAIN)) {
      LOG_ERR("ERROR: mqtt_live: %d", err);

      break;
    }

    if ((fds.revents & POLLIN) == POLLIN) {
      err = mqtt_input(client);
      if (err != 0) {
        LOG_ERR("mqtt_input: %d", err);

        break;
      }
    }

    if ((fds.revents & POLLERR) == POLLERR) {
      LOG_ERR("POLLERR");

      break;
    }

    if ((fds.revents & POLLNVAL) == POLLNVAL) {
      LOG_ERR("POLLNVAL");

      break;
    }
    LOG_DBG("mqtt_poll loop exit");
  }
}

/**@brief Function to publish data on the configured topic
 */
int mqtt_client_module_data_publish(struct mqtt_client *client, const char *topic, uint8_t *data,
                                    size_t len) {
  LOG_DBG("data_publish enter, topic:%s", topic);
  struct mqtt_publish_param param;

  param.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE;
  param.message.topic.topic.utf8 = topic;
  param.message.topic.topic.size = strlen(topic);
  param.message.payload.data = data;
  param.message.payload.len = len;
  param.message_id = sys_rand32_get();
  param.dup_flag = 0;
  param.retain_flag = 0;

  LOG_DBG("data_publish exit");
  return mqtt_publish(client, &param);
}

/**@brief Function to subscribe to the configured topic
 */
int mqtt_client_module_subscribe(struct mqtt_client *client, const char *topic) {
  LOG_DBG("subscribe enter");
  struct mqtt_topic subscribe_topic = {.topic = {.utf8 = topic, .size = strlen(topic)},
                                       .qos = MQTT_QOS_1_AT_LEAST_ONCE};

  const struct mqtt_subscription_list subscription_list = {
      .list = &subscribe_topic, .list_count = 1, .message_id = 1234};

  return mqtt_subscribe(client, &subscription_list);
}

static int publish_get_payload(struct mqtt_client *client, size_t length) {
  if (length > sizeof(payload_buf)) {
    return -EMSGSIZE;

  }

  return mqtt_readall_publish_payload(client, payload_buf, length);
}

/**@brief MQTT client event handler
 */
void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt) {
  switch (evt->type) {
    case MQTT_EVT_CONNACK:
      if (mqtt_client_module_cb.mqtt_connected_cb) {
        mqtt_client_module_cb.mqtt_connected_cb(evt->result);
      }
      if (evt->result != 0) {
        LOG_ERR("MQTT connect failed: %d", evt->result);

      } else {
        LOG_INF("MQTT client connected");
      }
      break;

    case MQTT_EVT_DISCONNECT:
      if (mqtt_client_module_cb.mqtt_disconnected_cb) {
        mqtt_client_module_cb.mqtt_disconnected_cb(evt->result);
      }
      LOG_INF("MQTT client disconnected: %d", evt->result);
      break;

    case MQTT_EVT_PUBLISH:
    LOG_INF("MQTT_EVT_PUBLISH %d", evt->result);
      if (evt->result != 0) {
        LOG_ERR("MQTT_EVT_PUBLISH failed: %d", evt->result);

      } else {
        const struct mqtt_publish_param *p = &evt->param.publish;

        if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
          const struct mqtt_puback_param ack = {.message_id = p->message_id};

          /* Send acknowledgment. */
          mqtt_publish_qos1_ack(client, &ack);
        }

        int err = publish_get_payload(client, p->message.payload.len);
        if (mqtt_client_module_cb.mqtt_received_cb) {
          mqtt_client_module_cb.mqtt_received_cb(err, payload_buf, p->message.payload.len);
        }
      }
      break;

    case MQTT_EVT_PUBACK:
      if (evt->result != 0) {
        LOG_ERR("MQTT PUBACK error: %d", evt->result);

        break;
      }

      LOG_DBG("PUBACK packet id: %u", evt->param.puback.message_id);
      break;

    case MQTT_EVT_SUBACK:
      if (evt->result != 0) {
        LOG_ERR("MQTT SUBACK error: %d", evt->result);

        break;
      }

      LOG_DBG("SUBACK packet id: %u", evt->param.suback.message_id);
      break;

    case MQTT_EVT_PINGRESP:
      if (evt->result != 0) {
        LOG_ERR("MQTT PINGRESP error: %d", evt->result);

      }
      break;

    default:
      LOG_WRN("Unhandled MQTT event type: %d", evt->type);
      break;
  }
}

static int fds_init(struct mqtt_client *client) {
  if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
    fds.fd = client->transport.tcp.sock;
  } else {
#if defined(CONFIG_MQTT_LIB_TLS)
    fds.fd = client->transport.tls.sock;
#else
    return -ENOTSUP;
#endif
  }

  fds.events = POLLIN;
  k_thread_create(&mqtt_poll_thread_data, mqtt_poll_stack_area,
                  K_THREAD_STACK_SIZEOF(mqtt_poll_stack_area), mqtt_poll, (void *)client, NULL,
                  NULL, MQTT_POLL_PRIORITY, 0, K_NO_WAIT);

  return 0;
}
int mqtt_client_module_disconnect(struct mqtt_client *client) { return mqtt_disconnect(client); }

int mqtt_client_module_connect(struct mqtt_client *client) {
  int err = mqtt_connect(client);
  if (err != 0) {
    LOG_ERR("mqtt_connect %d", err);

    return err;
  }

  err = fds_init(client);
  if (err != 0) {
    LOG_ERR("fds_init: %d", err);

    return err;
  }

  return 0;
}

/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int broker_init(struct mqtt_client_info *client_info) {
  int err;
  struct addrinfo *addr;
  struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};

  err = getaddrinfo(client_info->brokerHostname, NULL, &hints, &addr);
  if (err) {
    LOG_ERR("getaddrinfo failed: %d", err);

    return -ECHILD;
  }

  /* Look for address of the broker. */
  while (addr != NULL) {
    /* IPv4 Address. */
    if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
      struct sockaddr_in *broker4 = ((struct sockaddr_in *)&broker);
      char ipv4_addr[NET_IPV4_ADDR_LEN];

      broker4->sin_addr.s_addr = ((struct sockaddr_in *)addr->ai_addr)->sin_addr.s_addr;
      broker4->sin_family = AF_INET;
      broker4->sin_port = htons(client_info->brokerPort);

      inet_ntop(AF_INET, &broker4->sin_addr.s_addr, ipv4_addr, sizeof(ipv4_addr));
      LOG_INF("IPv4 Address found %s", log_strdup(ipv4_addr));

      break;
    } else {

      LOG_ERR("ai_addrlen = %u should be %u or %u", (unsigned int)addr->ai_addrlen,
              (unsigned int)sizeof(struct sockaddr_in), (unsigned int)sizeof(struct sockaddr_in6));
    }

    addr = addr->ai_next;
  }

  /* Free the address. */
  freeaddrinfo(addr);

  return err;
}

/**@brief Initialize the MQTT client structure
 */
static int client_init(struct mqtt_client *client, struct mqtt_client_info *client_info) {
  int err;
  static struct mqtt_utf8 password;
  static struct mqtt_utf8 username;

  mqtt_client_init(client);

  err = broker_init(client_info);
  if (err) {
    LOG_ERR("Failed to initialize broker connection");

    return err;
  }

  /* MQTT client configuration */
  client->broker = &broker;
  client->evt_cb = mqtt_evt_handler;
  client->client_id.utf8 = (uint8_t *)client_info->clientId;
  client->client_id.size = strlen(client_info->clientId);
  client->protocol_version = MQTT_VERSION_3_1_1;

  password.utf8 = (uint8_t *)client_info->password;
  password.size = strlen(client_info->password);
  client->password = &password;

  username.utf8 = (uint8_t *)client_info->userName;
  username.size = strlen(client_info->userName);
  client->user_name = &username;

  /* MQTT buffers configuration */
  client->rx_buf = rx_buffer;
  client->rx_buf_size = sizeof(rx_buffer);
  client->tx_buf = tx_buffer;
  client->tx_buf_size = sizeof(tx_buffer);

  /* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
  struct mqtt_sec_config *tls_cfg = &(client->transport).tls.config;

  LOG_INF("TLS enabled");
  client->transport.type = MQTT_TRANSPORT_SECURE;

  tls_cfg->peer_verify = client_info->peerVerify;
  tls_cfg->cipher_count = 0;
  tls_cfg->cipher_list = NULL;
  tls_cfg->sec_tag_count = client_info->tlsSecTagsLen;
  tls_cfg->sec_tag_list = client_info->tlsSecTags;
  tls_cfg->hostname = client_info->brokerHostname;

#if defined(CONFIG_NRF_MODEM_LIB)
  tls_cfg->session_cache = IS_ENABLED(client_info->sessionCaching) ? TLS_SESSION_CACHE_ENABLED
                                                                   : TLS_SESSION_CACHE_DISABLED;
#else
  /* TLS session caching is not supported by the Zephyr network stack */
  tls_cfg->session_cache = TLS_SESSION_CACHE_DISABLED;

#endif

#else
  client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

  return err;
}

int mqtt_client_module_init(struct mqtt_client *client, struct mqtt_client_info *client_info,
                            struct mqtt_client_module_cb *callbacks) {
  int err = client_init(client, client_info);
  if (err) {
    LOG_ERR("client_init: %d", err);

  }

  if (callbacks) {
    mqtt_client_module_cb.mqtt_connected_cb = callbacks->mqtt_connected_cb;
    mqtt_client_module_cb.mqtt_disconnected_cb = callbacks->mqtt_disconnected_cb;
    mqtt_client_module_cb.mqtt_received_cb = callbacks->mqtt_received_cb;
  }

  return err;
}
