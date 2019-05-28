#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

#define N_CONF     5
#define N_LEDS     60
#define N_SECTIONS 10
#define LED_PIN    4


void app_main(){


  struct mode_config conf[N_CONF];
  // TODO add to README

  resetModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  initRmt(conf, LED_PIN);

  conf[0].walk        = FALSE;
  conf[0].fade        = TRUE;
  conf[0].fadeWalk    = TRUE; //TODO change the func
  conf[0].cycleConfig = FALSE;
  conf[0].musicMode1  = FALSE;
  conf[0].musicMode2  = FALSE;

  // TODO handle fade in cycleConfig 
  conf[0].musicRate      = 1000;
  conf[0].smooth         = 10;
  conf[0].fadeWalkRate   = 0; //TODO change the func
  conf[0].walkRate       = 30;
  conf[0].debugRate      = 1;
  conf[0].configRate     = 0.5;
  conf[0].fadeIterations = 63;
  conf[0].fadeRate       = 40;
  conf[0].fadeDir        = 1;

  repeatModeZero(conf);

  conf[1].walk        = TRUE;
  conf[1].fade        = FALSE;
  conf[1].fadeWalk    = FALSE;
  conf[1].musicMode1  = FALSE;
  conf[1].musicMode2  = FALSE;

  conf[2].walk        = FALSE;
  conf[2].fade        = FALSE;
  conf[2].fadeWalk    = FALSE;
  conf[2].musicMode1  = TRUE;
  conf[2].musicMode2  = FALSE;

  conf[3].walk        = FALSE;
  conf[3].fade        = FALSE;
  conf[3].fadeWalk    = FALSE;
  conf[3].musicMode1  = FALSE;
  conf[3].musicMode2  = TRUE;

  conf[4].walk        = TRUE;
  conf[4].fade        = TRUE;
  conf[4].fadeWalk    = FALSE;
  conf[4].musicMode1  = FALSE;
  conf[4].musicMode2  = FALSE;


  // TODO add to README // TODO rename and ability to choose specific config
  initColors(conf, testColors);

  // TODO add to README, MIGHT BE REDUDANT???
  setSectionColors(conf[0]);

  TaskHandle_t xHandle = NULL;
  portBASE_TYPE x;

  x = xTaskCreate(ledEngineTest, "engine", 7000, (void *)conf,  2, xHandle);
  printf("%d\n", x);
  vTaskDelay(1000);
  vTaskDelete(NULL);

}

