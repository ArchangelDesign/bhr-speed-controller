#include "display.h"

TFT_eSPI tft = TFT_eSPI();

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
    // Clear status area (reduced height)
    tft.fillRect(0, 0, 320, 75, TFT_BLACK);
    
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    // Draw mode and state on one line
    tft.drawString("Mode: ", 5, 3, 2);
    if (g_state.config.mode == MODE_FIXED_POWER) {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString("POWER", 60, 3, 2);
    } else {
        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        tft.drawString("PID", 60, 3, 2);
    }
    
    // Draw motor state
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("|", 120, 3, 2);
    if (g_state.motorState == MOTOR_STOPPED) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("STOP", 135, 3, 2);
    } else if (g_state.motorState == MOTOR_STARTING) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("START", 135, 3, 2);
    } else {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("RUN", 135, 3, 2);
    }
    
    // Draw current power and RPM
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buffer[32];
    sprintf(buffer, "Power: %.1f%%", g_state.currentPower);
    tft.drawString(buffer, 5, 23, 2);
    
    // Draw current RPM
    if (g_state.config.rpmSensorEnabled) {
        sprintf(buffer, "RPM: %.0f", g_state.currentRPM);
        tft.drawString(buffer, 170, 23, 2);
    } else {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("RPM: ---", 170, 23, 2);
    }
    
    // Draw target values
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    sprintf(buffer, "Target: %.0f%%", g_state.config.targetPower);
    tft.drawString(buffer, 5, 48, 2);
    
    if (g_state.config.mode == MODE_FIXED_SPEED) {
        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        sprintf(buffer, "%.0f RPM", g_state.config.targetRPM);
        tft.drawString(buffer, 170, 48, 2);
    }
}

void drawFullUI() {
    tft.fillScreen(TFT_BLACK);
    
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
