#ifndef WS813B_H_
#define WS813B_H_
#include "driver/rmt.h"
#include <complex.h>

// Engine
#define UPDATE_FREQ    1000
#define UPDATE_FREQ_MS 1
/* #define UPDATE_FREQ_MS ((1000/UPDATE_FREQ)) */

// fft & music
#define SAMPLE_RATE_US 50       // 1000000 / 20000
#define SAMPLE_RATE    20000
#define N_SAMPLES      256
#define N_FREQS        12       // n of frequency "summations" to visualize. max 12 100-5k Hz
#define AVG_OFFSET     7
#define N_FFT_COLORS   4
#define MIN_AMP        1.5      // TODO This should be a multiplier not the actual value

// resist led change
#define M_RESISTANCE 5
#define L_RESISTANCE 7
#define H_RESISTANCE 3

// Bool values
#define TRUE    1
#define FALSE   0

// Debug
#define DEBUG   0
#define W_DEBUG 0
#define F_DEBUG 0

#if W_DEBUG > 0
# define WALK_DEBUG(x) printf x
#else
# define WALK_DEBUG(x) do {} while (0)
#endif

#if F_DEBUG > 0
# define fade_debug(x) printf x
#else
# define FADE_DEBUG(x) do {} while (0)
#endif


#if DEBUG > 0
# define DPRINT(x) printf x
#else
# define DPRINT(x) do {} while (0)
#endif

/* typedef uint8_t bool; */



struct led_struct
{
  rmt_item32_t item[24];
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t fadeR;
  uint8_t fadeG;
  uint8_t fadeB;
  bool dir;

} led_struct;


struct color_t
{
  uint8_t red;
  uint8_t blue;
  uint8_t green;
} color_t;

struct mode_config
{
  bool     step;
  bool     fade;
  bool     pulse;
  bool     walk;
  bool     music;
  bool     musicMode1;
  bool     musicMode2;
  float    configRate;
  int16_t  section_offset;
  uint8_t  fadeRate;
  uint8_t  fadeIterations;      // TODO rename this to something more relevant
  uint8_t  fadeDir;
  uint8_t  fadeWalk;
  uint8_t  fadeWalkRate;
  uint8_t  fadeWalkFreq;
  uint8_t  pulseRate;
  uint8_t  section_length;
  uint8_t  smooth;
  uint8_t  cycleConfig;
  uint8_t  nOfConfigs;
  uint32_t length;
  uint16_t walkRate;
  uint16_t musicRate;
  uint16_t debugRate;

  struct color_t *section_colors;
} mode_config;

static const rmt_item32_t setItem[] =
{
{{{
   500,
   0,
   500,
   0
}}}
};

static const rmt_item32_t oneItem[] =
{
{{{
   8,
   1,
   4,
   0
}}}
};

static const rmt_item32_t zeroItem[] =
{
{{{
   4,
   1,
   8,
   0
}}}
};

rmt_config_t rmt_conf;
struct led_struct *leds;



void initRmt(struct mode_config *mode_conf, uint8_t ledPin);
void stepFade(struct led_struct *led, struct mode_config  conf,
              uint8_t rTarget, uint8_t bTarget, uint8_t gTarget);
void initColors(struct mode_config *mode_conf, const struct color_t* color);
void ledEngine(struct mode_config  *mode_conf);
void outputLeds(struct mode_config  mode_conf);
void setLed(rmt_item32_t *item, uint8_t red, uint8_t blue, uint8_t green);
void setRed(uint8_t brightness, rmt_item32_t *item);
void setBlue(uint8_t brightness, rmt_item32_t *item);
void setGreen(uint8_t brightness, rmt_item32_t *item);
void setBrightness(uint8_t brightness, uint8_t start, uint8_t stop, rmt_item32_t *item);
void printDuration0(rmt_item32_t *item);
void stepForward(struct mode_config  *conf);
void setLeds(struct mode_config  conf);
void pulse(struct mode_config  conf);
void setSectionColors(struct mode_config  conf);
void fadeWalk(struct mode_config  conf);
void fadeTo(struct mode_config* conf);
void fadeZero(struct mode_config *conf);
void resetModeConfigs(struct mode_config* conf, uint8_t nConfigs, uint16_t nleds, uint8_t nSections);
void repeatModeZero(struct mode_config *conf);
void welcome();
void initAdc();
void fft(float complex x[], float complex y[], int N, int step, int offset);
void adcErrCtrl(esp_err_t r);
void startAdc(float complex *out , float complex *copy);
void fbinToFreq(float complex *in , float *out);
void scaleAmpRelative(float *power, uint8_t *amplitudeColor, float *color_brightness, float *relativeAmp);
void music_mode1(uint8_t *amplitudeColor, float *brightness, struct mode_config conf);
void music_mode2(float *relativeAmp, struct mode_config conf);
void resistLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float resist);
void resistLowerLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float resist);
void variableResistLedChange(uint8_t r, uint8_t g, uint8_t b, uint16_t led, float l_resist, float h_resist);










#endif
