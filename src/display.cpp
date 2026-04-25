#include "display.h"
#include "process_timer.h"

TFT_eSPI tft = TFT_eSPI();

// Track previous values to avoid unnecessary redraws
struct DisplayCache {
    ControlMode lastMode = MODE_FIXED_POWER;
    MotorState lastMotorState = MOTOR_STOPPED;
    float lastCurrentPower = -1.0f;
    float lastCurrentRPM = -1.0f;
    float lastTargetPower = -1.0f;
    float lastTargetRPM = -1.0f;
    bool lastRpmSensorEnabled = true;
    uint32_t lastProcessTimerSeconds = 0;
};
static DisplayCache displayCache;

// Define button layout for 320x240 screen (landscape)
// Status area: 0-70, Button area: 70-240
Button g_buttons[BTN_COUNT] = {
    {10, 185, 300, 45, "START", TFT_GREEN},      // BTN_START_STOP
    {10, 135, 300, 45, "Mode: Power", TFT_BLUE}, // BTN_MODE
    {10, 85, 70, 40, "+", TFT_CYAN},             // BTN_POWER_UP
    {90, 85, 70, 40, "-", TFT_CYAN},             // BTN_POWER_DOWN
    {170, 85, 70, 40, "+", TFT_MAGENTA},         // BTN_RPM_UP
    {250, 85, 60, 40, "-", TFT_MAGENTA},         // BTN_RPM_DOWN
    {250, 5, 60, 25, "SLEEP", TFT_ORANGE},       // BTN_SLEEP
};

void setupDisplay() {
    tft.init();
    tft.setRotation(1);  // Landscape mode (320x240)
    tft.fillScreen(TFT_BLACK);
    
    // Initialize button labels
    g_buttons[BTN_START_STOP].label = "START";
    g_buttons[BTN_MODE].label = "Mode: Power";
    
    g_state.needsRedraw = true;
}

void drawButton(const Button& btn, bool pressed) {
    uint16_t bgColor = pressed ? TFT_DARKGREY : btn.color;
    uint16_t textColor = TFT_WHITE;
    
    // Draw button background
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 8, bgColor);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 8, TFT_WHITE);
    
    // Draw button label
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textColor, bgColor);
    tft.drawString(btn.label, btn.x + btn.w/2, btn.y + btn.h/2, 2);
}

