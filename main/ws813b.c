#include <stdio.h>
#include <complex.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ws813b.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "colors.h"

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

  // TODO add if music mode enabled, move to ledEngine
  initAdc();

}

void initAdc(){

  esp_err_t r;
  gpio_num_t adc_gpio_num;
  r = adc2_pad_get_io_num( ADC2_CHANNEL_2, &adc_gpio_num );
  printf("2\n");
  assert( r == ESP_OK );
  adc2_config_channel_atten( ADC2_CHANNEL_2, ADC_ATTEN_11db );
  vTaskDelay(2 * portTICK_PERIOD_MS);

}

void ledEngine(struct mode_config  *mode_conf){
  welcome();

  // n of freertos ticks for each mode
  const  TickType_t freq = UPDATE_FREQ_MS;
  static TickType_t LastWakeTime;
  static TickType_t config_tick;
  static TickType_t fade_tick;
  static TickType_t walk_tick;
  static TickType_t debug_tick;
  static TickType_t music_tick;

  static uint16_t   fade_walk_t = 0;
  static bool       ledsUpdated;

  // config
  static uint16_t   i;
  static uint8_t    cc;         // Current config
  static uint8_t    pc;         // previous config
  static uint8_t    nc;         // Number of configs

  // Period of modes in milliseconds
  static uint16_t walk_rate_ms   = 1000;
  static uint16_t fade_rate_ms   = 1000;
  static uint16_t config_rate_ms = 1000;
  static uint16_t debug_rate_ms  = 1000;
  static uint16_t music_rate_ms  = 1;

  // Music mode
  float musicRelativeAmp[N_FREQS];
  float musicBrightness[N_FREQS];
  uint8_t musicAmplitudeColor[N_FREQS];
  float complex samples[N_SAMPLES];
  float complex copy[N_SAMPLES];
  float freqAmps[N_FREQS];

  if(mode_conf[0].walk)
   walk_rate_ms   = 1000/(((float)(mode_conf[0].walk_rate)));
  if(mode_conf[0].fade)
   fade_rate_ms   = 1000/(((float)(mode_conf[0].fadeRate)));
  if(mode_conf[0].cycleConfig)
   config_rate_ms = 1000/(((float)(mode_conf[0].configRate)));
  if(mode_conf[0].debugRate > 0)
   debug_rate_ms  = 1000/(((float)(mode_conf[0].debugRate)));

  DPRINT(("----------------------  \n Initial rates in mS\n---------------------- \n "));
  DPRINT(("walk_rate_ms %d\n" , walk_rate_ms));
  DPRINT(("fade_rate_ms %d\n" , fade_rate_ms));
  DPRINT(("config_rate_ms %d\n" , config_rate_ms));
  DPRINT(("debug_rate_ms %d\n" , debug_rate_ms));
  DPRINT(("----------------------  \n Initial rates in mS \n---------------------- \n\n "));

  int64_t bt = 0;
  int64_t et = 0;

  // TODO use tickcount to determine execution of mode 

  cc = nc;
  pc = nc;
  nc = mode_conf[0].nOfConfigs;
  debug_tick = mode_conf[0].debugRate + 1;


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

  walk_tick   = xTaskGetTickCount();
  fade_tick   = walk_tick;
  config_tick = walk_tick;
  music_tick  = walk_tick;

  while(1){
    LastWakeTime = xTaskGetTickCount();

    /* ------ */
    /* DEBUG */
    /* ------*/

    /* if(LastWakeTime - walk_tick >= 50){ */
    /*   printf("walk_tick: %d\n", walk_tick); */
    /*   walk_tick = LastWakeTime; */
    /* } */

    if(LastWakeTime - debug_tick > debug_rate_ms){
      debug_tick = LastWakeTime;
      DPRINT(("----------------------- Debug info ----------------------- \n"));
      DPRINT(("current conf: %d\nwalk:         %d\nfade:         %d\nsmooth:       %d\nconfig_tick:  %d\n",
              cc, mode_conf[cc].walk, mode_conf[cc].fade, mode_conf[cc].smooth, config_tick));
      DPRINT(("----------------------------------------------------------- \n\n "));


      DPRINT(("----------------------  Timing info ---------------------- \n"));
      DPRINT(("Time taken to output leds: %f mS\n", ((float)(et-bt))/1000));
      DPRINT(("---------------------------------------------------------- \n\n"));

      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));
      FADE_DEBUG(("Current conf: %d\n Fade: %d\n fadeRate: %d\n ", cc, mode_conf[cc].fade, mode_conf[cc].fadeRate));
      FADE_DEBUG(("fadeIteration: %d\n FadeDir: %d\n ", mode_conf[cc].fadeIteration, mode_conf[cc].fadeDir));
      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));

      /* DPRINT((" DebugRate: %d\n nc: %d\n mode_conf[cc].nOfConfigs: %d\n\n", */
              /* mode_conf[cc].debugRate, nc, mode_conf[cc].nOfConfigs)); */
    }

    /* ------------- */
    /* Config Update */
    /* --------------*/

    if((LastWakeTime - config_tick) > config_rate_ms && mode_conf[cc].cycleConfig){
      config_tick = LastWakeTime;
      pc = cc;
      cc = ((cc + 1) % nc);

      mode_conf[cc].fadeDir = mode_conf[pc].fadeDir;


      if(mode_conf[cc].walk)
        walk_rate_ms   = 1/(((float)(mode_conf[cc].walk_rate))/1000);
      if(mode_conf[cc].fade)
        fade_rate_ms   = 1/(((float)(mode_conf[cc].fadeRate))/1000);
      if(mode_conf[cc].cycleConfig)
        config_rate_ms = 1/(((float)(mode_conf[cc].configRate))/1000);
      if(mode_conf[cc].debugRate > 0)
        debug_rate_ms  = 1/(((float)(mode_conf[cc].debugRate))/1000);
    }

    /* --------- */
    /* fade mode */
    /* ----------*/

    if((LastWakeTime - fade_tick) >= fade_rate_ms && mode_conf[cc].fade){
      fade_tick = LastWakeTime;

      if(mode_conf[cc].fadeDir == 0){
        bt = esp_timer_get_time();
        fadeTo(&mode_conf[cc]);
        et = esp_timer_get_time();
        /* printf("FaderTo time: %lld\n", (et-bt)); */
      }
      else{
        fadeZero(&mode_conf[cc]);
      }
      setLeds(mode_conf[cc]);
      ledsUpdated = true;
      /* outputLeds(mode_conf[cc]); */
    }

    /* --------- */
    /* walk mode */
    /* ----------*/

    if((LastWakeTime - walk_tick) >= walk_rate_ms && mode_conf[cc].walk){
      walk_tick = LastWakeTime;

      stepForward(&mode_conf[cc]);
      ledsUpdated = true;
      /* outputLeds(mode_conf[cc]); */
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
    /* --------- */
    /*    FFT    */
    /* ----------*/

    // TODO change 150 an variable, add enable var in config
    if((LastWakeTime - music_tick) >= 0 && mode_conf[cc].music){
      music_tick = LastWakeTime;

      startAdc(samples, copy);
      fft(samples, copy, N_SAMPLES, 1, 0);
      fbinToFreq(samples, freqAmps);
      scaleAmpRelative(freqAmps, musicAmplitudeColor, musicBrightness, musicRelativeAmp);

      if(mode_conf[cc].musicMode1)
        music_mode1(musicAmplitudeColor, musicBrightness, mode_conf[cc]);
      if(mode_conf[cc].musicMode2)
        music_mode2(musicRelativeAmp, mode_conf[cc]);

      for(i = 0; i < mode_conf[cc].smooth; i++){
        fadeWalk(mode_conf[cc]);
      }

      setLeds(mode_conf[cc]);
      ledsUpdated = true;

    }

    /* ----------- */
    /* time Update */
    /* ------------*/

    if(ledsUpdated == true){
      ledsUpdated = false;

      bt = esp_timer_get_time();
      outputLeds(mode_conf[cc]);
      et = esp_timer_get_time();
    }
    vTaskDelayUntil( &LastWakeTime, freq);
  }
}

