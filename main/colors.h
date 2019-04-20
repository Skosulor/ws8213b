#include "ws813b.h"

/* Some predefined colors */

/* feel free to add more and */
/* make a pull request */

static const struct section_colors_t white = {
  .red = 255,
  .blue = 255,
  .green = 255,
};

static const struct section_colors_t black = {
  .red = 0,
  .blue = 0,
  .green = 0,
};

static const struct section_colors_t Purple  = {
  .red = 255,
  .green = 0,
  .blue = 200,
};

static const struct section_colors_t Yellow = {
  .red = 255,
  .green = 125,
  .blue = 0,
};

static const struct section_colors_t Green = {
  .red = 0,
  .green = 255,
  .blue = 0,
};

static const struct section_colors_t light_purple = {
  .red = 255,
  .green = 0,
  .blue = 100,
};

static const struct section_colors_t Orange = {
  .red = 255,
  .green = 50,
  .blue = 0,
};

static const struct section_colors_t light_pink = {
  .red = 255,
  .green = 40,
  .blue = 40,
};

static const struct section_colors_t PINK = {
  .red = 255,
  .green = 0,
  .blue = 25,
};

static const struct section_colors_t cyan = {
  .red = 200,
  .green = 255,
  .blue = 200
};

static const struct section_colors_t testColors[10] = {

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






