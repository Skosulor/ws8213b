#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

#define N_CONF     5
#define N_LEDS     60
#define N_SECTIONS 10


void app_main(){

  struct mode_config conf[N_CONF];
  // TODO add to README
  resetModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);

  init(conf);

  conf[0].walk        = false;
  conf[0].fade        = false;
  conf[0].cycleConfig = false;

  // TODO handle fade in cycleConfig
  conf[0].smooth        = 15;
  conf[0].walk_rate     = 30;
  conf[0].debugRate     = 1;
  conf[0].configRate    = 1;  // Cannot be frequency if int
  conf[0].fadeIteration = 63;
  conf[0].fadeRate      = 20;
  conf[0].fadeDir       = 1;

  repeatModeZero(conf);


  conf[1].walk_rate     = 20;
  conf[2].walk_rate     = 10;
  conf[3].walk_rate     = 5;
  conf[4].walk_rate     = 1;

  initColors(&conf, testColors);


  // TODO add to README
  initColors(&conf, testColors);

  // TODO add to README, MIGHT BE REDUDANT???
  setSectionColors(conf[0]);
  /* setLeds(conf[0]); */

  while(1)
    ledEngine(conf);
}

