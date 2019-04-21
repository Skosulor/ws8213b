#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"


void init(struct mode_config *mode_conf){

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

  leds = malloc(sizeof(led_struct) * mode_conf[0].length);

}

void ledEngine(struct mode_config  *mode_conf){
  static uint16_t  fade_walk_t    = 0;
  static uint16_t  fade_t         = 0;
  static uint16_t  walk_t         = 0;
  static uint16_t  config_t       = 0;
  static uint16_t  debug_t        = 0;
  const TickType_t freq           = UPDATE_FREQ_MS;
  TickType_t       LastWakeTime;
  int              i;
  static uint8_t   cc;          // Current config
  static uint8_t   pc;          // previous config
  static uint8_t   nc;          // Number of configs
  static TickType_t fade_tick;
  static TickType_t walk_tick;
  static TickType_t config_tick;

  // TODO use tickcount to determine execution of mode 

  cc = nc;
  pc = nc;
  nc = mode_conf[0].nOfConfigs;
  debug_t = mode_conf[0].debugRate + 1;


    for(i = 0; i < mode_conf[cc].smooth; i++){
      fadeWalk(mode_conf[cc]);
    }

  for(i = 0; i < mode_conf[0].length; i ++){
    leds[i].fadeR = leds[i].r;
    leds[i].fadeG = leds[i].g;
    leds[i].fadeB = leds[i].b;
  }

  setLeds(mode_conf[cc]);
  outputLeds(mode_conf[cc]);

  walk_tick = xTaskGetTickCount;
  fade_tick = walk_tick;
  config_tick = walk_tick;
  while(1){
    LastWakeTime = xTaskGetTickCount();

    /* ------ */
    /* DEBUG */
    /* ------*/

    /* if(LastWakeTime - walk_tick >= 50){ */
    /*   printf("walk_tick: %d\n", walk_tick); */
    /*   walk_tick = LastWakeTime; */
    /* } */

    if(debug_t > mode_conf[0].debugRate){
      debug_t = 0;
      DPRINT((" current conf: %d\n walk: %d\n fade: %d\n smooth: %d\n config_t %d\n\n",
              cc, mode_conf[cc].walk, mode_conf[cc].fade, mode_conf[cc].smooth, config_t));

      FADE_DEBUG(("----------------------  \n Fade info\n---------------------- \n "));
      FADE_DEBUG(("Current conf: %d\n Fade: %d\n fadeRate: %d\n ", cc, mode_conf[cc].fade, mode_conf[cc].fadeRate));
      FADE_DEBUG(("fadeIteration: %d\n FadeDir: %d\n ", mode_conf[cc].fadeIteration, mode_conf[cc].fadeDir));

      /* DPRINT((" DebugRate: %d\n nc: %d\n mode_conf[cc].nOfConfigs: %d\n\n", */
              /* mode_conf[cc].debugRate, nc, mode_conf[cc].nOfConfigs)); */
    }

    /* ------------- */
    /* Config Update */
    /* --------------*/

    if((LastWakeTime - config_tick) > mode_conf[cc].configRate && mode_conf[cc].cycleConfig){
      config_tick = LastWakeTime;
      pc = cc;
      cc = ((cc + 1) % nc);

      mode_conf[cc].fadeDir = mode_conf[pc].fadeDir;
    }

    /* --------- */
    /* fade mode */
    /* ----------*/

    if((LastWakeTime - fade_tick) >= mode_conf[cc].fadeRate && mode_conf[cc].fade){
      fade_tick = LastWakeTime;

      if(mode_conf[cc].fadeDir == 0){
          fadeTo(&mode_conf[cc]);
      }
      else{
          fadeZero(&mode_conf[cc]);
      }
      setLeds(mode_conf[cc]);
      outputLeds(mode_conf[cc]);
    }

    /* --------- */
    /* walk mode */
    /* ----------*/

    if((LastWakeTime - walk_tick) >= mode_conf[cc].walk_rate && mode_conf[cc].walk){
      walk_tick = LastWakeTime;

      stepForward(&mode_conf[cc]);
      outputLeds(mode_conf[cc]);
    }

    /* -------------- */
    /* fade_walk mode */
    /* ---------------*/

    if(fade_walk_t >= mode_conf[cc].fadeWalkRate && mode_conf[cc].fadeWalk){
      fade_walk_t = 0;
      fadeWalk(mode_conf[cc]);
      setLeds(mode_conf[cc]);
      outputLeds(mode_conf[cc]);
    }

    /* ----------- */
    /* time Update */
    /* ------------*/

    fade_t      += 1;
    walk_t      += 1;
    config_t    += 1;
    debug_t     += 1;
    fade_walk_t += 1;
    vTaskDelayUntil( &LastWakeTime, freq);
  }
}

