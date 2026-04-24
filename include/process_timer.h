#pragma once

#include <Arduino.h>
#include "global_state.h"

// Initialize process timer
void initProcessTimer();

// Start the timer (when motor starts)
void startProcessTimer();

// Pause the timer (when motor stops)
void pauseProcessTimer();

// Update timer (call regularly to update seconds)
void updateProcessTimer();

// Reset timer to zero
void resetProcessTimer();

// Get formatted time string (HH:MM:SS)
void getProcessTimerString(char* buffer, size_t bufferSize);