void scaleAmpRelative(float *power, uint8_t *amplitudeColor, float *color_brightness, float *relativeAmp){

  // TODO scale rangers from 3 to 10 or higher for better representation of freq.

  /* uint8_t amplitudeColor[N_FREQS]; */
  uint8_t i;
  uint16_t max = 0;
  static uint8_t avgCount = 0;
  float avgAmp[10];
  /* float brightness[N_FREQS]; */
  float avg, avgOfArrayAvg;
  float lowAmp, highAmp, maxAmp;

  avg = 0;

  // TODO move or use another way to "normalize" low freq
  power[0] -= 0.8;
  for(i = 0; i < N_FREQS; i++){
    avg += power[i];
    if(power[i] > max)
      max = power[i];
  }

  avg = avg / (N_FREQS);
  avgAmp[avgCount] = avg;
  avgCount = (avgCount + 1) % 10;

  avgOfArrayAvg = 0;
  for(i = 0; i < 10; i++){
    avgOfArrayAvg += avgAmp[i];
  }

  avgOfArrayAvg = avgOfArrayAvg / 10 + AVG_OFFSET;
  lowAmp = avgOfArrayAvg * 0.66;
  highAmp = avgOfArrayAvg * 1.33;
  maxAmp = highAmp * 1.33;
  if(lowAmp < MIN_AMP)
    lowAmp = MIN_AMP;


  // TODO dont use a billion else-ifs

  for(i = 0; i < N_FREQS; i++){
    if(power[i] <= MIN_AMP){
      amplitudeColor[i] = 3;
      color_brightness[i] = 0;
    }
    else if(power[i] <= lowAmp){
      amplitudeColor[i] = 2;
      color_brightness[i] = power[i]/lowAmp;
      /* printf("lowamp: %.2f, power[i]: %.2f\n", lowAmp, power[i]); */
    }
    else if(power[i] <= highAmp){
      amplitudeColor[i] = 1;
      color_brightness[i] = power[i]/highAmp;
    }
    else if(power[i] > highAmp){
      amplitudeColor[i] = 0;
      color_brightness[i] = power[i]/maxAmp;
    }

    if(power[i] > MIN_AMP)
      relativeAmp[i] = power[i]/maxAmp;
    else
      relativeAmp[i] = 0;

    if(color_brightness[i] < 0.3)
      color_brightness[i] = 0.3;
    if(color_brightness[i] > 1)
      color_brightness[i] = 1;

    power[i] = 0;

    /* printf("i: %d, amplitudeColor %d, brightness: %.2f\n", i, amplitudeColor[i], brightness[i]); */
  }

  /* printf("\n"); */
  /* printf("lowamp: %.2f, highAmp: %.2f, MaxAmp: %.2f, avg: %.2f\n", */
  /*        lowAmp, highAmp, maxAmp, avgOfArrayAvg); */

}

