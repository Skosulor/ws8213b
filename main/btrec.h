#ifndef __BTREC_H_
#define __BTREC_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "time.h"
#include "sys/time.h"

#define SPP_TAG "TOTAL_RECALL"
#define SPP_SERVER_NAME "THE_BEST_SERVER"
#define EXCAMPLE_DEVICE_NAME "MY_ESP32"

// enum: either callback mode or VFS write/read
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static struct timeval time_old;
// Security level
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_ENCRYPT;
// Master or Slave: slave
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;


// Callback function, arguments defined by esp_spp_cb_t
// Function is called at bluetooth events
// esp_spp_cb_param_t is a union of structures. Can hold e.g. the incoming data
// or status depending on type of event
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
// For authenticating/pairing device
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void start_bt();
void on_bt_recv(uint8_t *data, uint16_t len);
void log_err(const char *str, esp_err_t err);
void print_data(uint8_t *data, uint16_t len);
uint32_t arduino_remote_to_int(uint8_t *data, uint16_t len);
void arduino_remote_decode(uint8_t *dataIn, uint32_t dataOut[], uint16_t len);

#endif // __BTREC_H_
