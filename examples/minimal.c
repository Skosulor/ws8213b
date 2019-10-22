#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"
#include "freertos/queue.h"

#define N_CONF     1
#define N_LEDS     60
#define N_SECTIONS 10
#define LED_PIN    4

void app_main(){

  // Array of n configs
  mode_config conf[N_CONF];
  //
  //  Inititialize config and RMT
  initModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  // Walk mode i.e. rotate leds
  // at a frequency of 50 Hz
  conf[0].walk.on              = TRUE;
  conf[0].walk.rate            = 50;

  // init the sections to colors
  // and set each led according to their section color
  initColors(conf, &testColors);
  setSectionColors(conf[0]);

  // Create freertos task which runs the led engine
  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;
  x = xTaskCreate(ledEngineTask, "engine", 7000, (void *)conf,  2, xHandle);
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);
}