void music_mode2(float *relativeAmp, struct mode_config conf){

  float rgbBrightness[RGB];
  uint8_t i, j, r, g ,b;

  // TODO make the scaling a bit less extreme and do some magic in "scaleAmpRelative"
  const float min = 0.20;
  const float mid = 0.40;

  for(i = 0; i < RGB; i++){
    for(j = i*N_FREQS/RGB; j < N_FREQS; j++){
      rgbBrightness[i] += relativeAmp[j];
    }
    rgbBrightness[i] = rgbBrightness[i] / (N_FREQS/RGB);
    if(rgbBrightness[i] > 1)
      rgbBrightness[i] = 1;
    /* printf("i: %d, rgbBrightness: %.3f\n", i, rgbBrightness[i]); */

  }

  r = 255 * (relativeAmp[0] > relativeAmp[1] ? relativeAmp[0]: relativeAmp[1]);
  g = 255 * (relativeAmp[2] > relativeAmp[3] ? relativeAmp[2]: relativeAmp[3]);
  b = 255 * (relativeAmp[4] > relativeAmp[5] ? relativeAmp[4]: relativeAmp[5]);

  /* r = 255 * (relativeAmp[0] + relativeAmp[1])/2; */
  /* g = 255 * (relativeAmp[2] + relativeAmp[3])/2; */
  /* b = 255 * (relativeAmp[4] + relativeAmp[5])/2; */

  //TODO make this not so cancer

  if(r < g && r < b)
    r = r*min;
  else if(g < r && g < b)
    g = g*min;
  else if(b < g && b < r)
    b = b*min;

  if(r < g && r > b)
    r = r*mid;
  else if(g < r && g > b)
    g = g*mid;
  else if(b < g && b > r)
    b = b*mid;

  for(i = 0; i <  conf.length; i ++){

    /* resistLedChange(r, g, b, i, M_RESISTANCE); */
    /* resistLowerLedChange(r, g, b, i, L_RESISTANCE); */
    variableResistLedChange(r, g, b, i, L_RESISTANCE, H_RESISTANCE);

    /* leds[i].r = 255 * rgbBrightness[0]; */
    /* leds[i].g = 255 * rgbBrightness[1]; */
    /* leds[i].b = 255 * rgbBrightness[2]; */
  }

  for(i = 0; i < RGB; i++){
      rgbBrightness[i] = 0;
  }
}

