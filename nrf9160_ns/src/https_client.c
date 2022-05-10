/*
 * This code is based on the nRF9160: HTTPS Client sample which has the below license
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/nrf9160/https_client/README.html
 * 
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define MODULE https_client

#include <logging/log.h>
#include <modem/modem_key_mgmt.h>
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <net/socket.h>
#pragma GCC diagnostic pop    
#include <net/tls_credentials.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_HEATY_LOG_LEVEL);

#include <modem/lte_lc.h>

#include "https_client.h"
#include "memfault/http/root_certs.h"
#include "memfault/ports/zephyr/http.h"
#include "common/memory_hook.h"

// Setup TLS options on a given socket
// provisioning is expected to be done on startup within memfault_zephyr_port_install_root_certs
int tls_setup(int fd, const char* hostname, sec_tag_t* tls_sec_tag, socklen_t tls_sec_tag_len) {
  int err;
  int verify;

  /* Set up TLS peer verification */
  enum {
    NONE = 0,
    OPTIONAL = 1,
    REQUIRED = 2,
  };

  verify = REQUIRED;

  err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
  if (err) {
    LOG_DBG("Failed to setup peer verification, err %d", errno);
    return err;
  }

  /* Associate the socket with the security tag
   * we have provisioned the certificate with.
   */
  err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag, tls_sec_tag_len);
  if (err) {
    LOG_DBG("Failed to setup TLS sec tag, err %d", errno);
    return err;
  }

  err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, hostname, strlen(hostname) + 1);
  if (err) {
    LOG_DBG("Failed to setup TLS hostname, err %d", errno);
    return err;
  }
  return 0;
}

#define HEADER_BUF_SIZE 300

const char* createHeader(const char* device_serial, const char* hostname, const char* project_key,
                         size_t len) {
  uint8_t* buffer = my_malloc(HEADER_BUF_SIZE, HEAP_MEMORY_STATISTICS_ID_HTTPS_CLIENT_HEADER);
  size_t msg_len = (size_t)snprintf(buffer, HEADER_BUF_SIZE, "POST /api/v0/chunks/%s HTTP/1.1\r\n",
                                    device_serial);
  msg_len += (size_t)snprintf(buffer + msg_len, HEADER_BUF_SIZE - msg_len, "Host:%s\r\n", hostname);
  msg_len +=
      (size_t)snprintf(buffer + msg_len, HEADER_BUF_SIZE - msg_len,
                       "User-Agent: MemfaultSDK/0.4.2\r\nMemfault-Project-Key:%s\r\n", project_key);
  msg_len += (size_t)snprintf(buffer + msg_len, HEADER_BUF_SIZE - msg_len,
                              "Content-Type:application/octet-stream\r\n");
  msg_len += (size_t)snprintf(buffer + msg_len, HEADER_BUF_SIZE - msg_len,
                              "Content-Length:%d\r\n\r\n", len);
  return buffer;
}

// send_memfault_https("chunks.memfault.com", )
#define HTTPS_PORT 443

int sendData(int fd, const char* data, size_t len) {
  int bytes;
  size_t off = 0;
  do {
    bytes = send(fd, &data[off], len - off, 0);
    if (bytes < 0) {
      LOG_ERR("send() failed, err %d", errno);
      return -1;
    }
    off += bytes;
  } while (off < len);
  return off;
}

