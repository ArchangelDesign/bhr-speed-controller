#pragma once

#include <Arduino.h>
#include "global_state.h"

// Button interface for Arduino Nano AVR variant
// 3 buttons: Start/Stop, Speed Up, Speed Down

#ifdef ARDUINO_NANO_AVR

// Pin definitions (from platformio.ini)
#ifndef BUTTON_START_STOP
#define BUTTON_START_STOP 3
#endif

#ifndef BUTTON_SPEED_UP
#define BUTTON_SPEED_UP 4
#endif

#ifndef BUTTON_SPEED_DOWN
#define BUTTON_SPEED_DOWN 5
#endif

// Button debounce and behavior
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_HOLD_MS 500
#define BUTTON_REPEAT_MS 200

// Button states
struct ButtonState {
    bool pressed;
    bool held;
    uint32_t pressTime;
    uint32_t lastRepeatTime;
    bool lastState;
};

// Initialize button interface
void setupButtons();

// Process button inputs (call this in loop)
void handleButtons();

#endif // ARDUINO_NANO_AVR
