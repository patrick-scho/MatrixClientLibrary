#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"
// #include "protocol_examples_utils.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#if !CONFIG_IDF_TARGET_LINUX
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#endif

#include "esp_http_client.h"

/* Root cert for howsmyssl.com, taken from howsmyssl_com_root_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect www.howsmyssl.com:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
// extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
// extern const char howsmyssl_com_root_cert_pem_end[]   asm("_binary_howsmyssl_com_root_cert_pem_end");

// extern const char postman_root_cert_pem_start[] asm("_binary_postman_root_cert_pem_start");
// extern const char postman_root_cert_pem_end[]   asm("_binary_postman_root_cert_pem_end");

#include "matrix.h"

#define HTTP_CONNECTION_DATA_SIZE 1024*64
#define AUTHORIZATION_HEADER_LEN 64

struct MatrixHttpConnection {
    esp_http_client_handle_t client;

    const char * host;
    const char * accessToken;

    // char data[HTTP_CONNECTION_DATA_SIZE];
    char * data;
    int dataCap;
    int dataLen;
};


static const char *TAG = "HTTP_CLIENT";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    vTaskDelay(10/portTICK_PERIOD_MS);

    MatrixHttpConnection * hc = (MatrixHttpConnection *)evt->user_data;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGD(TAG, "Non-Chunked Encoding");
            }
            else {
                ESP_LOGD(TAG, "Chunked Encoding");
            }

            int copy_len = 0;

            // const int64_t buffer_len = esp_http_client_get_content_length(evt->client);
            // if (buffer_len < hc->dataCap) {
            //     ESP_LOGE(TAG, "Output buffer too small: %" PRIu64 ", data_len: %d", buffer_len, evt->data_len);
            //     return ESP_FAIL;
            // }
            copy_len = MIN(evt->data_len, (hc->dataCap - hc->dataLen));
            if (copy_len) {
                memcpy(hc->data + hc->dataLen, evt->data, copy_len);
                hc->data[hc->dataLen + copy_len] = '\0';
            }

            hc->dataLen += copy_len;

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            // esp_http_client_set_header(evt->client, "From", "user@example.com");
            // esp_http_client_set_header(evt->client, "Accept", "text/html");
            // esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

void
MatrixHttpConnect(
    MatrixHttpConnection * hc)
{
    esp_http_client_config_t config = {
        .url = hc->host,
        // .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = hc,
        .disable_auto_redirect = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    hc->client = esp_http_client_init(&config);

    esp_http_client_set_timeout_ms(hc->client, 20000);
}

void
MatrixHttpDisconnect(
    MatrixHttpConnection * hc)
{
    esp_http_client_cleanup(hc->client);
    hc->client = NULL;
}

bool
MatrixHttpInit(
    MatrixHttpConnection ** hc,
    const char * host)
{
    *hc = (MatrixHttpConnection *)calloc(1, sizeof(MatrixHttpConnection));
    
    (*hc)->host = host;

    MatrixHttpConnect(*hc);

    return true;
}

bool
MatrixHttpDeinit(
    MatrixHttpConnection ** hc)
{
    MatrixHttpDisconnect(*hc);

    free(*hc);
    *hc = NULL;

    return true;
}

bool
MatrixHttpSetAccessToken(
    MatrixHttpConnection * hc,
    const char * accessToken)
{
    hc->accessToken = accessToken;

    return true;
}

bool
MatrixHttpGet(
    MatrixHttpConnection * hc,
    const char * url,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Bearer %s", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    printf("GET %s%s\n", hc->host, url);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    hc->dataLen = 0;

    static char hostAndUrl[MAX_URL_LEN];
    snprintf(hostAndUrl, MAX_URL_LEN, "%s%s", hc->host, url);
    
    esp_http_client_set_url(hc->client, hostAndUrl);
    esp_http_client_set_method(hc->client, HTTP_METHOD_GET);
    if (authenticated)
        esp_http_client_set_header(hc->client, "Authorization", authorizationHeader);
    esp_err_t err = esp_http_client_perform(hc->client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(hc->client),
                esp_http_client_get_content_length(hc->client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    // ESP_LOG_BUFFER_HEX(TAG, hc->data, hc->dataLen);

    return true;
}

bool
MatrixHttpPost(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Bearer %s", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    printf("POST %s%s\n%s\n", hc->host, url, requestBuffer);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    hc->dataLen = 0;

    static char hostAndUrl[MAX_URL_LEN];
    snprintf(hostAndUrl, MAX_URL_LEN, "%s%s", hc->host, url);
    
    esp_http_client_set_url(hc->client, hostAndUrl);
    esp_http_client_set_method(hc->client, HTTP_METHOD_POST);
    if (authenticated)
        esp_http_client_set_header(hc->client, "Authorization", authorizationHeader);
    esp_http_client_set_header(hc->client, "Content-Type", "application/json");
    esp_http_client_set_post_field(hc->client, requestBuffer, strlen(requestBuffer));
    esp_err_t err = esp_http_client_perform(hc->client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(hc->client),
                esp_http_client_get_content_length(hc->client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    // ESP_LOG_BUFFER_HEX(TAG, hc->data, hc->dataLen);

    return true;
}

bool
MatrixHttpPut(
    MatrixHttpConnection * hc,
    const char * url,
    const char * requestBuffer,
    char * outResponseBuffer, int outResponseCap,
    bool authenticated)
{
    static char authorizationHeader[AUTHORIZATION_HEADER_LEN];
    if (authenticated)
        snprintf(authorizationHeader, AUTHORIZATION_HEADER_LEN,
            "Bearer %s", hc->accessToken);
    else
        authorizationHeader[0] = '\0';

    printf("PUT %s%s\n%s\n", hc->host, url, requestBuffer);

    hc->data = outResponseBuffer;
    hc->dataCap = outResponseCap;
    hc->dataLen = 0;

    static char hostAndUrl[MAX_URL_LEN];
    snprintf(hostAndUrl, MAX_URL_LEN, "%s%s", hc->host, url);
    
    esp_http_client_set_url(hc->client, hostAndUrl);
    esp_http_client_set_method(hc->client, HTTP_METHOD_PUT);
    if (authenticated)
        esp_http_client_set_header(hc->client, "Authorization", authorizationHeader);
    esp_http_client_set_header(hc->client, "Content-Type", "application/json");
    esp_http_client_set_post_field(hc->client, requestBuffer, strlen(requestBuffer));
    esp_err_t err = esp_http_client_perform(hc->client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(hc->client),
                esp_http_client_get_content_length(hc->client));
    } else {
        ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    }
    // ESP_LOG_BUFFER_HEX(TAG, hc->data, hc->dataLen);

    return true;
}
