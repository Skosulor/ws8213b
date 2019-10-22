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

  leds = malloc(sizeof(led_struct) * mode_conf[0].nLeds);
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

void ledEngineTask(void *param){

  int i,j;
  mode_config *conf = (mode_config*) param;
  mode_config configCopy[conf[0].cycleConfig.nConfigs];

  for(i = 0; i < conf[0].cycleConfig.nConfigs; i++){
    configCopy[i].section_colors = (malloc(conf[0].sectionLength * sizeof(color_t)));
    configCopy[i] = conf[i];

    for(j = 0; j < configCopy[i].sectionLength; j ++){
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
  static TickType_t gravTick;

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
   r.gravRateMs   = 1000;
   r.musicRateMs  = 1;

  // Music mode
  bool    adcInitiated = FALSE;
  float   musicRelativeAmp[N_FREQS];
  float   musicBrightness[N_FREQS];
  uint8_t musicAmplitudeColor[N_FREQS];
  float   complex samples[N_SAMPLES];
  float   complex copy[N_SAMPLES];
  float   freqAmps[N_FREQS];


  if(mode_conf[cc].music.mode1 || mode_conf[cc].music.mode2)
    mode_conf[cc].music.on = TRUE;

  updateRates(&r, mode_conf[cc]);

  DPRINT(("----------------------  \n Initial rates in mS\n---------------------- \n "));
  DPRINT(("walkRateMs %d\n" , r.walkRateMs));
  DPRINT(("r.fadeRateMs %d\n" , r.fadeRateMs));
  DPRINT(("r.configRateMs %d\n" , r.configRateMs));
  DPRINT(("r.musicRateMs %d\n" , r.musicRateMs));
  DPRINT(("----------------------  \n Initial rates in mS \n---------------------- \n\n "));

  int64_t bt = 0;
  int64_t et = 0;

  cc = nc;
  pc = nc;
  nc = mode_conf[0].cycleConfig.nConfigs;
  debugTick = DEBUG_RATE + 1;


    for(i = 0; i < mode_conf[cc].smooth; i++){
      fadeWalk(mode_conf[cc]);
    }

  for(i = 0; i < mode_conf[0].nLeds; i ++){
    leds[i].fadeR = leds[i].r;
    leds[i].fadeG = leds[i].g;
    leds[i].fadeB = leds[i].b;
  }

  setLeds(mode_conf[cc]);
  outputLeds(mode_conf[cc]);


  if(mode_conf[cc].music.on){
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

    if(LastWakeTime - debugTick > DEBUG_RATE){
      debugTick = LastWakeTime;

      DPRINT(("----------------------- Debug info ----------------------- \n"));
      DPRINT(("current conf: %d\nwalk:         %d\nfade:         %d\nsmooth:       %d\nconfigTick:  %d\n",
              cc, mode_conf[cc].walk.on, mode_conf[cc].fade.on, mode_conf[cc].smooth, configTick));
      DPRINT(("----------------------------------------------------------- \n\n "));


      DPRINT(("----------------------  Timing info ---------------------- \n"));
      DPRINT(("Time taken to output leds: %f mS\n", ((float)(et-bt))/1000));
      DPRINT(("---------------------------------------------------------- \n\n"));

      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));
      FADE_DEBUG(("Current conf: %d\n Fade: %d\n fadeRate: %d\n ", cc, mode_conf[cc].fade.on, mode_conf[cc].fade.rate));
      FADE_DEBUG(("fadeIteration: %d\n FadeDir: %d\n ", mode_conf[cc].fade.iteration, mode_conf[cc].fade.dir));
      FADE_DEBUG(("---------------------- Fade ---------------------- \n"));

    }

    /* ------------- */
    /* Config Update */
    /* --------------*/

    if((LastWakeTime - configTick) > r.configRateMs && mode_conf[cc].cycleConfig.on){
      configTick = LastWakeTime;
      pc = cc;
      cc = ((cc + 1) % nc);

      mode_conf[cc].fade.dir = mode_conf[pc].fade.dir;

      if(mode_conf[cc].music.mode1 || mode_conf[cc].music.mode2)
        mode_conf[cc].music.on = TRUE;
      else
        mode_conf[cc].music.on = FALSE;

      if(mode_conf[cc].music.on && !adcInitiated){
        initAdc();
        adcInitiated = TRUE;
      }
      updateRates(&r, mode_conf[cc]);
    }

    /* --------- */
    /* Grav mode */
    /* ----------*/

    if((LastWakeTime - gravTick) >= r.gravRateMs && mode_conf[cc].simGrav.on){
      gravTick = LastWakeTime;
      simulateGravity(mode_conf[cc]);
      setLeds(mode_conf[cc]);
      ledsUpdated = TRUE;
    }

    /* --------- */
    /* fade mode */
    /* ----------*/

    if((LastWakeTime - fadeTick) >= r.fadeRateMs && mode_conf[cc].fade.on){
      fadeTick = LastWakeTime;

      if(mode_conf[cc].fade.dir == 0){
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

    if((LastWakeTime - walkTick) >= r.walkRateMs && mode_conf[cc].walk.on){
      walkTick = LastWakeTime;
      if(mode_conf[cc].walk.dir)
        stepForward(&mode_conf[cc]);
      else
        stepBackward(&mode_conf[cc]);
      setLeds(mode_conf[cc]);
      ledsUpdated = TRUE;
    }

    /* -------------- */
    /* fade_walk mode */
    /* ---------------*/

    // TODO change behaviour of this one
    if(fadeWalk_t >= mode_conf[cc].fadeWalk.rate && mode_conf[cc].fadeWalk.on){
      fadeWalk_t = 0;
      fadeWalk(mode_conf[cc]);
      setLeds(mode_conf[cc]);
      ledsUpdated = TRUE;
      /* outputLeds(mode_conf[cc]); */
    }
    /* --------- */
    /*    FFT    */
    /* ----------*/

    if((LastWakeTime - musicTick) >= r.musicRateMs && mode_conf[cc].music.on){
      musicTick = LastWakeTime;
      startAdc(samples, copy);
      fft(samples, copy, N_SAMPLES, 1, 0);
      fbinToFreq(samples, freqAmps);
      scaleAmpRelative(freqAmps, musicAmplitudeColor, musicBrightness, musicRelativeAmp);

      if(mode_conf[cc].music.mode1)
        musicMode1(musicAmplitudeColor, musicBrightness, mode_conf[cc]);

      if(mode_conf[cc].music.mode2)
        musicMode2(musicRelativeAmp, mode_conf[cc]);


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

      if(mode_conf[cc].mirror.on){
        mirror(mode_conf[cc]);
        /* setLeds(mode_conf[cc]); */
      }

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

void musicMode2(float *relativeAmp, mode_config conf){

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

  for(i = 0; i <  conf.nLeds; i ++){
    variableResistLedChange(r, g, b, i, L_RESISTANCE, H_RESISTANCE);
  }

  for(i = 0; i < RGB; i++){
      rgbBrightness[i] = 0;
  }
}


void musicMode1(uint8_t *amplitudeColor, float *brightness, mode_config conf){
  uint16_t i,j, pos;
  uint8_t r,g,b;

  for(i = 0; i < N_FREQS; i ++){
    for(j = i*(conf.nLeds/(N_FREQS)); j <  (i+1)*(conf.nLeds/(N_FREQS)); j ++){

      r = (int)((float)music_colors[amplitudeColor[i]].red   * brightness[i]);
      g = (int)((float)music_colors[amplitudeColor[i]].green * brightness[i]);
      b = (int)((float)music_colors[amplitudeColor[i]].blue  * brightness[i]);
      pos = getOffsetPos(conf, j);
      variableResistLedChange(r, g, b, pos, L_RESISTANCE, H_RESISTANCE);
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

uint16_t getOffsetPos(mode_config conf, uint16_t pos){
  return ((pos + conf.ledOffset) % conf.nLeds);
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

void simulateGravity(mode_config conf){

  static int64_t t1, t2   = 0;
  static int16_t pos      = 0;
  static int16_t oldPos   = 0;
  static float   velocity = 0;  // m/s
  static float   fallDist = 0;
  float          tDiff;

  //TODO support for multipe drops generated in rt
  t1 = esp_timer_get_time(); // microseconds
  tDiff = ((float)t1 - (float)t2) / 1000000;
  /* tDiff = (t1 - t2); */
  /* if(tDiff > 1){ */
    /* tDiff = 0; */
    /* printf("What what\n"); */
  /* } */

  // Clear current led based on fallDist
  leds[pos].r = 0;
  leds[pos].b = 0;
  leds[pos].g = 0;

  velocity += conf.simGrav.gravity * tDiff;
  fallDist += velocity * tDiff;

  /* fallDist = fallDist + velocity * tDiff + 0.5 * conf.simGrav.gravity * pow(tDiff, 2.0); */
  /* velocity = velocity + tDiff/fallDist; */

  // Calculate current position in cm
  pos = (uint16_t)(fallDist * 100);
  printf("Pos: %d\n", pos);

  // Calculate pos in terms of led n
  pos = ((float)conf.simGrav.ledsPerMeter / 100.0)  * pos;

  /* pos = getOffsetPos(conf, pos); // TODO: func returns uint16_t, need signed value */

  printf("Pos: %d\n", pos);

  if(pos >= conf.nLeds){
    pos = 0;
    fallDist = 0;
    velocity = 0;
    printf("zero\n");
  }
  else if(pos < 0){
    pos = conf.nLeds - 1;
    fallDist = 0;
    velocity = 0;
    printf("less than zero\n");
  }

  leds[pos].r = 255;
  leds[pos].b = 120;
  leds[pos].g = 0;

  printf("Pos: %d\n", pos);
  printf("FallDist: %f\n, velocity %f, tdiff %f\n", fallDist, velocity, tDiff);

  if(oldPos == pos)
    t2 = t1 + tDiff;
  else
    t2 = t1;
  oldPos = pos;

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

    conf[i].nLeds                   = nleds;
    conf[i].sectionLength           = nSections;
    conf[i].ledOffset               = 0;
    conf[i].smooth                  = 0;
    // CycleConfig
    conf[i].cycleConfig.nConfigs    = nConfigs;
    conf[i].cycleConfig.on          = 0;
    conf[i].cycleConfig.rate        = 0;
    // FadeWalk
    conf[i].fadeWalk.on             = 0;
    conf[i].fadeWalk.freq           = 0;
    conf[i].fadeWalk.rate           = 0;
    // Fade
    conf[i].fade.iteration          = 0; // TODO rename this to something relevant
    conf[i].fade.rate               = 0;
    conf[i].fade.on                 = FALSE;
    conf[i].fade.dir                = 0;
    // Walk
    conf[i].walk.on                 = FALSE;
    conf[i].walk.rate               = 0;
    conf[i].walk.dir                = FALSE;
    // Mirror
    conf[i].mirror.on               = FALSE;
    conf[i].mirror.mirrors          = 1;
    conf[i].mirror.sharedReflection = FALSE;
    // Grav
    conf[i].simGrav.on               = FALSE;
    conf[i].simGrav.gravity          = 9.82;
    conf[i].simGrav.newDropRate      = 0;
    conf[i].simGrav.ledsPerMeter     = 60;
    conf[i].simGrav.t                = 0;
    conf[i].simGrav.rate             = 0;
    // Music
    conf[i].music.rate              = 0;
    conf[i].music.on                = FALSE;
    conf[i].music.mode1             = FALSE;
    conf[i].music.mode2             = FALSE;

    conf[i].section_colors = (malloc(conf[0].sectionLength * sizeof(color_t)));
  }
}

void repeatModeZero(mode_config *conf){
  int i;
  for(i = 1; i < conf[0].cycleConfig.nConfigs; i++)
    conf[i] = conf[0];
}

void initColors(mode_config  *mode_conf, const color_t *color){
  int i;
  for(i = 0; i < mode_conf->sectionLength; i ++){
    mode_conf->section_colors[i] = color[i];
  }
}

void fadeWalk( mode_config  conf){
  int i;
  for(i = 0; i < conf.nLeds; i++){
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
    leds[i].r = leds[i].r - ((leds[i].r - leds[(i+1)%conf.nLeds].r)/(3));
    leds[i].g = leds[i].g - ((leds[i].g - leds[(i+1)%conf.nLeds].g)/(3));
    leds[i].b = leds[i].b - ((leds[i].b - leds[(i+1)%conf.nLeds].b)/(3));
    /* printf("Red: %d, i: %d\n", leds[i].r, i); */
  }
}

void mirror(mode_config conf){
  int i, j;
  int mirrorSize, stepSize, source, target,target2;
  stepSize = (conf.nLeds/conf.mirror.mirrors);
  mirrorSize = stepSize/2;

  for(i = 0; i < conf.mirror.mirrors; i++){
    target = (i+1)*stepSize;
    for(j = i*stepSize; j < i*stepSize + mirrorSize; j++){
      source = j;
      target--;
      copyRmtItem(source, target);

      if(conf.mirror.sharedReflection && stepSize*(i+1)+(j-i*stepSize) < conf.nLeds)

        {
          target2 = stepSize*(i+1)+(j-i*stepSize);
          copyRmtItem(j, target2);
        }
    }
  }
}

void copyRmtItem(uint16_t source, uint16_t target){
  int i;
  for(i = 0; i < ITEMS_PER_LED; i ++){
    leds[target].item[i] = leds[source].item[i];
  }
}


void fadeZero(mode_config *conf){
  int i;
  static uint8_t it = 0;
  for(i = 0; i < conf->nLeds; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - 0)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - 0)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - 0)/(64 - it));
  }
  it++;
  if(it == conf->fade.iteration){
    it = 0;
    conf->fade.dir = 0;
  }
}

void fadeTo(mode_config *conf){
  int i;
  static uint8_t it = 0;

  for(i = 0; i < conf->nLeds; i++){
    // TODO define a variable for 128
    leds[i].r = leds[i].r - ((leds[i].r - leds[i].fadeR)/(64 - it));
    leds[i].g = leds[i].g - ((leds[i].g - leds[i].fadeG)/(64 - it));
    leds[i].b = leds[i].b - ((leds[i].b - leds[i].fadeB)/(64 - it));
  }

  it++;
  if(it == conf->fade.iteration){
    it = 0;
    conf->fade.dir = 1;
  }
}

void setSectionColors(mode_config  conf){
  int i,j;
  for(i = 0; i < conf.sectionLength; i ++){
    for(j = i*(conf.nLeds/conf.sectionLength); j <  (i+1)*(conf.nLeds/conf.sectionLength); j ++){
      leds[j].r = conf.section_colors[i].red;
      leds[j].g = conf.section_colors[i].green;
      leds[j].b = conf.section_colors[i].blue;
    }
  }
}

void outputLeds(mode_config  mode_conf){
  int i;
  for(i = 0; i < mode_conf.nLeds; i ++){
    ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, leds[i].item, 24, 1));
  }

  ESP_ERROR_CHECK(rmt_write_items(rmt_conf.channel, setItem, 1, 1));

}

void setLeds(mode_config  conf){
  int i;
  for(i = 0; i < conf.nLeds; i ++){
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
  struct led_struct temp = leds[conf->nLeds-1];
  /* printf("led one red = %d\n", leds[0].r); */
  for(i = conf->nLeds-1; i < conf->nLeds; i --){
    leds[i] = leds[((i-1) % conf->nLeds)];
  }
  leds[0] = temp;
  conf->ledOffset += 1;
  if(conf->ledOffset <= -(int16_t)conf->nLeds)
    conf->ledOffset = 0;
}
void stepBackward(mode_config *conf){
  int i;
  struct led_struct temp = leds[0];
  /* printf("led one red = %d\n", leds[0].r); */
  for(i = 0; i < conf->nLeds; i ++){
    leds[i] = leds[((i+1) % conf->nLeds)];
  }
  leds[conf->nLeds - 1] = temp;
  conf->ledOffset -= 1;
  if(conf->ledOffset <= -(int16_t)conf->nLeds)
    conf->ledOffset = 0;
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
  if(mode_conf.walk.on)
    r->walkRateMs   = 1000/(((float)(mode_conf.walk.rate)));
  if(mode_conf.fade.on)
    r->fadeRateMs   = 1000/(((float)(mode_conf.fade.rate)));
  if(mode_conf.cycleConfig.on)
    r->configRateMs = 1000/(((float)(mode_conf.cycleConfig.rate)));
  if(mode_conf.music.on)
    r->musicRateMs = 1000/(((float)(mode_conf.music.rate)));
  if(mode_conf.simGrav.on)
    r->gravRateMs = 1000/(((float)(mode_conf.simGrav.rate)));
}

