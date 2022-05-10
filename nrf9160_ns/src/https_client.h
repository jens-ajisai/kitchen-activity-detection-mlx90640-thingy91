#ifndef __HTTPS_CLIENT__
#define __HTTPS_CLIENT__

#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

#ifdef CONFIG_MEMFAULT
void send_memfault_https(const char* hostname, const int port, sec_tag_t* tls_sec_tags, socklen_t tls_sec_tags_len, const char* device_serial, const char* project_key,
uint8_t* data, size_t len);
int cert_provision(sec_tag_t* tls_sec_tags, const char** certs, size_t len);
#endif

#endif
