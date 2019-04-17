#include "driver/rmt.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "colors.h"

void app_main(){
  int i;
  init();

  // Led config
  volatile struct led_config conf;
  conf.length         = 60;
  conf.fadeRate       = 255;
  conf.section_length = 10;
  conf.section_offset = 0;
  conf.section_colors = (malloc(conf.section_length * sizeof(section_colors_t)));
  conf.fade           = 0;
  conf.walk           = 1;
  conf.walk_rate      = 10;
  conf.smooth         = 20;
  conf.fadeWalk       = 0;
  conf.fadeWalkFreq   = 30;
  conf.fadeWalkRate   = 3;
  struct led_struct leds[conf.length];

  for(i = 0; i < conf.section_length; i++){
    conf.section_colors[i] = testColors[i];
  }

  setSectionColors(conf, leds);
  setLeds(leds, conf);

  for(i = 0; i < conf.length; i ++){
    leds[i].fadeR = 0;
    leds[i].fadeG = 0;
    leds[i].fadeB = 0;
  }

  while(1){
    printf("sizeof leds %d\n", sizeof(leds));
    printf("Changing light\n");
    ledEngine(conf, leds);
    vTaskDelay( UPDATE_FREQ_MS / portTICK_PERIOD_MS); 
  }
}
