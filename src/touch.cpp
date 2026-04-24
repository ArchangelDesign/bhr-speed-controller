#include "touch.h"
#include "display.h"
#include "motor_control.h"
#include "pid_controller.h"
#include "power_management.h"
#include "process_timer.h"
#include <SPI.h>

// Touch pins are defined in platformio.ini
#ifndef TOUCH_CS
#define TOUCH_CS 33
#endif

#ifndef TOUCH_IRQ
#define TOUCH_IRQ 36
#endif

// Create touchscreen instance with IRQ pin
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Calibration values for CYD (typical values)
#define TS_MIN_X 200
#define TS_MAX_X 3700
#define TS_MIN_Y 240
#define TS_MAX_Y 3800

// Custom SPI for touch
SPIClass touchSPI = SPIClass(VSPI);

void setupTouch() {
    // CYD uses separate SPI pins for touch controller
    // Touch uses: CLK=25, MOSI=32, MISO=39, CS=33
    Serial.println("Initializing touch controller...");
    #if defined(TOUCH_CLK) && defined(TOUCH_DIN) && defined(TOUCH_DO)
        Serial.printf("Using custom SPI: CLK=%d, MOSI=%d, MISO=%d, CS=%d\n", 
                      TOUCH_CLK, TOUCH_DIN, TOUCH_DO, TOUCH_CS);
        touchSPI.begin(TOUCH_CLK, TOUCH_DO, TOUCH_DIN, TOUCH_CS);
        ts.begin(touchSPI);
    #else
        Serial.println("Using default SPI");
        ts.begin();
    #endif
    ts.setRotation(1);  // Match display rotation
    Serial.println("Touch controller initialized");
}

int getTouchedButton(int16_t x, int16_t y) {
    for (int i = 0; i < BTN_COUNT; i++) {
        const Button& btn = g_buttons[i];
        if (x >= btn.x && x <= btn.x + btn.w &&
            y >= btn.y && y <= btn.y + btn.h) {
            return i;
        }
    }
    return -1;
}

void handleTouch() {
    if (!ts.tirqTouched() || !ts.touched()) {
        return;
    }
    
    TS_Point p = ts.getPoint();
    
    // Check for valid touch (z should be between reasonable pressure values)
    // Invalid readings typically have z at max value (4095) or x/y at max (8191)
    if (p.z < 200 || p.z > 4000 || p.x > 8000 || p.y > 8000) {
        Serial.println("Invalid touch reading - ignoring");
        return;
    }
    
    // Map touch coordinates to display coordinates
    int16_t x = map(p.x, TS_MIN_X, TS_MAX_X, 0, 320);
    int16_t y = map(p.y, TS_MIN_Y, TS_MAX_Y, 0, 240);
    
    // Clamp to screen bounds
    x = constrain(x, 0, 319);
    y = constrain(y, 0, 239);
    
    
    // Debounce
    uint32_t now = millis();
    if (now - g_state.lastTouchTime < 200) {
        return;
    }
    g_state.lastTouchTime = now;
    
    // Check which button was pressed
    int buttonId = getTouchedButton(x, y);
    
    if (buttonId < 0) {
        Serial.printf("Touch missed all buttons: x=%d, y=%d\n", x, y);
        return;
    }

    Serial.printf("Button pressed: %d (%s)\n", buttonId, g_buttons[buttonId].label);
    
    // Visual feedback
    drawButton(g_buttons[buttonId], true);
    delay(100);
    
    // Handle button press
    switch (buttonId) {
        case BTN_START_STOP:
            if (g_state.motorState == MOTOR_STOPPED) {
                // Start motor
                g_state.motorState = MOTOR_STARTING;
                g_state.motorStartTime = millis();
                g_pidController.reset();
                startProcessTimer();
            } else {
                // Stop motor
                stopMotor();
            }
            g_state.needsRedraw = true;
            break;
            
        case BTN_MODE:
            // Toggle mode
            if (g_state.config.mode == MODE_FIXED_POWER) {
                g_state.config.mode = MODE_FIXED_SPEED;
            } else {
                g_state.config.mode = MODE_FIXED_POWER;
            }
            g_pidController.reset();
            g_state.needsRedraw = true;
            break;
            
        case BTN_POWER_UP:
            g_state.config.targetPower += CONTROL_RESOLUTION;
            if (g_state.config.targetPower > 100.0f) {
                g_state.config.targetPower = 100.0f;
            }
            break;
            
        case BTN_POWER_DOWN:
            g_state.config.targetPower -= CONTROL_RESOLUTION;
            if (g_state.config.targetPower < 0.0f) {
                g_state.config.targetPower = 0.0f;
            }
            break;
            
        case BTN_RPM_UP:
            g_state.config.targetRPM += 50.0f;
            if (g_state.config.targetRPM > 5000.0f) {
                g_state.config.targetRPM = 5000.0f;
            }
            break;
            
        case BTN_RPM_DOWN:
            g_state.config.targetRPM -= 50.0f;
            if (g_state.config.targetRPM < 0.0f) {
                g_state.config.targetRPM = 0.0f;
            }
            break;
            
        case BTN_SLEEP:
            // Enter deep sleep mode
            enterDeepSleep();
            // This code won't be reached - device will sleep
            break;
    }
    
    drawButton(g_buttons[buttonId], false);
        
}
