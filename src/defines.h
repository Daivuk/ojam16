#pragma once
#include <onut/Maths.h>

#define PLANET_SIZE 100
#define PLANET_SIDES 360
#define ZOOM 100
#define ATMOSPHERES_COUNT 4
#define ATMOSPHERES_SCALE 0.05f
#define STAR_COUNT 300

static const Color PLANET_COLOR = Color(0, .5f, 0, 1).AdjustedSaturation(.5f);
static const Color ATMOSPHERE_BASE_COLOR = Color(0, .75f, 1, 1).AdjustedSaturation(.5f);
static const Color ATMOSPHERE_COLORS[ATMOSPHERES_COUNT] = {
    ATMOSPHERE_BASE_COLOR,
    ATMOSPHERE_BASE_COLOR * 0.75f,
    ATMOSPHERE_BASE_COLOR * 0.5f,
    ATMOSPHERE_BASE_COLOR * .25f
};
static const Color ROCKET_COLOR = Color(1, .95f, .9f, 1).AdjustedSaturation(.5f);
static const Color ROCKET_SHADOW_COLOR = ROCKET_COLOR * .6f;
static const Color ENGINE_COLOR = Color(.16f, .18f, .2f, 1).AdjustedSaturation(.5f);
static const Color ENGINE_SHADOW_COLOR = ENGINE_COLOR * .6f;

