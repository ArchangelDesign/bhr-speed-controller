#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "global_state.h"

// Display setup
void setupDisplay();
void updateDisplay();
void drawFullUI();

// Button areas for touch
struct Button {
    int16_t x, y, w, h;
    const char* label;
    uint16_t color;
};

enum ButtonID {
    BTN_START_STOP,
    BTN_MODE,
    BTN_POWER_UP,
    BTN_POWER_DOWN,
    BTN_RPM_UP,
    BTN_RPM_DOWN,
    BTN_SLEEP,
    BTN_COUNT
};

extern Button g_buttons[BTN_COUNT];
extern TFT_eSPI tft;

// Helper functions
void drawButton(const Button& btn, bool pressed = false);
void drawStatus();
