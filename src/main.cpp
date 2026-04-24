#include <Arduino.h>

#include "global_state.h"
#include "pid_controller.h"
#include "display.h"
#include "touch.h"
#include "motor_control.h"
#include "power_management.h"
#include "process_timer.h"

// Global state instance
GlobalState g_state;

// Update intervals
const uint32_t DISPLAY_UPDATE_INTERVAL = 500;  // Update display every 500ms (reduce flicker)
const uint32_t MOTOR_UPDATE_INTERVAL = 20;     // Update motor control every 20ms

uint32_t lastDisplayUpdate = 0;
uint32_t lastMotorUpdate = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("BHR Speed Controller Starting...");
    
    // Initialize global state
    g_state.motorState = MOTOR_STOPPED;
    g_state.currentPower = 0.0f;
    g_state.currentRPM = 0.0f;
    g_state.motorStartTime = 0;
    g_state.lastUpdateTime = millis();
    g_state.needsRedraw = true;
    g_state.lastTouchTime = 0;
    
    // Initialize default configuration
    initDefaultConfig(g_state.config);
    
    // Initialize process timer
    Serial.println("Initializing process timer...");
    initProcessTimer();
    
    // Setup PID controller
    g_pidController.setTunings(g_state.config.pidKp, 
                               g_state.config.pidKi, 
                               g_state.config.pidKd);
    g_pidController.setOutputLimits(0.0f, 100.0f);
    g_pidController.reset();
    
    // Setup power management (check wake-up reason)
    Serial.println("Setting up power management...");
    setupPowerManagement();
    
    // Setup hardware
    Serial.println("Setting up display...");
    setupDisplay();
    
    Serial.println("Setting up touch...");
    setupTouch();
    
    Serial.println("Setting up motor control...");
    setupMotorControl();
    
    Serial.println("Setting up RPM measurement...");
    setupRPMMeasurement();
    
    // Draw initial UI
    drawFullUI();
    
    Serial.println("Setup complete!");
    Serial.println("===================================");
    Serial.println("Touch controls:");
    Serial.println("- START/STOP: Start or stop motor");
    Serial.println("- Mode: Toggle between Fixed Power and PID");
    Serial.println("- Power +/-: Adjust target power");
    Serial.println("- RPM +/-: Adjust target RPM (PID mode)");
    Serial.println("- SLEEP: Enter deep sleep (wake with BOOT button)");
    Serial.println("===================================");
}

void loop() {
    uint32_t now = millis();
    
    // Handle touch input
    handleTouch();
    
    // Update process timer
    updateProcessTimer();
    
    // Update motor control at high frequency
    if (now - lastMotorUpdate >= MOTOR_UPDATE_INTERVAL) {
        updateMotorControl();
        lastMotorUpdate = now;
        g_state.lastUpdateTime = now;
    }
    
    // Update display at lower frequency
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = now;
    }
    
    // Small delay to prevent overwhelming the CPU
    delay(1);
}


