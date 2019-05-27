#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

#define N_CONF     2
#define N_LEDS     120
#define N_SECTIONS 10
#define LED_PIN    4


void app_main(){

  struct mode_config conf[N_CONF];
  // TODO add to README
  resetModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);

  initRmt(conf, LED_PIN);

  conf[0].walk        = FALSE;
  conf[0].fade        = FALSE;
  conf[0].fadeWalk    = FALSE;
  conf[0].cycleConfig = FALSE;
  conf[0].musicMode1  = TRUE;
  conf[0].musicMode2  = FALSE;

  // TODO handle fade in cycleConfig 
  conf[0].musicRate      = 1000;
  conf[0].smooth         = 10;
  conf[0].walkRate       = 30;
  conf[0].debugRate      = 1;
  conf[0].configRate     = 0.1;
  conf[0].fadeIterations = 63;
  conf[0].fadeRate       = 20;
  conf[0].fadeDir        = 1;

  repeatModeZero(conf);

  conf[1].musicMode1  = FALSE;
  conf[1].musicMode1  = TRUE;

  // TODO add to README
  initColors(conf, testColors);

  // TODO add to README, MIGHT BE REDUDANT???
  setSectionColors(conf[0]);

  ledEngine(conf);
  /* while(1) */
}

