#pragma once

#include <Arduino.h>
#include <XPT2046_Touchscreen.h>
#include "global_state.h"
#include "display.h"

// Touch setup and handling
void setupTouch();
void handleTouch();

// Check if a button was touched
int getTouchedButton(int16_t x, int16_t y);

extern XPT2046_Touchscreen ts;
