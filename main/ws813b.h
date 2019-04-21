#ifndef WS813B_H_
#define WS813B_H_
#include "driver/rmt.h"
#define UPDATE_FREQ 1000
#define UPDATE_FREQ_MS ((1000/UPDATE_FREQ))

/* #define DEBUG */
#ifdef DEBUG
# define DPRINT(x) printf x
#else
# define DPRINT(x) do {} while (0)
#endif

struct led_struct
{
  rmt_item32_t item[24];
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t fadeR;
  uint8_t fadeG;
  uint8_t fadeB;
  uint8_t dir;

} led_struct;



struct section_colors_t
{
  uint8_t red;
  uint8_t blue;
  uint8_t green;
} section_colors_t;

struct mode_config
{
  uint32_t length;
  uint32_t configRate;
  uint16_t walk_rate;
  uint16_t debugRate;
  int16_t section_offset;
  uint8_t step;
  uint8_t fade;
  uint8_t fadeRate;
  uint8_t fadeIteration; // TODO rename this to something more relevant
  uint8_t fadeDir;
  uint8_t fadeWalk;
  uint8_t fadeWalkRate;
  uint8_t fadeWalkFreq;
  uint8_t pulse;
  uint8_t pulseRate;
  uint8_t section_length;
  uint8_t walk;
  uint8_t smooth;
  uint8_t cycleConfig;
  uint8_t nOfConfigs;
  struct section_colors_t *section_colors;
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



void init();
void stepFade(struct led_struct *led, struct mode_config  conf,
              uint8_t rTarget, uint8_t bTarget, uint8_t gTarget);
void initColors(struct mode_config *mode_conf, struct section_colors_t* color);
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
void setSectionFadeColors(struct mode_config  conf, );
void setFadeColorsSection(struct mode_config  conf, );
void fadeTo(struct mode_config* conf);
void fadeZero(struct mode_config *conf);
void resetModeConfigs(struct mode_config* conf, uint8_t nConfigs, uint16_t nleds, uint8_t nSections);
void repeatModeZero(struct mode_config *conf);





#endif
