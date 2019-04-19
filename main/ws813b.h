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
  uint8_t pulseDir;

} led_struct;

struct section_colors_t
{
  uint8_t red;
  uint8_t blue;
  uint8_t green;
} section_colors_t;

struct led_config
{
  uint32_t length;
  uint32_t configRate;
  uint16_t walk_rate;
  uint16_t debugRate;
  int16_t section_offset;
  uint8_t step;
  uint8_t fade;
  uint8_t fadeRate;
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
} led_config;

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



void init();
void initColors(struct led_config* led_conf, struct section_colors_t* color);
void ledEngine(struct led_config *led_conf, struct led_struct* leds);
void outputLeds(struct led_struct *leds, struct led_config led_conf);
void setLed(rmt_item32_t *item, uint8_t red, uint8_t blue, uint8_t green);
void setRed(uint8_t brightness, rmt_item32_t *item);
void setBlue(uint8_t brightness, rmt_item32_t *item);
void setGreen(uint8_t brightness, rmt_item32_t *item);
void setBrightness(uint8_t brightness, uint8_t start, uint8_t stop, rmt_item32_t *item);
void printDuration0(rmt_item32_t *item);
void stepForward(struct led_struct *leds, struct led_config *conf);
void setLeds(struct led_struct *leds, struct led_config conf);
void pulse(struct led_struct * leds, struct led_config conf);
void setSectionColors(struct led_config conf, struct led_struct *leds);
int fade(struct led_config conf, struct led_struct *leds, uint8_t it);
void fadeWalk(struct led_struct *leds, struct led_config conf);
void setSectionFadeColors(struct led_config conf, struct led_struct *leds);
void setFadeColorsSection(struct led_config conf, struct led_struct *leds);
void stepFade(struct led_struct *led, struct led_config conf,
              uint8_t rTarget, uint8_t bTarget, uint8_t gTarget);


#endif