void music_mode1(uint8_t *amplitudeColor, float *brightness, struct mode_config conf){
  uint16_t i,j;
  uint8_t r,g,b;


  for(i = 0; i < N_FREQS; i ++){
    for(j = i*(conf.length/(N_FREQS)); j <  (i+1)*(conf.length/(N_FREQS)); j ++){

      /* leds[j].r = (int)((float)music_colors[amplitudeColor[i]].red   * brightness[i]); */
      /* leds[j].g = (int)((float)music_colors[amplitudeColor[i]].green * brightness[i]); */
      /* leds[j].b = (int)((float)music_colors[amplitudeColor[i]].blue  * brightness[i]); */

      r = (int)((float)music_colors[amplitudeColor[i]].red   * brightness[i]);
      g = (int)((float)music_colors[amplitudeColor[i]].green * brightness[i]);
      b = (int)((float)music_colors[amplitudeColor[i]].blue  * brightness[i]);

      variableResistLedChange(r, g, b, j, L_RESISTANCE, H_RESISTANCE);
    }

  }
}

void resistLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float resist){
  leds[led].r = leds[led].r - (leds[led].r - r)/resist;
  leds[led].g = leds[led].g - (leds[led].g - g)/resist;
  leds[led].b = leds[led].b - (leds[led].b - b)/resist;
}

void resistLowerLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float resist){
  if(leds[led].r > r)
    leds[led].r = leds[led].r - (leds[led].r - r)/resist;
  else
    leds[led].r = r;

  if(leds[led].g > g)
    leds[led].g = leds[led].g - (leds[led].g - g)/resist;
  else
    leds[led].g = g;

  if(leds[led].b > b)
    leds[led].b = leds[led].b - (leds[led].b - b)/resist;
  else
    leds[led].b = b;
}

void variableResistLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float l_resist, float h_resist){
  if(leds[led].r > r)
    leds[led].r = leds[led].r - (leds[led].r - r)/l_resist;
  else
    leds[led].r = leds[led].r - (leds[led].r - r)/h_resist;

  if(leds[led].g > g)
    leds[led].g = leds[led].g - (leds[led].g - g)/l_resist;
  else
    leds[led].g = leds[led].g - (leds[led].g - g)/h_resist;

  if(leds[led].b > b)
    leds[led].b = leds[led].b - (leds[led].b - b)/l_resist;
  else
    leds[led].b = leds[led].b - (leds[led].b - b)/h_resist;
}

void fft(float complex x[], float complex y[], int N, int step, int offset){

  // Make sure N = 2^x
  int k, i;
  float complex t;

  if(step < N){
    fft(y, x, N, step*2, offset);          // ftt even
    fft(y, x, N, step*2, (offset + step)); // ftt odd
    i = 0;

    for(k = offset; k < N/2; k += step){
      int even = step*2*i + offset;
      int odd  = step + offset + 2*i*step;

      t = cexp(-2*I*M_PI*i/(N/step)) * y[odd];
      x[k]        = y[even] + t; 
      x[k + N/2]  = y[even] - t;
      i++;
    }
  }
}

void fbinToFreq(float complex *in, float *out){
  const static int freqs[N_FREQS]        = {100, 250, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};
  // TODO consider only using certain frequencies instead of summing them up in chunks
  const static int freqSpacing[N_FREQS] = {175, 375, 750, 1250, 1750, 2250, 2750, 3250, 3750, 4250, 4750, 5000};

  float power[N_SAMPLES];
  int freqCount, i;

  for(i = 0; i < N_SAMPLES; i++){
    power[i] = cabs(in[i]);
  }

  freqCount = 0;

  for(i = 2; i < N_SAMPLES; i++){
   out[freqCount] += power[i];
    if(freqSpacing[freqCount] < i*(float)SAMPLE_RATE/N_SAMPLES)
      freqCount += 1;
    if(freqCount >= N_FREQS)
      break;
  }

  /* for(i = 0; i < N_FREQS; i ++){ */
  /*   printf("Freq: %4d, power: %.10f\n", abs(freqs[i]), out[i]); */
  /* } */
  /* printf("\n"); */

  /* for(i = 0; i < freqCount; i++){ */
  /*   out[i] = 0; */
  /* } */
}

