#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

// array of found devices
#define MAX_DISCOVERED_DEVICES 1000
esp_bd_addr_t discovered_devices[MAX_DISCOVERED_DEVICES];
int discovered_devices_num = 0;

uint8_t test_payload[] = {
//0x02,0x01,0x1A,
0x03,0x03,0x6F,0xFD,

0x17,0x16,0x6F,0xFD,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

0x80,0xFF,
0x00,0x00
};

// scan parameters
static esp_ble_scan_params_t ble_scan_params = {
		.scan_type              = BLE_SCAN_TYPE_ACTIVE,
		.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
		.scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
		.scan_interval          = 0x50,
		.scan_window            = 0x30
	};

// advertise parameters
static esp_ble_adv_params_t ble_adv_params = {
	.adv_int_min = 0x20,
	.adv_int_max = 0x40,
	.adv_type = ADV_TYPE_NONCONN_IND,
	.own_addr_type  = BLE_ADDR_TYPE_PUBLIC,
	.channel_map = ADV_CHNL_ALL,
	.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// check if the device was already discovered
bool alreadyDiscovered(esp_bd_addr_t address) {
	bool found = false;
	for(int i = 0; i < discovered_devices_num; i++) {
		for(int j = 0; j < ESP_BD_ADDR_LEN; j++)
			found = (discovered_devices[i][j] == address[j]);
		if(found) break;
	}
	return found;
}

// add a new device to the list
void addDevice(esp_bd_addr_t address) {
	discovered_devices_num++;
	if(discovered_devices_num > MAX_DISCOVERED_DEVICES) return;
	for(int i = 0; i < ESP_BD_ADDR_LEN; i++)
		discovered_devices[discovered_devices_num - 1][i] = address[i];
}

// clean devices list
void cleanDevices() {
	discovered_devices_num = 0;
}

void stop_adv_task(void *pvParameter){
  vTaskDelay(10*1000 / portTICK_RATE_MS);
  ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());
	vTaskDelete(NULL);
}

// GAP callback
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
      case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:

  			printf("ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT\n");
  			if(param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) {
  				printf("Scan parameters set, start scanning for 10 seconds\n\n");
  				esp_ble_gap_start_scanning(10);
  			}
  			else printf("Unable to set scan parameters, error code %d\n\n", param->scan_param_cmpl.status);
  			break;

  		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:

  			printf("ESP_GAP_BLE_SCAN_START_COMPLETE_EVT\n");
  			if(param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
  				printf("Scan started\n\n");
  			}
  			else printf("Unable to start scan process, error code %d\n\n", param->scan_start_cmpl.status);
  			break;

  		case ESP_GAP_BLE_SCAN_RESULT_EVT:

				//esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
        switch(param->scan_rst.search_evt) {
            case ESP_GAP_SEARCH_INQ_RES_EVT:
						{
							if(!alreadyDiscovered(param->scan_rst.bda)) {
							  //printf("ESP_GAP_BLE_SCAN_RESULT_EVT\n");
								if(memcmp(test_payload, param->scan_rst.ble_adv, 8)==0){
								 printf("addr = ");
								 for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
									 printf("%02X", param->scan_rst.bda[i]);
									 if(i != ESP_BD_ADDR_LEN -1) printf(":");
								 }
								 printf(" payload = ");
								 for(int i = 0; i < param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len; i++) {
									 printf("%02X", param->scan_rst.ble_adv[i]);
									 if(i != param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len -1) printf(" ");
								 }
								 printf("\n");
								}
							 addDevice(param->scan_rst.bda);
						 }
             break;
            }
            case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            {
							printf("Scan complete - advertise\n\n");
							ESP_ERROR_CHECK(esp_ble_gap_config_adv_data_raw(test_payload, 28));
							xTaskCreate(&stop_adv_task, "stop_adv_task", 2048, NULL, 5, NULL);
              break;
            }
            default:
              break;
        }
        break;

      case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:

				printf("ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT\n\n");
				esp_err_t err;
				if((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
					 printf("Scan stop failed: %s", esp_err_to_name(err));
				}
				else {
					 printf("Stop scan successfully");
				}
        break;

  		case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:

  			printf("ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT\n");
  			esp_ble_gap_start_advertising(&ble_adv_params);
  			break;

  		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:

  			printf("ESP_GAP_BLE_ADV_START_COMPLETE_EVT\n");
  			if(param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
  				printf("Advertising started\n\n");
  			}
  			else printf("Unable to start advertising process, error code %d\n\n", param->scan_start_cmpl.status);
  			break;

			case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
				printf("ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT\n");
				printf("advertise complete - scan\n\n");
				ESP_ERROR_CHECK(esp_bt_controller_disable());
				ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
				ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
				cleanDevices();
				ESP_ERROR_CHECK(esp_ble_gap_start_scanning(10));
				break;

  		default:

  			printf("Event %d unhandled\n\n", event);
  			break;
	}
}

void app_main() {

	printf("BT scan and broadcast\n\n");

	// set components to log only errors
	esp_log_level_set("*", ESP_LOG_ERROR);

	// initialize nvs
	ESP_ERROR_CHECK(nvs_flash_init());
	printf("- NVS init ok\n");

	// release memory reserved for classic BT (not used)
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	printf("- Memory for classic BT released\n");

	// initialize the BT controller with the default config
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
	printf("- BT controller init ok\n");

	// enable the BT controller in BLE mode
  esp_bt_controller_enable(ESP_BT_MODE_BLE);
	printf("- BT controller enabled in BLE mode\n");

	// initialize Bluedroid library
	esp_bluedroid_init();
  esp_bluedroid_enable();
	printf("- Bluedroid initialized and enabled\n");

	// register GAP callback function
	ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_gap_cb));
	printf("- GAP callback registered\n\n");

  // configure scan parameters
	ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
}
