#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! DigiCert Global Root CA. SHA-1/RSA-256. Valid until 2031-10-11.
#define ADAFRUIT_CERTS_DIGICERT_GLOBAL_ROOT_CA                         \
  "-----BEGIN CERTIFICATE-----\n"                                      \
  "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
  "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
  "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
  "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
  "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
  "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
  "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
  "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
  "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
  "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
  "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
  "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
  "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
  "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
  "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
  "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"                 \
  "-----END CERTIFICATE-----\n"

//! DigiCert Global Root G2. SHA-1/RSA-256. Valid until 2038-01-15.
#define ADAFRUIT_CERTS_DIGICERT_GEOTRUST_RSA_CA_2018                   \
  "-----BEGIN CERTIFICATE-----\n"                                      \
  "MIIEizCCA3OgAwIBAgIQBUb+GCP34ZQdo5/OFMRhczANBgkqhkiG9w0BAQsFADBh\n" \
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
  "QTAeFw0xNzExMDYxMjIzNDVaFw0yNzExMDYxMjIzNDVaMF4xCzAJBgNVBAYTAlVT\n" \
  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
  "b20xHTAbBgNVBAMTFEdlb1RydXN0IFJTQSBDQSAyMDE4MIIBIjANBgkqhkiG9w0B\n" \
  "AQEFAAOCAQ8AMIIBCgKCAQEAv4rRY03hGOqHXegWPI9/tr6HFzekDPgxP59FVEAh\n" \
  "150Hm8oDI0q9m+2FAmM/n4W57Cjv8oYi2/hNVEHFtEJ/zzMXAQ6CkFLTxzSkwaEB\n" \
  "2jKgQK0fWeQz/KDDlqxobNPomXOMJhB3y7c/OTLo0lko7geG4gk7hfiqafapa59Y\n" \
  "rXLIW4dmrgjgdPstU0Nigz2PhUwRl9we/FAwuIMIMl5cXMThdSBK66XWdS3cLX18\n" \
  "4ND+fHWhTkAChJrZDVouoKzzNYoq6tZaWmyOLKv23v14RyZ5eqoi6qnmcRID0/i6\n" \
  "U9J5nL1krPYbY7tNjzgC+PBXXcWqJVoMXcUw/iBTGWzpwwIDAQABo4IBQDCCATww\n" \
  "HQYDVR0OBBYEFJBY/7CcdahRVHex7fKjQxY4nmzFMB8GA1UdIwQYMBaAFAPeUDVW\n" \
  "0Uy7ZvCj4hsbw5eyPdFVMA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEF\n" \
  "BQcDAQYIKwYBBQUHAwIwEgYDVR0TAQH/BAgwBgEB/wIBADA0BggrBgEFBQcBAQQo\n" \
  "MCYwJAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBCBgNVHR8E\n" \
  "OzA5MDegNaAzhjFodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vRGlnaUNlcnRHbG9i\n" \
  "YWxSb290Q0EuY3JsMD0GA1UdIAQ2MDQwMgYEVR0gADAqMCgGCCsGAQUFBwIBFhxo\n" \
  "dHRwczovL3d3dy5kaWdpY2VydC5jb20vQ1BTMA0GCSqGSIb3DQEBCwUAA4IBAQAw\n" \
  "8YdVPYQI/C5earp80s3VLOO+AtpdiXft9OlWwJLwKlUtRfccKj8QW/Pp4b7h6QAl\n" \
  "ufejwQMb455OjpIbCZVS+awY/R8pAYsXCnM09GcSVe4ivMswyoCZP/vPEn/LPRhH\n" \
  "hdgUPk8MlD979RGoUWz7qGAwqJChi28uRds3thx+vRZZIbEyZ62No0tJPzsSGSz8\n" \
  "nQ//jP8BIwrzBAUH5WcBAbmvgWfrKcuv+PyGPqRcc4T55TlzrBnzAzZ3oClo9fTv\n" \
  "O9PuiHMKrC6V6mgi0s2sa/gbXlPCD9Z24XUMxJElwIVTDuKB0Q4YMMlnpN/QChJ4\n" \
  "B0AFsQ+DU0NCO+f78Xf7\n"                                             \
  "-----END CERTIFICATE-----\n"

//! Baltimore CyberTrust Root. Valid until 2025-05-12.
#define ADAFRUIT_CERTS_ADAFRUIT                                        \
  "-----BEGIN CERTIFICATE-----\n"                                      \
  "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\n" \
  "-----END CERTIFICATE-----\n"

#define ADAFRUIT_CERTS_PEM                     \
  ADAFRUIT_CERTS_DIGICERT_GLOBAL_ROOT_CA,       \
  ADAFRUIT_CERTS_DIGICERT_GEOTRUST_RSA_CA_2018, \
  ADAFRUIT_CERTS_ADAFRUIT,

typedef enum {
  // arbitrarily high base so as not to conflict with id used for other certs in use by the system
  kAdafruitRootCert_DigicertRootCa = 1001, // the same as kMemfaultRootCert_DigicertRootCa
  kAdafruitRootCert_DigicertGeotrustCa = 1011,
  kAdafruitRootCert_Adafruit = 24,
  // Must be last, used to track number of root certs in use
  kAdafruitRootCert_MaxIndex,
} eAdafruitRootCert;

#ifdef __cplusplus
}
#endif
