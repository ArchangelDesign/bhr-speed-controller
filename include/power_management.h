#pragma once

#include <Arduino.h>

// GPIO 0 is the BOOT button on ESP32 (active LOW)
#define BOOT_BUTTON 0

// Power management
void setupPowerManagement();
void enterDeepSleep();
void checkWakeupReason();
