#include <Arduino.h>

#include "global_state.h"
#include "pid_controller.h"
#include "motor_control.h"

// Conditional includes based on build variant
#ifdef ARDUINO_NANO_AVR
    #include "buttons.h"
#else
    #include "display.h"
    #include "touch.h"
    #include "power_management.h"
#endif
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
    
#ifdef ARDUINO_NANO_AVR
    Serial.println("BHR Speed Controller - Arduino Nano AVR");
    Serial.println("Button Interface Mode");
#else
    Serial.println("BHR Speed Controller - ESP32");
    Serial.println("Touch Display Mode");
#endif
    
    // Initialize global state
    g_state.motorState = MOTOR_STOPPED;
    g_state.currentPower = 0.0f;
    g_state.currentRPM = 0.0f;
    g_state.motorStartTime = 0;
    g_state.motorStopTime = 0;
    g_state.lastUpdateTime = millis();
    
#ifndef ARDUINO_NANO_AVR
    g_state.needsRedraw = true;
    g_state.lastTouchTime = 0;
#endif
    
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
    
#ifdef ARDUINO_NANO_AVR
    // Arduino Nano: Setup button interface
    Serial.println("Setting up buttons...");
    setupButtons();
#else
    // ESP32: Setup display, touch, and power management
    setupPowerManagement();
    
    Serial.println("Setting up display...");
    setupDisplay();
    
    Serial.println("Setting up touch...");
    setupTouch();
#endif
    
    // Setup hardware (common to both variants)
    Serial.println("Setting up motor control...");
    setupMotorControl();
    
    Serial.println("Setting up RPM measurement...");
    setupRPMMeasurement();
    
#ifndef ARDUINO_NANO_AVR
    // Draw initial UI (ESP32 only)
    drawFullUI();
#endif
    
    Serial.println("Setup complete!");
    Serial.println("===================================");
    
#ifdef ARDUINO_NANO_AVR
    Serial.println("Button controls:");
    Serial.println("- Pin 3: START/STOP motor");
    Serial.println("- Pin 4: SPEED UP (hold to repeat)");
    Serial.println("- Pin 5: SPEED DOWN (hold to repeat)");
#else
    Serial.println("Touch controls:");
    Serial.println("- START/STOP: Start or stop motor");
    Serial.println("- Mode: Toggle between Fixed Power and PID");
    Serial.println("- Power +/-: Adjust target power");
    Serial.println("- RPM +/-: Adjust target RPM (PID mode)");
    Serial.println("- SLEEP: Enter deep sleep (wake with BOOT button)");
#endif
    Serial.println("===================================");
}

void loop() {
    uint32_t now = millis();
    
#ifdef ARDUINO_NANO_AVR
    // Arduino Nano: Handle button input
    handleButtons();
    
    // Update motor control
    updateMotorControl();
    g_state.lastUpdateTime = now;
    
#else
    // ESP32: Handle touch input and display updates
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
#endif
    
    // Small delay to prevent overwhelming the CPU
    delay(1);
}


