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

void initRmt(mode_config *mode_conf, uint8_t ledPin){

  rmt_conf.rmt_mode                       = RMT_MODE_TX;
  rmt_conf.channel                        = RMT_CHANNEL_0;
  rmt_conf.gpio_num                       = ledPin;
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

void initAdc(){

  esp_err_t r;
  gpio_num_t adc_gpio_num;
  r = adc1_pad_get_io_num( ADC1_CHANNEL_6, &adc_gpio_num );
  assert( r == ESP_OK );
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten( ADC1_CHANNEL_6, ADC_ATTEN_11db );
  vTaskDelay(2 * portTICK_PERIOD_MS);

}

void ledEngineTest(void *param){

  int i,j;
  mode_config *conf = (mode_config*) param;
  mode_config configCopy[conf[0].nOfConfigs];

  for(i = 0; i < conf[0].nOfConfigs; i++){
    configCopy[i].section_colors = (malloc(conf[0].section_length * sizeof(color_t)));
    configCopy[i] = conf[i];

    for(j = 0; j < configCopy[i].section_length; j ++){
      configCopy[i].section_colors[j] = conf[i].section_colors[j];
    }
  }

  ledEngine(configCopy);
}

void ledEngine(mode_config  *mode_conf){
  welcome();

  modeConfigQueue = xQueueCreate(1, sizeof(mode_config*));
  if(modeConfigQueue == 0){
    printf("failed to allocate memory for modeConfigQueue\n");
    printf("Entering endless loop\n");
    while(1){}
  }

  modeConfigQueueAck = xQueueCreate(1, sizeof(mode_config*));
  if(modeConfigQueue == 0){
    printf("failed to allocate memory for modeConfigQueueAck\n");
    printf("Entering endless loop\n");
    while(1){}
  }

  // n of freertos ticks for each mode
  const  TickType_t freq = UPDATE_FREQ_MS;
  static TickType_t LastWakeTime;
  static TickType_t configTick;
  static TickType_t fadeTick;
  static TickType_t walkTick;
  static TickType_t debugTick;
  static TickType_t musicTick;

  static uint16_t   fadeWalk_t = 0;
  static bool       ledsUpdated;

  // config
  static uint16_t   i;
  static uint8_t    cc;         // Current config
  static uint8_t    pc;         // previous config
  static uint8_t    nc;         // Number of configs

  // Period of modes in milliseconds
  static rates_struct r;
   r.walkRateMs   = 1000;
   r.fadeRateMs   = 1000;
   r.configRateMs = 1000;
   r.debugRateMs  = 1000;
   r.musicRateMs  = 1;

  // Music mode
  bool    adcInitiated = FALSE;
  float   musicRelativeAmp[N_FREQS];
  float   musicBrightness[N_FREQS];
  uint8_t musicAmplitudeColor[N_FREQS];
  float   complex samples[N_SAMPLES];
  float   complex copy[N_SAMPLES];
  float   freqAmps[N_FREQS];


  if(mode_conf[cc].musicMode1 || mode_conf[cc].musicMode2)
    mode_conf[cc].music = TRUE;

  updateRates(&r, mode_conf[cc]);

  DPRINT(("----------------------  \n Initial rates in mS\n---------------------- \n "));
  DPRINT(("walkRateMs %d\n" , r.walkRateMs));
  DPRINT(("r.fadeRateMs %d\n" , r.fadeRateMs));
  DPRINT(("r.configRateMs %d\n" , r.configRateMs));
  DPRINT(("r.debugRateMs %d\n" , r.debugRateMs));
  DPRINT(("r.musicRateMs %d\n" , r.musicRateMs));
  DPRINT(("----------------------  \n Initial rates in mS \n---------------------- \n\n "));

  int64_t bt = 0;
  int64_t et = 0;

  cc = nc;
  pc = nc;
  nc = mode_conf[0].nOfConfigs;
  debugTick = mode_conf[0].debugRate + 1;


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


  if(mode_conf[cc].music){
    initAdc();
    adcInitiated = TRUE;
  }

  walkTick   = xTaskGetTickCount();
  fadeTick   = walkTick;
  configTick = walkTick;
  musicTick  = walkTick;

  while(1){
    LastWakeTime = xTaskGetTickCount();

    if(updateConfigFromQueue(mode_conf))
      updateRates(&r, mode_conf[cc]);

    /* ------ */
    /* DEBUG */
    /* ------*/

    if(LastWakeTime - debugTick > r.debugRateMs){
      debugTick = LastWakeTime;

      DPRINT(("----------------------- Debug info ----------------------- \n"));
      DPRINT(("current conf: %d\nwalk:         %d\nfade:         %d\nsmooth:       %d\nconfigTick:  %d\n",
              cc, mode_conf[cc].walk, mode_conf[cc].fade, mode_conf[cc].smooth, configTick));
      DPRINT(("----------------------------------------------------------- \n\n "));


      DPRINT(("----------------------  Timing info ---------------------- \n"));
      DPRINT(("Time taken to output leds: %f mS\n", ((float)(et-bt))/1000));
      DPRINT(("---------------------------------------------------------- \n\n"));

      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));
      FADE_DEBUG(("Current conf: %d\n Fade: %d\n fadeRate: %d\n ", cc, mode_conf[cc].fade, mode_conf[cc].fadeRate));
      FADE_DEBUG(("fadeIterations: %d\n FadeDir: %d\n ", mode_conf[cc].fadeIterations, mode_conf[cc].fadeDir));
      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));

    }

    /* ------------- */
    /* Config Update */
    /* --------------*/

    if((LastWakeTime - configTick) > r.configRateMs && mode_conf[cc].cycleConfig){
      configTick = LastWakeTime;
      pc = cc;
      cc = ((cc + 1) % nc);

      mode_conf[cc].fadeDir = mode_conf[pc].fadeDir;

      if(mode_conf[cc].musicMode1 || mode_conf[cc].musicMode2)
        mode_conf[cc].music = TRUE;
      else
        mode_conf[cc].music = FALSE;

      if(mode_conf[cc].music && !adcInitiated){
        initAdc();
        adcInitiated = TRUE;
      }
      updateRates(&r, mode_conf[cc]);
    }

    /* --------- */
    /* fade mode */
    /* ----------*/

    if((LastWakeTime - fadeTick) >= r.fadeRateMs && mode_conf[cc].fade){
      fadeTick = LastWakeTime;

      if(mode_conf[cc].fadeDir == 0){
        bt = esp_timer_get_time();
        fadeTo(&mode_conf[cc]);
        et = esp_timer_get_time();
      }
      else{
        fadeZero(&mode_conf[cc]);
      }
      setLeds(mode_conf[cc]);
      ledsUpdated = TRUE;
    }

    /* --------- */
    /* walk mode */
    /* ----------*/

    if((LastWakeTime - walkTick) >= r.walkRateMs && mode_conf[cc].walk){
      walkTick = LastWakeTime;
      stepForward(&mode_conf[cc]);
      ledsUpdated = TRUE;
    }

    /* -------------- */
    /* fade_walk mode */
    /* ---------------*/

    // TODO change behaviour of this one
    if(fadeWalk_t >= mode_conf[cc].fadeWalkRate && mode_conf[cc].fadeWalk){
      fadeWalk_t = 0;
      fadeWalk(mode_conf[cc]);
      setLeds(mode_conf[cc]);
      ledsUpdated = TRUE;
      /* outputLeds(mode_conf[cc]); */
    }
    /* --------- */
    /*    FFT    */
    /* ----------*/

    if((LastWakeTime - musicTick) >= r.musicRateMs && mode_conf[cc].music){
      musicTick = LastWakeTime;
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
      ledsUpdated = TRUE;

    }

    /* ----------- */
    /* time Update */
    /* ------------*/

    if(ledsUpdated == TRUE){
      ledsUpdated = FALSE;

      bt = esp_timer_get_time();
      outputLeds(mode_conf[cc]);
      et = esp_timer_get_time();
    }

    vTaskDelayUntil( &LastWakeTime, freq);
  }
}

