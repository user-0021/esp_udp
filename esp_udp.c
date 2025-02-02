#include "esp_udp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <string.h>

static const char *TAG = "wifi softAP";

void udp_event_handler(void* arg, esp_event_base_t event_base,
									int32_t event_id, void* event_data)
{
	switch (event_id)
	{
		static int diss_connect_count = 0;
		case WIFI_EVENT_STA_START:{
			esp_err_t err = esp_wifi_connect();

			if(err)
				ESP_ERROR_CHECK_WITHOUT_ABORT(err);
			else
				ESP_LOGI(TAG,"Wifi client started.");
		}
			break;
		case WIFI_EVENT_STA_CONNECTED:{
			diss_connect_count = 0;
			ESP_LOGI(TAG,"Wifi client connected.");
		}
			break;
		case WIFI_EVENT_STA_DISCONNECTED:{
			if(diss_connect_count > 100){
				ESP_LOGI(TAG,"AP not found.");
				esp_deep_sleep(1000 * 1000 * 300);
			}

			esp_err_t err = esp_wifi_connect();
			diss_connect_count++;

			if(err)
				ESP_ERROR_CHECK_WITHOUT_ABORT(err);
		}
			break;

		case IP_EVENT_STA_GOT_IP:{
			ip_event_got_ip_t* event_arg = event_data;
			ESP_LOGI(TAG,"Ip got : %d,%d,%d,%d"
				,(int)(event_arg->ip_info.ip.addr >> 24) & 0xFF
				,(int)(event_arg->ip_info.ip.addr >> 16) & 0xFF
				,(int)(event_arg->ip_info.ip.addr >>  8) & 0xFF
				,(int)(event_arg->ip_info.ip.addr >>  0) & 0xFF);
		}
			break;
		
		default:
			break;
	}

	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
				 MAC2STR(event->mac), event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
				 MAC2STR(event->mac), event->aid, event->reason);
	}
}

void esp_udp_init(const char* ssid,const char* pass)
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&udp_event_handler,
														NULL,
														NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = {},
			.password = {},
			.scan_method = WIFI_FAST_SCAN,
			.channel = 0,
			.bssid_set = 0,
			.threshold = {
				.authmode = WIFI_AUTH_WPA2_PSK,
				.rssi = 0
			}
		}
	};

	if (strlen(ssid) < 32)
		memcpy(wifi_config.sta.ssid,ssid,strlen(ssid));
	else
		ESP_LOGE(TAG,"%s() SSID:%s is too long",__func__,ssid);

	if (strlen(pass) < 64)
		memcpy(wifi_config.sta.password,pass,strlen(pass));
	else
		ESP_LOGE(TAG,"%s() PASSWORD:%s is too long",__func__,pass);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG,"%s() finished. SSID:%s password:%s",__func__,wifi_config.sta.ssid,wifi_config.sta.password);
}
