#include "driver/rmt.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"

void init(){

  rmt_conf.rmt_mode                       = RMT_MODE_TX;
  rmt_conf.channel                        = RMT_CHANNEL_0;
  rmt_conf.gpio_num                       = 4;
  rmt_conf.mem_block_num                  = 1;
  rmt_conf.tx_config.loop_en              = 0;
  rmt_conf.tx_config.carrier_en           = 0;
  rmt_conf.tx_config.idle_output_en       = 1;
  rmt_conf.tx_config.idle_level           = 0;
  rmt_conf.tx_config.carrier_duty_percent = 50;
  rmt_conf.tx_config.carrier_freq_hz      = 10000;
  rmt_conf.tx_config.carrier_level        = 1;
  rmt_conf.clk_div                        = 8;

  ESP_ERROR_CHECK(rmt_config(&rmt_conf));
  ESP_ERROR_CHECK(rmt_driver_install(rmt_conf.channel, 0, 0));
}

void ledEngine(struct led_config *led_conf, struct led_struct *leds){
  volatile uint8_t fade_iteration = 0;
  static uint16_t  fade_walk_t    = 0;
  static uint16_t  fade_t         = 0;
  static uint16_t  walk_t         = 0;
  static uint16_t  config_t       = 0;
  static uint8_t   fade_dir       = 0;
  static uint16_t  debug_t        = 0;
  const TickType_t freq           = UPDATE_FREQ_MS;
  TickType_t       LastWakeTime;
  int              i;
  static uint8_t   cc;          // Current config
  static uint8_t   nc;          // Number of configs

  cc = nc;
  nc = led_conf[0].nOfConfigs;
  debug_t = led_conf[nc].debugRate + 1;


    for(i = 0; i < led_conf[cc].smooth; i++){
      fadeWalk(leds, led_conf[cc]);
    }

  setLeds(leds, led_conf[cc]);
  outputLeds(leds, led_conf[cc]);

  while(1){
    LastWakeTime = xTaskGetTickCount();

    /* ------ */
    /* DEBUG */
    /* ------*/

    if(debug_t > led_conf[cc].debugRate){
      debug_t = 0;
      DPRINT((" current conf: %d\n walk: %d\n fade: %d\n smooth: %d\n config_t %d\n\n",
              cc, led_conf[cc].walk, led_conf[cc].fade, led_conf[cc].smooth, config_t));

      DPRINT((" DebugRate: %d\n nc: %d\n led_conf[cc].nOfConfigs: %d\n\n",
              led_conf[cc].debugRate, nc, led_conf[cc].nOfConfigs));
    }

    /* ------------- */
    /* Config Update */
    /* --------------*/

    if(config_t > led_conf[cc].configRate && led_conf[nc].cycleConfig){
      config_t = 0;
      cc = ((cc + 1) % nc);
    }

    /* --------- */
    /* fade mode */
    /* ----------*/

    if(fade_t >= 5 && led_conf[cc].fade){
      fade_t = 0;
      fade_iteration = fade(led_conf[cc], leds, fade_iteration);
      setLeds(leds, led_conf[cc]);
      outputLeds(leds, led_conf[cc]);

      if(fade_iteration == 110){
        /* if(fade_iteration == led_conf[cc].fadeRate){ */
        if(fade_dir == 0){
          setSectionFadeColors(led_conf[cc], leds);
          fade_dir = 1;
        }
        else{
          for(i = 0; i < led_conf[cc].length; i ++){
            leds[i].fadeR = 0;
            leds[i].fadeG = 0;
            leds[i].fadeB = 0;
          }
          fade_dir = 0;
        }
        fade_iteration = 0;
      }
    }

    /* --------- */
    /* walk mode */
    /* ----------*/

    if(walk_t >= led_conf[cc].walk_rate && led_conf[cc].walk){
      walk_t = 0;
      stepForward(leds, &led_conf[cc]);
      outputLeds(leds, led_conf[cc]);
    }

    /* -------------- */
    /* fade_walk mode */
    /* ---------------*/

    if(fade_walk_t >= led_conf[cc].fadeWalkRate && led_conf[cc].fadeWalk){
      fade_walk_t = 0;
      fadeWalk(leds, led_conf[cc]);
      setLeds(leds, led_conf[cc]);
      outputLeds(leds, led_conf[cc]);
    }

    /* ----------- */
    /* time Update */
    /* ------------*/

    fade_t      += 1;
    walk_t      += 1;
    config_t    += 1;
    debug_t     += 1;
    fade_walk_t += 1;
    /* vTaskDelay(1 / portTICK_PERIOD_MS); */
    vTaskDelayUntil( &LastWakeTime, freq);
  }
}

void initColors(struct led_config *led_conf, struct section_colors_t *color){
  int i;
  for(i = 0; i < led_conf->section_length; i ++){
    led_conf->section_colors[i] = color[i];
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