void resetModeConfigs(struct mode_config *conf, uint8_t nConfigs, uint16_t nleds, uint8_t nSections){
  int i;
  for(i = 0; i < nConfigs; i ++){

    conf[i].length          = nleds;
    conf[i].nOfConfigs      = nConfigs;
    conf[i].section_length  = nSections;
    conf[i].section_offset  = 0;
    conf[i].fadeIteration   = 0; // TODO rename this to something relevant
    conf[i].fadeWalkFreq    = 0;
    conf[i].fadeWalkRate    = 0;
    conf[i].cycleConfig     = 0;
    conf[i].configRate      = 0;
    conf[i].debugRate       = 500;
    conf[i].pulseRate       = 0;
    conf[i].walk_rate       = 0;
    conf[i].fadeWalk        = 0;
    conf[i].fadeRate        = 0;
    conf[i].fadeDir         = 0;
    conf[i].smooth          = 0;
    conf[i].pulse           = 0;
    conf[i].step            = 0;
    conf[i].fade            = 0;
    conf[i].walk            = 0;
    conf[i].section_colors  = (malloc(conf[0].section_length * sizeof(section_colors_t)));
  }
}

void repeatModeZero(struct mode_config *conf){
  int i;
  for(i = 1; i < conf[0].nOfConfigs; i++)
    conf[i] = conf[0];
}

void initColors(struct mode_config  *mode_conf, struct section_colors_t *color){
  int i;
  for(i = 0; i < mode_conf->section_length; i ++){
    mode_conf->section_colors[i] = color[i];
  }
}

void fadeWalk( struct mode_config  conf){
  int i;
  for(i = 0; i < conf.length; i++){
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
    leds[i].r = leds[i].r - ((leds[i].r - leds[(i+1)%conf.length].r)/(3));
    leds[i].g = leds[i].g - ((leds[i].g - leds[(i+1)%conf.length].g)/(3));
    leds[i].b = leds[i].b - ((leds[i].b - leds[(i+1)%conf.length].b)/(3));
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
  }
}

void setSectionFadeColors(struct mode_config  conf){
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

void setFadeColorsSection(struct mode_config  conf){
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
void fadeZero(struct mode_config *conf){
  int i;
  static uint8_t it = 0;
  for(i = 0; i < conf->length; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - 0)/(128 - it));
    leds[i].g = leds[i].g - ((leds[i].g - 0)/(128 - it));
    leds[i].b = leds[i].b - ((leds[i].b - 0)/(128 - it));
  }
  it++;
  if(it == conf->fadeIteration){
    it = 0;
    conf->fadeDir = 0;
  }
}

void fadeTo(struct mode_config *conf){
  int i;
  static uint8_t it = 0;

  for(i = 0; i < conf->length; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(128 - it));
    leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(128 - it));
    leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(128 - it));
  }

  it++;
  if(it == conf->fadeIteration){
    it = 0;
    conf->fadeDir = 1;
  }
}

void setSectionColors(struct mode_config  conf){
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

void outputLeds(struct mode_config  mode_conf){
  int i;
  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[0].item, 24, 1));
  for(i = 0; i < mode_conf.length; i ++){
    ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[i].item, 24, 1));
    /* printf("i: %d, R: %d\n", i, leds[i].r); */
  }

  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, setItem, 1, 1));

}

void setLeds(struct mode_config  conf){
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

void stepForward(struct mode_config  *conf){
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

void stepFade(struct led_struct *led, struct mode_config  conf,
              uint8_t rTarget, uint8_t bTarget, uint8_t gTarget){

  led->r = led->r - (((double)led->r-(double)rTarget)/(double)conf.fadeRate);
  led->b = led->b - ((led->b-bTarget)/conf.fadeRate);
  led->g = led->g - ((led->g-gTarget)/conf.fadeRate);

}

void pulse(struct mode_config  conf){
}

