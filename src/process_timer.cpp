#include "process_timer.h"

void initProcessTimer() {
    g_state.processTimerSeconds = 0;
    g_state.processTimerStartMillis = 0;
    g_state.processTimerRunning = false;
}

void startProcessTimer() {
    if (!g_state.processTimerRunning) {
        g_state.processTimerRunning = true;
        g_state.processTimerStartMillis = millis();
    }
}

void pauseProcessTimer() {
    if (g_state.processTimerRunning) {
        // Add elapsed time to accumulated seconds
        uint32_t elapsed = millis() - g_state.processTimerStartMillis;
        g_state.processTimerSeconds += elapsed / 1000;
        g_state.processTimerRunning = false;
    }
}

void updateProcessTimer() {
    // This function is called regularly to check if a second has passed
    // It doesn't accumulate fractional seconds until the timer is paused
    if (g_state.processTimerRunning) {
        uint32_t elapsed = millis() - g_state.processTimerStartMillis;
        // Check if at least one second has passed
        if (elapsed >= 1000) {
            // Add complete seconds to the accumulated time
            uint32_t completeSeconds = elapsed / 1000;
            g_state.processTimerSeconds += completeSeconds;
            // Reset start time to current time minus any fractional second
            g_state.processTimerStartMillis = millis() - (elapsed % 1000);
        }
    }
}

void resetProcessTimer() {
    g_state.processTimerSeconds = 0;
    g_state.processTimerStartMillis = millis();
    g_state.processTimerRunning = false;
}

void getProcessTimerString(char* buffer, size_t bufferSize) {
    uint32_t totalSeconds = g_state.processTimerSeconds;
    
    // If timer is running, add current elapsed time
    if (g_state.processTimerRunning) {
        uint32_t currentElapsed = (millis() - g_state.processTimerStartMillis) / 1000;
        totalSeconds += currentElapsed;
    }
    
    uint32_t hours = totalSeconds / 3600;
    uint32_t minutes = (totalSeconds % 3600) / 60;
    uint32_t seconds = totalSeconds % 60;
    
    if (hours > 0) {
        snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    } else {
        snprintf(buffer, bufferSize, "%02lu:%02lu", minutes, seconds);
    }
}