void drawStatus() {
    tft.setTextDatum(TL_DATUM);
    char buffer[32];
    
    // Update mode if changed (avoid sleep button area: x=250-310)
    if (displayCache.lastMode != g_state.config.mode) {
        tft.fillRect(5, 3, 110, 18, TFT_BLACK);  // Clear mode area only
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("Mode: ", 5, 3, 2);
        if (g_state.config.mode == MODE_FIXED_POWER) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawString("POWER", 60, 3, 2);
        } else {
            tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
            tft.drawString("PID", 60, 3, 2);
        }
        displayCache.lastMode = g_state.config.mode;
    }
    
    // Update motor state if changed
    if (displayCache.lastMotorState != g_state.motorState) {
        tft.fillRect(120, 3, 80, 18, TFT_BLACK);  // Clear state area only
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString("|", 120, 3, 2);
        if (g_state.motorState == MOTOR_STOPPED) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString("STOP", 135, 3, 2);
        } else if (g_state.motorState == MOTOR_STARTING) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString("START", 135, 3, 2);
        } else if (g_state.motorState == MOTOR_STOPPING) {
            tft.setTextColor(TFT_ORANGE, TFT_BLACK);
            tft.drawString("STOP", 135, 3, 2);
        } else {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString("RUN", 135, 3, 2);
        }
        displayCache.lastMotorState = g_state.motorState;
    }
    
    // Update current power if changed
    if (abs(displayCache.lastCurrentPower - g_state.currentPower) > 0.05f) {
        tft.fillRect(5, 23, 155, 18, TFT_BLACK);  // Clear power area only
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        sprintf(buffer, "Power: %.1f%%", g_state.currentPower);
        tft.drawString(buffer, 5, 23, 2);
        displayCache.lastCurrentPower = g_state.currentPower;
    }
    
    // Update current RPM if changed or sensor state changed
    bool rpmChanged = abs(displayCache.lastCurrentRPM - g_state.currentRPM) > 0.5f;
    bool sensorStateChanged = displayCache.lastRpmSensorEnabled != g_state.config.rpmSensorEnabled;
    if (rpmChanged || sensorStateChanged) {
        tft.fillRect(170, 23, 145, 18, TFT_BLACK);  // Clear RPM area only
        if (g_state.config.rpmSensorEnabled) {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            sprintf(buffer, "RPM: %.0f", g_state.currentRPM);
            tft.drawString(buffer, 170, 23, 2);
        } else {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString("RPM: ---", 170, 23, 2);
        }
        displayCache.lastCurrentRPM = g_state.currentRPM;
        displayCache.lastRpmSensorEnabled = g_state.config.rpmSensorEnabled;
    }
    
    // Update target power if changed
    if (abs(displayCache.lastTargetPower - g_state.config.targetPower) > 0.05f) {
        tft.fillRect(5, 48, 155, 18, TFT_BLACK);  // Clear target power area only
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        sprintf(buffer, "Target: %.0f%%", g_state.config.targetPower);
        tft.drawString(buffer, 5, 48, 2);
        displayCache.lastTargetPower = g_state.config.targetPower;
    }
    
    // Update target RPM if changed and in PID mode
    if (g_state.config.mode == MODE_FIXED_SPEED) {
        if (abs(displayCache.lastTargetRPM - g_state.config.targetRPM) > 0.5f) {
            tft.fillRect(170, 48, 145, 18, TFT_BLACK);  // Clear target RPM area only
            tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
            sprintf(buffer, "%.0f RPM", g_state.config.targetRPM);
            tft.drawString(buffer, 170, 48, 2);
            displayCache.lastTargetRPM = g_state.config.targetRPM;
        }
    } else {
        // Clear target RPM area if mode switched from PID to Power
        if (displayCache.lastTargetRPM >= 0) {
            tft.fillRect(170, 48, 145, 18, TFT_BLACK);
            displayCache.lastTargetRPM = -1.0f;
        }
    }
    
    // Update process timer (always update when running, or when changed)
    uint32_t currentTimerSeconds = g_state.processTimerSeconds;
    if (g_state.processTimerRunning) {
        currentTimerSeconds += (millis() - g_state.processTimerStartMillis) / 1000;
    }
    
    if (displayCache.lastProcessTimerSeconds != currentTimerSeconds) {
        // Position timer on the right side, top row (before sleep button)
        tft.fillRect(195, 3, 48, 18, TFT_BLACK);  // Clear timer area
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        char timerStr[16];
        getProcessTimerString(timerStr, sizeof(timerStr));
        tft.drawString(timerStr, 195, 3, 2);
        displayCache.lastProcessTimerSeconds = currentTimerSeconds;
    }
}

void drawFullUI() {
    tft.fillScreen(TFT_BLACK);
    
    // Reset display cache to force full redraw
    displayCache.lastMode = (ControlMode)!g_state.config.mode;
    displayCache.lastMotorState = (MotorState)!g_state.motorState;
    displayCache.lastCurrentPower = -1.0f;
    displayCache.lastCurrentRPM = -1.0f;
    displayCache.lastTargetPower = -1.0f;
    displayCache.lastTargetRPM = -1.0f;
    displayCache.lastRpmSensorEnabled = !g_state.config.rpmSensorEnabled;
    displayCache.lastProcessTimerSeconds = 0xFFFFFFFF;  // Force timer redraw
    
    // Draw status area
    drawStatus();
    
    // Update button labels based on state
    if (g_state.motorState == MOTOR_STOPPED) {
        g_buttons[BTN_START_STOP].label = "START";
        g_buttons[BTN_START_STOP].color = TFT_GREEN;
    } else {
        g_buttons[BTN_START_STOP].label = "STOP";
        g_buttons[BTN_START_STOP].color = TFT_RED;
    }
    
    if (g_state.config.mode == MODE_FIXED_POWER) {
        g_buttons[BTN_MODE].label = "Mode: Power";
    } else {
        g_buttons[BTN_MODE].label = "Mode: PID";
    }
    
    // Draw all buttons
    for (int i = 0; i < BTN_COUNT; i++) {
        drawButton(g_buttons[i]);
    }
    
    // Draw labels for adjustment buttons
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Power", 50, 72, 1);
    
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.drawString("RPM", 205, 72, 1);
}

void updateDisplay() {
    if (g_state.needsRedraw) {
        drawFullUI();
        g_state.needsRedraw = false;
    } else {
        // Just update the status area
        drawStatus();
    }
}
