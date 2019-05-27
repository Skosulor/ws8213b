#include "ws813b.h"

#define RGB 3

/* Some predefined colors */

/* feel free to add more and */
/* make a pull request */


static const struct color_t color_red = {
  .red = 0,
  .blue = 255,
  .green = 0,
};
static const struct color_t color_blue = {
  .red = 0,
  .blue = 255,
  .green = 0,
};
static const struct color_t color_green = {
  .red = 0,
  .blue = 0,
  .green = 255,
};
static const struct color_t color_white = {
  .red = 255,
  .blue = 255,
  .green = 255,
};

static const struct color_t color_black = {
  .red = 0,
  .blue = 0,
  .green = 0,
};

static const struct color_t color_Purple  = {
  .red = 255,
  .green = 0,
  .blue = 200,
};

static const struct color_t color_Yellow = {
  .red = 255,
  .green = 125,
  .blue = 0,
};

static const struct color_t color_Green = {
  .red = 0,
  .green = 255,
  .blue = 0,
};

static const struct color_t color_light_purple = {
  .red = 255,
  .green = 0,
  .blue = 100,
};

static const struct color_t color_Orange = {
  .red = 255,
  .green = 50,
  .blue = 0,
};

static const struct color_t color_light_pink = {
  .red = 255,
  .green = 40,
  .blue = 40,
};

static const struct color_t color_pink = {
  .red = 255,
  .green = 0,
  .blue = 25,
};

static const struct color_t color_cyan = {
  .red = 200,
  .green = 255,
  .blue = 200
};

static const struct color_t music_colors[N_FFT_COLORS] = {


  {
    .red = 255,
    .green = 0,
    .blue = 0,
  },

  {
    .red = 0,
    .green = 0,
    .blue = 255,
  },

  {
    .red = 0,
    .green = 255,
    .blue = 0,
  },

  {
    .red = 30,
    .green = 30,
    .blue = 30,
  }};

static const struct color_t testColors[10] = {

  {
    .red = 255,      // Purple
    .green = 0,
    .blue = 200,
  },

  {
    .red = 255,     // Yellow
    .green = 125,
    .blue = 0,
  },

  {
    .red = 0,      // Green
    .green = 255,
    .blue = 0,
  },

  {
    .red = 255,      // LIGHT PURPLE
    .green = 0,
    .blue = 100,
  },

  {
    .red = 255,      // Orange
    .green = 50,
    .blue = 0,
  },

  {
    .red = 255,      // LIGHT PINK
    .green = 40,
    .blue = 40,
  },

  {
    .red = 255,      // PINK
    .green = 0,
    .blue = 25,
  },

  {
    .red = 0,
    .green = 55,
    .blue = 255,
  },


  {
    .red = 200,
    .green = 255,    // CYAN
    .blue = 200,
  },

  {
    .red = 0,
    .green = 100,
    .blue = 0,
  }
};