void scaleAmpRelative(float *power, uint8_t *amplitudeColor, float *color_brightness, float *relativeAmp){

  // TODO scale rangers from 3 to 10 or higher for better representation of freq.

  uint8_t i, j;
  uint16_t max = 0;
  static uint8_t avgCount = 0;
  float avgAmp[N_AVG_VAL];
  float avg, avgOfArrayAvg;
  float lowAmp, highAmp, maxAmp;
  static float testAVG[N_FREQS][N_AVG_VAL]; // TODO rename / change stuff
  float invidAvg[N_FREQS];
  avg = 0;

  // TODO move or use another way to "normalize" low freq
  power[0] -= LOW_HZ_OFFSET;
  for(i = 0; i < N_FREQS; i++){
    avg += power[i];
    testAVG[i][avgCount] = power[i];
    if(power[i] > max)
      max = power[i];
  }

  for(i = 0; i < N_FREQS; i++){
    invidAvg[i] = 0;
  }

  for(j = 0; j < N_FREQS; j++){
    for(i = 0; i < N_AVG_VAL; i++){
      invidAvg[j] += testAVG[j][i];
    }
    invidAvg[j] = invidAvg[j] / N_AVG_VAL;
  }

  avg = avg / (N_FREQS);
  avgAmp[avgCount] = avg;
  avgCount = (avgCount + 1) % N_AVG_VAL;

  avgOfArrayAvg = 0;
  for(i = 0; i < 10; i++){
    avgOfArrayAvg += avgAmp[i];
  }

  avgOfArrayAvg = avgOfArrayAvg / N_AVG_VAL + AVG_OFFSET;

  lowAmp = avgOfArrayAvg * 0.80;
  highAmp = avgOfArrayAvg * 1.10;
  maxAmp = highAmp * 1.20;

  if(lowAmp < MIN_AMP)
    lowAmp = MIN_AMP;


  // TODO dont use a billion else-ifs

  /* for(i = 0; i < N_FREQS; i++){ */
  /*   if(power[i] <= MIN_AMP){ */
  /*     amplitudeColor[i] = 3; */
  /*     color_brightness[i] = 0; */
  /*   } */
  /*   else if(power[i] <= lowAmp){ */
  /*     amplitudeColor[i] = 2; */
  /*     color_brightness[i] = power[i]/lowAmp; */
  /*     /\* printf("lowamp: %.2f, power[i]: %.2f\n", lowAmp, power[i]); *\/ */
  /*   } */
  /*   else if(power[i] <= highAmp){ */
  /*     amplitudeColor[i] = 1; */
  /*     color_brightness[i] = power[i]/highAmp; */
  /*   } */
  /*   else if(power[i] > highAmp){ */
  /*     amplitudeColor[i] = 0; */
  /*     color_brightness[i] = power[i]/maxAmp; */
  /*   } */


    for(i = 0; i < N_FREQS; i++){
      if(power[i] <= MIN_AMP || power[i] < invidAvg[i]){
        amplitudeColor[i] = 3;
        color_brightness[i] = 0;
      }
      else if(power[i] <= invidAvg[i] * 1.2){
        amplitudeColor[i] = 2;
        color_brightness[i] = power[i]/lowAmp;
        /* printf("lowamp: %.2f, power[i]: %.2f\n", lowAmp, power[i]); */
      }
      else if(power[i] <=invidAvg[i] * 1.5 ){
        amplitudeColor[i] = 1;
        color_brightness[i] = power[i]/highAmp;
      }
      else if(power[i] > invidAvg[i] * 1.8){
        amplitudeColor[i] = 0;
        color_brightness[i] = power[i]/maxAmp;
      }

    /* if(power[i] > invidAvg[i] * 3 && power[i] > MIN_AMP) */
    if(power[i] > MIN_AMP && power[i] > invidAvg[i])
      /* relativeAmp[i] = power[i]/maxAmp; */
      relativeAmp[i] = power[i]/max;
    else
      relativeAmp[i] = 0;

    if(color_brightness[i] < 0.3)
      color_brightness[i] = 0.3;
    if(color_brightness[i] > 1)
      color_brightness[i] = 1;

    power[i] = 0;
  }
}

