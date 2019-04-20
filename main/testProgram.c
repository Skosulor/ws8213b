#include "driver/rmt.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

#define CONF_SIZE 1

void app_main(){

  // Led config
  volatile struct mode_config conf[CONF_SIZE];
  conf[0].length         = 60;
  conf[0].section_length = 10;
  conf[0].section_offset = 0;
  conf[0].section_colors = (malloc(conf[0].section_length * sizeof(section_colors_t)));
  conf[0].fade           = 1;
  conf[0].fadeDir        = 0;
  conf[0].fadeRate       = 255;
  conf[0].fadeIteration  = 110;
  conf[0].walk           = 1;
  conf[0].walk_rate      = 15;
  conf[0].smooth         = 30;
  conf[0].fadeWalk       = 0;
  conf[0].fadeWalkFreq   = 30;
  conf[0].fadeWalkRate   = 3;
  conf[0].configRate     = 0;
  conf[0].cycleConfig    = 0;
  conf[0].nOfConfigs     = CONF_SIZE;
  conf[0].debugRate      = 1000;

  init(conf);
  /* struct led_struct leds[conf[0].length]; */

  /* for(i = 1; i < conf[0].nOfConfigs; i++){ */
  /*   conf[i] = conf[0]; */
  /*   if(i < conf[0].nOfConfigs/2) */
  /*     conf[i].walk_rate = conf[i-1].walk_rate + 1; */
  /*   else */
  /*     conf[i].walk_rate = conf[i-1].walk_rate - 1; */
  /* } */

  initColors(&conf, testColors);

  // TODO add to README
  setSectionColors(conf[0], leds);
  /* setLeds(leds, conf[0]); */


  while(1){
    /* printf("size of mode_conf %d\n", sizeof(conf)); */
    /* printf("sizeof leds %d\n", sizeof(leds)); */
    /* printf("Changing light\n"); */

    ledEngine(conf);
    vTaskDelay( UPDATE_FREQ_MS / portTICK_PERIOD_MS);
  }
}