int connectToHttps(const char* hostname, sec_tag_t* tls_sec_tags, socklen_t tls_sec_tags_len) {
  int err;
  int fd;
  struct addrinfo* res;
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
  };

  //  char port[10] = { 0 };
  //  snprintf(port, sizeof(port), "%d", port_num);
  //  err = getaddrinfo(hostname, port, &hints, &res);

  err = getaddrinfo(hostname, NULL, &hints, &res);
  if (err) {
    LOG_ERR("getaddrinfo() failed, err %d", errno);
    goto clean_up;
  }

  ((struct sockaddr_in*)res->ai_addr)->sin_port = htons(HTTPS_PORT);

  // nonsecure would be IPPROTO_TCP
  fd = socket(res->ai_family, res->ai_socktype, IPPROTO_TLS_1_2);

  if (fd < 0) {
    LOG_ERR("Failed to open socket!");
    goto clean_up;
  }

  /* Setup TLS socket options */
  err = tls_setup(fd, hostname, tls_sec_tags, tls_sec_tags_len);
  if (err) {
    goto clean_up;
  }

  LOG_DBG("Connecting to %s", hostname);
  err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
  if (err) {
    LOG_ERR("connect() failed, err: %d", errno);
    goto clean_up;
  }

  return 0;

clean_up:
  freeaddrinfo(res);
  return -1;
}

#define RECV_BUF_SIZE 2048
static char recv_buf[RECV_BUF_SIZE];

int handleResponse(int fd) {
  int bytes;
  size_t off = 0;
  do {
    bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 0);
    if (bytes < 0) {
      LOG_ERR("recv() failed, err %d", errno);
      return -1;
    }
    off += bytes;
  } while (bytes != 0 /* peer closed connection */);

  LOG_DBG("Received %d bytes", off);

  /* Print HTTP response */
  char* p = strstr(recv_buf, "");
  if (p) {
    off = p - recv_buf;
    recv_buf[off + 1] = '\0';
    LOG_DBG("%s", recv_buf);
  }

  return 0;
}

int cert_provision(sec_tag_t* tls_sec_tags, const char** certs, size_t len) {
  int err;
  bool exists;
  int mismatch;

#pragma GCC diagnostic ignored "-Wsign-compare"        
for (sec_tag_t cert_id_idx = 0; cert_id_idx < len; cert_id_idx++) {
#pragma GCC diagnostic pop     
    sec_tag_t cert_id = tls_sec_tags[cert_id_idx];
    const char* cert = certs[cert_id_idx];

    err = modem_key_mgmt_exists(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
    if (err) {
      LOG_DBG("Failed to check for certificates err %d", err);
      return err;
    }

    if (exists) {
      mismatch = modem_key_mgmt_cmp(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, strlen(cert));
      if (!mismatch) {
        LOG_DBG("Certificate match");
        continue;
      }

      LOG_DBG("Certificate mismatch");
      err = modem_key_mgmt_delete(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
      if (err) {
        LOG_DBG("Failed to delete existing certificate, err %d", err);
        return err;
      }
    }

    LOG_DBG("Provisioning certificate");

    /*  Provision certificate to the modem */
    err = modem_key_mgmt_write(cert_id, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, cert, strlen(cert));
    if (err) {
      LOG_DBG("Failed to provision certificate, err %d", err);
      return err;
    }
  }
  return 0;
}

void send_memfault_https(const char* hostname, const int port, sec_tag_t* tls_sec_tags,
                         socklen_t tls_sec_tags_len, const char* device_serial,
                         const char* project_key, uint8_t* data, size_t len) {
  int fd;

  LOG_DBG("Waiting for network.. ");
  int err = lte_lc_init_and_connect();
  if (err) {
    LOG_ERR("Failed to connect to the LTE network, err %d", err);
    return;
  }

  const char* header = createHeader(device_serial, hostname, project_key, len);
  size_t headerLength = strlen(header);

  LOG_DBG("header: %s.", header);

  // TODO check why the sizeof did not work here
  fd = connectToHttps(hostname, tls_sec_tags, tls_sec_tags_len);
  if (fd < 0) goto close_socket;

  int sentBytes = sendData(fd, header, headerLength);
  if (sentBytes < 0) goto close_socket;
  LOG_DBG("Sent %d bytes", sentBytes);

  sentBytes = sendData(fd, data, len);
  if (sentBytes < 0) goto close_socket;
  LOG_DBG("Sent %d bytes", sentBytes);

  handleResponse(fd);

  LOG_DBG("Finished send_memfault_https, closing socket.");

close_socket:
  (void)close(fd);
}
