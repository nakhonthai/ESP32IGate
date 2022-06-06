
#include "PPPoS.h"

extern "C" {

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "tcpip_adapter.h"
#include "netif/ppp/pppos.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "netif/ppp/pppapi.h"

}

#define BUF_SIZE (1024)


/* The PPP control block */
static ppp_pcb *ppp;

/* The PPP IP interface */
static struct netif ppp_netif;

static const char *TAG = "pppos";

static void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    PPPoS* pppos = (PPPoS*)ctx;

    struct netif *pppif = ppp_netif(pcb);

    switch (err_code) {
    case PPPERR_NONE: {
        ESP_LOGE(TAG, "status_cb: Connected\n");
    #if PPP_IPV4_SUPPORT
        ESP_LOGE(TAG, "   our_ipaddr  = %s\n", ipaddr_ntoa(&pppif->ip_addr));
        ESP_LOGE(TAG, "   his_ipaddr  = %s\n", ipaddr_ntoa(&pppif->gw));
        ESP_LOGE(TAG, "   netmask     = %s\n", ipaddr_ntoa(&pppif->netmask));
    #endif /* PPP_IPV4_SUPPORT */
    #if PPP_IPV6_SUPPORT
        ESP_LOGE(TAG, "   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
    #endif /* PPP_IPV6_SUPPORT */
        pppos->mConnected = true;
      
        break;
    }
    case PPPERR_PARAM: {
        ESP_LOGE(TAG, "status_cb: Invalid parameter\n");
        break;
    }
    case PPPERR_OPEN: {
        ESP_LOGE(TAG, "status_cb: Unable to open PPP session\n");
        break;
    }
    case PPPERR_DEVICE: {
        ESP_LOGE(TAG, "status_cb: Invalid I/O device for PPP\n");
        break;
    }
    case PPPERR_ALLOC: {
        ESP_LOGE(TAG, "status_cb: Unable to allocate resources\n");
        break;
    }
    case PPPERR_USER: {
       ESP_LOGE(TAG, "status_cb: User interrupt\n");
       pppos->mStarted = false;
       pppos->mConnected = false;
       break;
    }
    case PPPERR_CONNECT: {
       ESP_LOGE(TAG, "status_cb: Connection lost\n");
       pppos->mStarted = false;
       pppos->mConnected = false;
       break;
    }
    case PPPERR_AUTHFAIL: {
        ESP_LOGE(TAG, "status_cb: Failed authentication challenge\n");
        break;
    }
    case PPPERR_PROTOCOL: {
        ESP_LOGE(TAG, "status_cb: Failed to meet protocol\n");
        break;
    }
    case PPPERR_PEERDEAD: {
        ESP_LOGE(TAG, "status_cb: Connection timeout\n");
        break;
    }
    case PPPERR_IDLETIMEOUT: {
        ESP_LOGE(TAG, "status_cb: Idle Timeout\n");
        break;
    }
    case PPPERR_CONNECTTIME: {
        ESP_LOGE(TAG, "status_cb: Max connect time reached\n");
        break;
    }
    case PPPERR_LOOPBACK: {
        ESP_LOGE(TAG, "status_cb: Loopback detected\n");
        break;
    }
    default: {
        ESP_LOGE(TAG, "status_cb: Unknown error code %d\n", err_code);
        break;
    }
    }
}

static u32_t ppp_output_callback(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    PPPoS* pppos = (PPPoS*)ctx;
    return pppos->mStream->write(data, len);
}


static void pppos_client_task(void *ctx)
{
    PPPoS* pppos = (PPPoS*)ctx;

    u8_t* data =  (u8_t*)malloc(BUF_SIZE);
    while (1) {
        while (pppos->mStarted) {
            //while (pppos->mStream->available()) {
                int len = pppos->mStream->readBytes(data, BUF_SIZE);
                if (len > 0) {
                    pppos_input_tcpip(ppp, data, len);
                }
            //}
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void PPPoS::begin(Stream* stream)
{
    mStream = stream;

    // This is needed, because WiFiClient and WiFiClientSecure are used to handle connections
    WiFi.begin();

    xTaskCreate(&pppos_client_task, "pppos_client_task", 10048, this, 5, NULL);
}

bool PPPoS::waitResponse(String& data)
{
    unsigned timeout_ms = 500;
    data.reserve(64);
    uint8_t  index       = 0;
    uint32_t startMillis = millis();
    do {
      while (mStream->available() > 0) {
        int8_t a = mStream->read();
        if (a <= 0) continue;  // Skip 0x00 bytes, just in case
        data += static_cast<char>(a);
        if (data.endsWith("OK\r\n")) {
          index = 1;
          goto finish;
        } else if (data.endsWith("ERROR\r\n")) {
          index = 2;
          goto finish;
        } else if (data.endsWith("NO CARRIER\r\n")) {
          index = 3;
          goto finish;
        } else if (data.endsWith("CONNECT\r\n")) {
          index = 4;
          goto finish;
        }
      }
    } while (millis() - startMillis < timeout_ms);
  finish:
    if (!index) {
      data = "";
    }
    return index > 0;
  }

bool PPPoS::connect(const char* apn, const char* user, const char* pass)
{
    while (mStream->available()) {
        mStream->read();
    }
    while (1) {
        mStream->print("AT+CGDCONT=1,\"IP\",\"");
        mStream->print(apn);
        mStream->print("\"\r\n");
        mStream->flush();
        String data;
        waitResponse(data);
        data.trim();
        if (data == "OK") break;
        delay(1000);
    }
    while (1) {
        mStream->print("ATD*99***1#\r\n");
        mStream->flush();
        String data;
        waitResponse(data);
        data.trim();
        if (data == "CONNECT") break;
        delay(1000);
    }

    if (!mFirstStart) {
        ppp = pppapi_pppos_create(&ppp_netif, ppp_output_callback, ppp_status_cb, this);

        if (ppp == NULL) {
            return false;
        }

        pppapi_set_default(ppp);
        pppapi_set_auth(ppp, PPPAUTHTYPE_PAP, user, pass);
        ppp_set_usepeerdns(ppp, 1);
    }
    pppapi_connect(ppp, 0);

    mStarted = true;
    mFirstStart = true;

    return true;
}


void PPPoS::end() {
    pppapi_close(ppp, 0);
}
