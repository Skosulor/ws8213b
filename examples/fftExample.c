#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"
#include "freertos/queue.h"

#define N_CONF     2
#define N_LEDS     120
#define N_SECTIONS 10
#define LED_PIN    4

void app_main(){


  mode_config conf[N_CONF];
  initModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  // FFT/Music mode (requires mic)
  conf[0].music.mode1             = TRUE;
  conf[0].music.mode2             = FALSE;
  conf[0].music.rate              = 1000;
  // Mirror
  conf[0].mirror.on               = TRUE;
  conf[0].mirror.sharedReflection = TRUE;
  conf[0].mirror.mirrors          = 1;
  // Cycle configurations
  conf[0].cycleConfig.on          = TRUE;
  conf[0].cycleConfig.rate        = 0.1; //

  // smooths the color transition between sections
  conf[0].smooth = 10;
  
  //   make all configs a copy of conf[0], this is not neccesary but can be handy
  // if the configs diverts little from each other.
  repeatModeZero(conf);

  // Settings for conf[1]
  conf[1].music.mode1             = FALSE;
  conf[1].music.mode2             = TRUE;


  initColors(conf, &testColors);
  setSectionColors(conf[0]);

  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;

  x = xTaskCreate(ledEngineTask, "engine", 7000, (void *)conf,  2, xHandle);
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);

}