void music_mode2(float *relativeAmp, mode_config conf){

  float rgbBrightness[RGB];
  uint8_t i, j, r, g ,b;

  // TODO DEFINE THEESE
  const float min = 0.0;
  const float mid = 0.3;

  for(i = 0; i < RGB; i++){
    for(j = i*N_FREQS/RGB; j < N_FREQS; j++){
      rgbBrightness[i] += relativeAmp[j];
    }
    rgbBrightness[i] = rgbBrightness[i] / (N_FREQS/RGB);
    if(rgbBrightness[i] > 1)
      rgbBrightness[i] = 1;

  }

  r = 255 * (relativeAmp[0] > relativeAmp[1] ? relativeAmp[0]: relativeAmp[1]);
  g = 255 * (relativeAmp[2] > relativeAmp[3] ? relativeAmp[2]: relativeAmp[3]);
  b = 255 * (relativeAmp[4] > relativeAmp[5] ? relativeAmp[4]: relativeAmp[5]);

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
    variableResistLedChange(r, g, b, i, L_RESISTANCE, H_RESISTANCE);
  }

  for(i = 0; i < RGB; i++){
      rgbBrightness[i] = 0;
  }
}

void music_mode1(uint8_t *amplitudeColor, float *brightness, mode_config conf){
  uint16_t i,j;
  uint8_t r,g,b;

  for(i = 0; i < N_FREQS; i ++){
    for(j = i*(conf.length/(N_FREQS)); j <  (i+1)*(conf.length/(N_FREQS)); j ++){

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
  /* const static int freqs[N_FREQS]        = {100, 250, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000}; */
  const static int freqSpacing[N_FREQS] = {175, 375, 750, 1250, 1750, 2250, 2750, 3250, 3750, 4250, 4750, 5000};

  float power[N_SAMPLES];
  int freqCount, i;

  for(i = 0; i < N_SAMPLES; i++){
    power[i] = cabs(in[i]);
  }

  freqCount = 0;

  for(i = 1; i < N_SAMPLES; i++){
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

    if(currTime - lastTime >= SAMPLE_RATE_US){
      lastTime = esp_timer_get_time();
      amplitude = adc1_get_raw( ADC1_CHANNEL_6);

        voltage = (amplitude - 2048) * 3.3/4098;
        samplesOut[i] = voltage;
        copy[i] = voltage;
        i += 1;
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

void initModeConfigs(mode_config *conf, uint8_t nConfigs, uint16_t nleds, uint8_t nSections){
  int i;
  for(i = 0; i < nConfigs; i ++){

    conf[i].length         = nleds;
    conf[i].nOfConfigs     = nConfigs;
    conf[i].section_length = nSections;
    conf[i].section_offset = 0;
    conf[i].fadeIterations = 0; // TODO rename this to something relevant
    conf[i].fadeWalkFreq   = 0;
    conf[i].fadeWalkRate   = 0;
    conf[i].cycleConfig    = 0;
    conf[i].configRate     = 0;
    conf[i].debugRate      = 500;
    conf[i].musicRate      = 0;
    conf[i].walkRate       = 0;
    conf[i].fadeRate       = 0;
    conf[i].fadeWalk       = 0;
    conf[i].fadeDir        = 0;
    conf[i].smooth         = 0;
    conf[i].step           = FALSE;
    conf[i].fade           = FALSE;
    conf[i].walk           = FALSE;
    conf[i].music          = FALSE;
    conf[i].musicMode1     = FALSE;
    conf[i].musicMode2     = FALSE;

    conf[i].section_colors = (malloc(conf[0].section_length * sizeof(color_t)));
  }
}

void repeatModeZero(mode_config *conf){
  int i;
  for(i = 1; i < conf[0].nOfConfigs; i++)
    conf[i] = conf[0];
}

void initColors(mode_config  *mode_conf, const color_t *color){
  int i;
  for(i = 0; i < mode_conf->section_length; i ++){
    mode_conf->section_colors[i] = color[i];
  }
}

void fadeWalk( mode_config  conf){
  int i;
  for(i = 0; i < conf.length; i++){
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
    leds[i].r = leds[i].r - ((leds[i].r - leds[(i+1)%conf.length].r)/(3));
    leds[i].g = leds[i].g - ((leds[i].g - leds[(i+1)%conf.length].g)/(3));
    leds[i].b = leds[i].b - ((leds[i].b - leds[(i+1)%conf.length].b)/(3));
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
  }
}

void fadeZero(mode_config *conf){
  int i;
  static uint8_t it = 0;
  for(i = 0; i < conf->length; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - 0)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - 0)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - 0)/(64 - it));
  }
  it++;
  if(it == conf->fadeIterations){
    it = 0;
    conf->fadeDir = 0;
  }
}

void fadeTo(mode_config *conf){
  int i;
  static uint8_t it = 0;

  for(i = 0; i < conf->length; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(64 - it));
  }

  it++;
  if(it == conf->fadeIterations){
    it = 0;
    conf->fadeDir = 1;
  }
}