void startAdc(float complex *samplesOut, float complex *copy){

  // TODO remove wait of 100 uS before getting first sample
  int i;
  int amplitude;
  float voltage;
  esp_err_t err;
  int64_t currTime;
  int64_t lastTime;

  i = 0;
  lastTime = esp_timer_get_time();

  while(i < N_SAMPLES){
    currTime = esp_timer_get_time();

    if(currTime - lastTime >= 50){
      lastTime = esp_timer_get_time();
      err = adc2_get_raw( ADC2_CHANNEL_2,  ADC_WIDTH_12Bit, &amplitude);

      if ( err == ESP_OK ) {
        /* voltage = 3.3/4098 * amplitude - 1.633; */
        voltage = (amplitude - 2048) * 3.3/4098;
        samplesOut[i] = voltage;
        copy[i] = voltage;
        i += 1;
      }
      else
        adcErrCtrl(err);
    }
  }
}

void adcErrCtrl(esp_err_t err){

  if ( err == ESP_ERR_INVALID_STATE ) {
    printf("%s: ADC2 not initialized yet.\n", esp_err_to_name(err));
  }
  else if ( err == ESP_ERR_TIMEOUT ) {
    printf("%s: ADC2 is in use by Wi-Fi.\n", esp_err_to_name(err));
  }
  else {
    printf("%s\n", esp_err_to_name(err));
  }
}

void resetModeConfigs(struct mode_config *conf, uint8_t nConfigs, uint16_t nleds, uint8_t nSections){
  int i;
  for(i = 0; i < nConfigs; i ++){

    conf[i].length         = nleds;
    conf[i].nOfConfigs     = nConfigs;
    conf[i].section_length = nSections;
    conf[i].section_offset = 0;
    conf[i].fadeIteration  = 0; // TODO rename this to something relevant
    conf[i].fadeWalkFreq   = 0;
    conf[i].fadeWalkRate   = 0;
    conf[i].cycleConfig    = 0;
    conf[i].configRate     = 0;
    conf[i].debugRate      = 500;
    conf[i].pulseRate      = 0;
    conf[i].walk_rate      = 0;
    conf[i].fadeWalk       = 0;
    conf[i].fadeRate       = 0;
    conf[i].fadeDir        = 0;
    conf[i].smooth         = 0;
    conf[i].pulse          = false;
    conf[i].step           = false;
    conf[i].fade           = false;
    conf[i].walk           = false;
    conf[i].music          = false;
    conf[i].musicMode1     = false;
    conf[i].musicMode2     = false;

    conf[i].section_colors = (malloc(conf[0].section_length * sizeof(section_colors_t)));
  }
}

void repeatModeZero(struct mode_config *conf){
  int i;
  for(i = 1; i < conf[0].nOfConfigs; i++)
    conf[i] = conf[0];
}

void initColors(struct mode_config  *mode_conf, const struct section_colors_t *color){
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

void fadeZero(struct mode_config *conf){
  int i;
  static uint8_t it = 0;
  for(i = 0; i < conf->length; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - 0)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - 0)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - 0)/(64 - it));
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
    leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(64 - it));
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
  /* ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[0].item, 24, 1)); */
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

void pulse(struct mode_config  conf){}

void welcome(){


  printf(" __            _    ____        _                \n");
  printf("|  |    ___  _| |  |    \\  ___ |_| _ _  ___  ___ \n");
  printf("|  |__ | -_|| . |  |  |  ||  _|| || | || -_||  _|\n");
  printf("|_____||___||___|  |____/ |_|  |_| \\_/ |___||_|  \n");
  printf("                                                 \n");
  printf("                                                 \n");
  printf("                   _                             \n");
  printf("                  | |_  _ _                      \n");
  printf("                  | . || | |                     \n");
  printf("                  |___||_  |                     \n");
  printf("                       |___|                     \n");
  printf("                                                 \n");
  printf("      _____         _                                 \n");
  printf("     |  _  | ___  _| | ___  ___  ___  ___             \n");
  printf("     |     ||   || . ||  _|| -_|| .'||_ -|            \n");
  printf("     |__|__||_|_||___||_|  |___||__,||___|            \n");
  printf("                                                 \n");
  printf("          __ __                                           \n");
  printf("         |__|__| _                                        \n");
  printf("         |     || |_  _____  ___  ___                     \n");
  printf("         |  |  ||   ||     || .'||   |                    \n");
  printf("         |_____||_|_||_|_|_||__,||_|_|  \n");

}
