#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

#define N_CONF     2
#define N_LEDS     60
#define N_SECTIONS 5

void app_main(){

  volatile struct mode_config conf[N_CONF];
  // TODO add to README
  resetModeConfigs(conf, N_CONF, N_LEDS, N_SECTIONS);
  init(conf);

  conf[0].smooth    = 30;
  conf[0].walk      = 2;
  conf[0].walk_rate = 15;
  conf[0].cycleConfig = 1;
  conf[0].configRate = 250;
  repeatModeZero(conf);

  conf[1].walk_rate = 35;

  initColors(&conf, testColors);


  // TODO add to README
  initColors(&conf, testColors);

  // TODO add to README, MIGHT BE REDUDANT???
  setSectionColors(conf[0]);
  /* setLeds(conf[0]); */

  while(1)
    ledEngine(conf);
}