void setSectionColors(mode_config  conf){
  int i,j;
  for(i = 0; i < conf.section_length; i ++){
    for(j = i*(conf.length/conf.section_length); j <  (i+1)*(conf.length/conf.section_length); j ++){
      leds[j].r = conf.section_colors[i].red;
      leds[j].g = conf.section_colors[i].green;
      leds[j].b = conf.section_colors[i].blue;
    }
  }
}

void outputLeds(mode_config  mode_conf){
  int i;
  for(i = 0; i < mode_conf.length; i ++){
    ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[i].item, 24, 1));
  }

  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, setItem, 1, 1));

}

void setLeds(mode_config  conf){
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
    }
    else{
      item[i] = *zeroItem;
    }
  }
}

void printDuration0(rmt_item32_t *item){
  int i;
  for(i = 0; i < 24; i ++){
    printf("Duration0: %d\n", item[i].duration0);
  }
}

void stepForward(mode_config  *conf){
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

void welcome(){


  printf(" __            _    ____        _\n");
  printf("|  |    ___  _| |  |    \\  ___ |_| _ _  ___  ___ \n");
  printf("|  |__ | -_|| . |  |  |  ||  _|| || | || -_||  _|\n");
  printf("|_____||___||___|  |____/ |_|  |_| \\_/ |___||_|  \n");
  printf("\n");
  printf("\n");
  printf("                   _\n");
  printf("                  | |\n");
  printf("                  | . || | |\n");
  printf("                  |___||_  |\n");
  printf("                       |___|\n");
  printf("\n");
  printf("      _____         _\n");
  printf("     |  _  | ___  _| | ___  ___  ___  ___\n");
  printf("     |     ||   || . ||  _|| -_|| .'||_ -|\n");
  printf("     |__|__||_|_||___||_|  |___||__,||___|\n");
  printf("\n");
  printf("          __ __\n");
  printf("         |__|__| _\n");
  printf("         |     || |_  _____  ___  ___\n");
  printf("         |  |  ||   ||     || .'||   |\n");
  printf("         |_____||_|_||_|_|_||__,||_|_|\n");

}

int updateConfigFromQueue(mode_config *mode_conf){
  struct mode_conf *noData;
  if( xQueueReceive( modeConfigQueueAck, &noData,(TickType_t)0)){
    xQueueSend( modeConfigQueue, ( void * ) &(mode_conf), ( TickType_t ) 0 );
    if( xQueueReceive( modeConfigQueueAck, &noData,(TickType_t)100000)){
      printf("Config updated\n");
      return 1;
    }
    else
      return 0;
  }
  return 0;
}
mode_config* requestConfig(){
  mode_config *noData;
  mode_config *config;
  if(modeConfigQueue != 0){
    xQueueSend( modeConfigQueueAck, ( void * ) &noData, ( TickType_t ) 0 );
    if( xQueueReceive( modeConfigQueue, &config,(TickType_t)1000)){
      printf("received conf\n");
      return config;
    }
  }
  printf("failed to receive");
  return NULL;
}

void sendAck(){
  mode_config *noData;
  xQueueSend( modeConfigQueueAck, ( void * ) &noData, ( TickType_t ) 0 );
}

void updateRates(rates_struct *r, mode_config mode_conf){
  if(mode_conf.walk)
    r->walkRateMs   = 1000/(((float)(mode_conf.walkRate)));
  if(mode_conf.fade)
    r->fadeRateMs   = 1000/(((float)(mode_conf.fadeRate)));
  if(mode_conf.cycleConfig)
    r->configRateMs = 1000/(((float)(mode_conf.configRate)));
  if(mode_conf.debugRate > 0)
    r->debugRateMs  = 1000/(((float)(mode_conf.debugRate)));
  if(mode_conf.music)
    r->musicRateMs = 1000/(((float)(mode_conf.musicRate)));
}
