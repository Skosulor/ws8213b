
/*  */
// old main stack size 3584

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "sdkconfig.h"
#include "colors.h"


void app_main(){

  struct section_colors_t colors[10];

  int i;

  for(i = 0; i < 10; i++){
    colors[i] = testColors[i];
  }

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
  conf.walk_rate      = 25;
  conf.smooth         = 1;
  conf.fadeWalk       = 0;
  conf.fadeWalkFreq   = 30;
  conf.fadeWalkRate   = 3;
  struct led_struct leds[conf.length];

  conf.section_colors = colors;
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

void init(){
  ESP_ERROR_CHECK(rmt_config(&rmt_conf));
  ESP_ERROR_CHECK(rmt_driver_install(rmt_conf.channel, 0, 0));
}

void ledEngine(struct led_config led_conf, struct led_struct *leds){
  static uint16_t  fade_t         = 0;
  static uint16_t  walk_t         = 0;
  static uint16_t  fade_walk_t    = 0;
  static uint8_t   fade_dir       = 0;
  volatile uint8_t fade_iteration = 0;
  const TickType_t freq           = 1;
  TickType_t       LastWakeTime;
  int i;

  if(led_conf.smooth == 1){
    for(i = 0; i < 15; i++){
      fadeWalk(leds, led_conf);
    }
  }


  setLeds(leds, led_conf);
  outputLeds(leds, led_conf);

  while(1){
    LastWakeTime = xTaskGetTickCount();

    if(fade_t >= 5 && led_conf.fade){
      fade_t = 0;
      fade_iteration = fade(led_conf, leds, fade_iteration);
      setLeds(leds, led_conf);
      outputLeds(leds, led_conf);

      if(fade_iteration == 110){
        /* if(fade_iteration == led_conf.fadeRate){ */
        if(fade_dir == 0){
          setSectionFadeColors(led_conf, leds);
          fade_dir = 1;
        }
        else{
          for(i = 0; i < led_conf.length; i ++){
            leds[i].fadeR = 0;
            leds[i].fadeG = 0;
            leds[i].fadeB = 0;
          }
          fade_dir = 0;
        }
        fade_iteration = 0;
      }
    }

    if(walk_t >= led_conf.walk_rate && led_conf.walk){
      walk_t = 0;
      stepForward(leds, &led_conf);
      outputLeds(leds, led_conf);
    }

    if(fade_walk_t >= led_conf.fadeWalkRate && led_conf.fadeWalk){
      fade_walk_t = 0;
      fadeWalk(leds, led_conf);
      setLeds(leds, led_conf);
      outputLeds(leds, led_conf);
    }

    fade_t += 1;
    walk_t += 1;
    fade_walk_t += 1;
    /* vTaskDelay(1 / portTICK_PERIOD_MS); */
    vTaskDelayUntil( &LastWakeTime, freq);

  }
}

void fadeWalk(struct led_struct *leds, struct led_config conf){
  int i;
  for(i = 0; i < conf.length; i++){
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
    leds[i].r = leds[i].r - ((leds[i].r - leds[(i+1)%conf.length].r)/(conf.fadeWalkRate));
    leds[i].g = leds[i].g - ((leds[i].g - leds[(i+1)%conf.length].g)/(conf.fadeWalkRate));
    leds[i].b = leds[i].b - ((leds[i].b - leds[(i+1)%conf.length].b)/(conf.fadeWalkRate));
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
  }
}

void setSectionFadeColors(struct led_config conf, struct led_struct *leds){
  int i,j;
  int offset;
  for(i = 0; i < conf.section_length; i ++){
    for(j = i*(conf.length/conf.section_length); j <  (i+1)*(conf.length/conf.section_length); j ++){

      if(j + conf.section_offset < 0)
        offset = (conf.length + j + conf.section_offset) % conf.length;
      else
        offset = (j + conf.section_offset) % conf.length;

      /* printf("offset: %d, s offset: %d, j: %d\n", offset, conf.section_offset, j); */
      leds[offset].fadeR = conf.section_colors[i].red;
      leds[offset].fadeG = conf.section_colors[i].green;
      leds[offset].fadeB = conf.section_colors[i].blue;
    }
  }

}

void setFadeColorsSection(struct led_config conf, struct led_struct *leds){
  int i,j;
  int offset;
  for(i = 0; i < conf.section_length; i ++){
    for(j = i*(conf.length/conf.section_length); j <  (i+1)*(conf.length/conf.section_length); j ++){

      if(j + conf.section_offset < 0)
        offset = (conf.length + j + conf.section_offset) % conf.length;
      else
        offset = (j + conf.section_offset) % conf.length;

      /* printf("offset: %d, s offset: %d, j: %d\n", offset, conf.section_offset, j); */
      conf.section_colors[offset].red = leds[i].fadeR;
      conf.section_colors[offset].green = leds[i].fadeG;
      conf.section_colors[offset].blue = leds[i].fadeB;
    }
  }
}

int fade(struct led_config conf, struct led_struct *leds, uint8_t it){
  int i;
  for(i = 0; i < conf.length; i++){
    leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(128 - it));
    leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(128 - it));
    leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(128 - it));

    /* leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(conf.fadeRate - it)); */
    /* leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(conf.fadeRate - it)); */
    /* leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(conf.fadeRate - it)); */

  }
  if(conf.fadeRate - it == 0)
    return 1;
  else
    return (it + 1);
}

