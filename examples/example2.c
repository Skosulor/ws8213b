#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"
#include "freertos/queue.h"

#define N_CONF     1
#define N_LEDS     120
#define N_SECTIONS 10
#define LED_PIN    4

// The task which will edit the config during execution
void exampleTask(void *param);

void app_main(){

  // Init
  mode_config conf[N_CONF];
  initModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  // Walk mode
  conf[0].walk.on   = TRUE;
  conf[0].walk.rate = 10;

  // smoothing
  conf[0].smooth = 10;

  // Init colors
  initColors(conf, &testColors);
  setSectionColors(conf[0]);

  // freertos task
  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;

  x = xTaskCreate(ledEngineTask, "engine", 7000, (void *)conf,  2, xHandle);
  x = xTaskCreate(exampleTask, "test", 5000, NULL,  3, xHandle);
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);

}


// About every two seconds the example task will change the rate at which the
// leds rotate
void exampleTask(void *param){
  mode_config *config;
  while(1){
    vTaskDelay(2000);

    // Send a request for the config file. When recieved the "led Engine" will
    // pause until an acknowledgement is sent back
    config              = requestConfig();
    config[0].walk.rate = 20;
    // send acknowledgement to start the ledengine
    sendAck();

    vTaskDelay(2000);
    config              = requestConfig();
    config[0].walk.rate = 40;
    sendAck();

    vTaskDelay(2000);
    config              = requestConfig();
    config[0].walk.rate = 80;
    sendAck();

    vTaskDelay(2000);
    config              = requestConfig();
    config[0].walk.rate = 160;
    sendAck();

    vTaskDelay(2000);
    config              = requestConfig();
    config[0].walk.rate = 320;
    sendAck();
  }
  vTaskDelete(NULL);
}
