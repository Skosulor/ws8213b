#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"
#include "freertos/queue.h"

#define N_CONF     4
#define N_LEDS     120
#define N_SECTIONS 10
#define LED_PIN    4

void app_main(){


  mode_config conf[N_CONF];
  initModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  // Walk mode
  conf[0].walk.on                 = TRUE;
  conf[0].walk.dir                = FALSE;
  conf[0].walk.rate               = 50;
  // Fade mode
  conf[0].fade.on                 = FALSE;
  conf[0].fade.iteration          = 63;
  conf[0].fade.rate               = 40;
  conf[0].fade.dir                = 1;  // NOTE: Dont change
  // FFT/Music mode (requires mic)
  conf[0].music.mode1             = FALSE;
  conf[0].music.mode2             = FALSE;
  conf[0].music.rate              = 1000;
  // Mirror
  conf[0].mirror.on               = FALSE;
  conf[0].mirror.sharedReflection = TRUE;
  conf[0].mirror.mirrors          = 2;
  //Fadewalk | NOTE: does not work as intended but can create some intresting effects
  //         | should be renamed and reworked
  conf[0].fadeWalk.on             = FALSE;
  conf[0].fadeWalk.rate           = 10;
  // Cycle configurations
  conf[0].cycleConfig.on          = TRUE;
  conf[0].cycleConfig.rate        = 0.5; //
  // Gravity mode (lacks a lot in features)
  conf[0].simGrav.on              = FALSE;
  conf[0].simGrav.rate            = 500;
  conf[0].simGrav.ledsPerMeter    = 60;
  conf[0].simGrav.gravity         = 1.00;
  // Other
  conf[0].smooth                  = 50;

  // make all configs a copy of conf[0], this is not neccesary but can be handy
  // if the configs diverts little from each other.
  repeatModeZero(conf);

  // Settings for conf[1]
  conf[1].walk.on          = TRUE;
  conf[1].fade.on          = TRUE;
  conf[1].walk.dir         = TRUE;
  conf[1].mirror.on        = TRUE;
  conf[1].mirror.mirrors   = 4;
  conf[1].cycleConfig.rate = 0.2;

  // Settings for conf[2]
  conf[2].walk.rate        = 50;
  conf[2].fade.on          = TRUE;
  conf[2].mirror.on        = TRUE;
  conf[2].walk.dir         = FALSE;
  conf[2].mirror.mirrors   = 2;
  conf[2].cycleConfig.rate = 0.2;

  // Settings for conf[3]
  conf[3].walk.rate = 100;

  // Settings for conf[4]
  conf[4].fade.rate = 100;
  conf[4].walk.on   = FALSE;



  initColors(conf, &testColors);
  setSectionColors(conf[0]);

  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;

  x = xTaskCreate(ledEngineTask, "engine", 7000, (void *)conf,  2, xHandle);
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);

}