void setSectionColors(struct led_config conf, struct led_struct *leds){
  int i,j;
  for(i = 0; i < conf.section_length; i ++){
    for(j = i*(conf.length/conf.section_length); j <  (i+1)*(conf.length/conf.section_length); j ++){
      leds[j].r = conf.section_colors[i].red;
      leds[j].g = conf.section_colors[i].green;
      leds[j].b = conf.section_colors[i].blue;
    }
  }
  /* for(i = 0; i < conf.length; i++){ */
  /* printf("R: %d, B: %d, G: %d, i: %d\n", leds[i].r, leds[i].b, leds[i].g, i); */
  /* } */
}

void outputLeds(struct led_struct *leds, struct led_config led_conf){
  int i;
  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[0].item, 24, 1));
  for(i = 0; i < led_conf.length; i ++){
    ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[i].item, 24, 1));
    /* printf("i: %d, R: %d\n", i, leds[i].r); */
  }

  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, setItem, 1, 1));

}

void setLeds(struct led_struct *leds, struct led_config conf){
  int i;
  for(i = 0; i < conf.length; i ++){
    setLed(leds[i].item, leds[i].r, leds[i].b, leds[i].g);
  }

}

void setLed(rmt_item32_t *item, uint8_t red, uint8_t blue, uint8_t green){
  setRed(red, item);
  setBlue(blue, item);
  setGreen(green, item);
}

void setRed(uint8_t brightness, rmt_item32_t *item){
  setBrightness(brightness, 8, 16, item);
}

void setGreen(uint8_t brightness, rmt_item32_t *item){
  setBrightness(brightness, 0, 8, item);
}

void setBlue(uint8_t brightness, rmt_item32_t *item){
  setBrightness(brightness, 16, 24, item);
}

void setBrightness(uint8_t brightness, uint8_t start, uint8_t stop, rmt_item32_t *item){
  uint8_t i;
  uint8_t j = 8;
  for(i = start; i < stop; i++){
    if(((brightness >> --j) & 1) == 1){
      item[i] = *oneItem;
      /* printf("1 "); */

    }
    else{
      item[i] = *zeroItem;
      /* printf("0 "); */
    }

  }
  /* printf("\n"); */
}

void printDuration0(rmt_item32_t *item){
  int i;
  for(i = 0; i < 24; i ++){
    printf("Duration0: %d\n", item[i].duration0);
  }
}

void stepForward(struct led_struct *leds, struct led_config *conf){
  int i;
  struct led_struct temp = leds[0];
  /* printf("led one red = %d\n", leds[0].r); */
  for(i = 0; i < conf->length; i ++){
    leds[i] = leds[((i+1) % conf->length)];
  }
  leds[conf->length - 1] = temp;
  conf->section_offset -= 1;
  if(conf->section_offset <= -(int16_t)conf->length)
    conf->section_offset = 0;
}

void stepFade(struct led_struct *led, struct led_config conf,
              uint8_t rTarget, uint8_t bTarget, uint8_t gTarget){

  led->r = led->r - (((double)led->r-(double)rTarget)/(double)conf.fadeRate);
  led->b = led->b - ((led->b-bTarget)/conf.fadeRate);
  led->g = led->g - ((led->g-gTarget)/conf.fadeRate);

}

void pulse(struct led_struct * leds, struct led_config conf){

}

