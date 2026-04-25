#pragma once

#include <Arduino.h>

// Pin definitions (from platformio.ini)
#ifndef PID_OUTPUT
#define PID_OUTPUT 18
#endif

#ifndef RPM_INPUT
#define RPM_INPUT 27
#endif

// PWM configuration
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8     // 8-bit resolution (0-255)

// Control modes
enum ControlMode {
    MODE_FIXED_POWER,
    MODE_FIXED_SPEED
};

// Motor states
enum MotorState {
    MOTOR_STOPPED,
    MOTOR_STARTING,  // Soft start phase
    MOTOR_RUNNING,
    MOTOR_STOPPING   // Soft stop phase
};

// Global configuration structure
struct Config {
    ControlMode mode;
    float targetPower;      // Target power percentage (0-100)
    float targetRPM;        // Target RPM for PID mode
    uint16_t timeoutMinutes; // Timeout in minutes (0 = no timeout)
    uint16_t softStartSeconds; // Soft start duration in seconds
    uint16_t softStopSeconds;  // Soft stop duration in seconds
    bool rpmSensorEnabled;   // Whether RPM sensor is connected
    
    // PID parameters
    float pidKp;
    float pidKi;
    float pidKd;
};

// Global state structure
struct GlobalState {
    MotorState motorState;
    float currentPower;     // Current power output (0-100%)
    float currentRPM;       // Current measured RPM
    uint32_t motorStartTime; // millis() when motor started
    uint32_t motorStopTime;  // millis() when motor stopping started
    uint32_t lastUpdateTime; // millis() of last update
    Config config;
    
    // Process timer state
    uint32_t processTimerSeconds;       // Accumulated running time in seconds
    uint32_t processTimerStartMillis;   // millis() when timer was last started
    bool processTimerRunning;           // Is timer currently running
    
    // UI state
    bool needsRedraw;
    uint32_t lastTouchTime;
};

// Global state instance
extern GlobalState g_state;

// Default configuration
inline void initDefaultConfig(Config& cfg) {
    cfg.mode = MODE_FIXED_POWER;
    cfg.targetPower = 50.0f;
    cfg.targetRPM = 1000.0f;
    cfg.timeoutMinutes = 0;
    cfg.softStartSeconds = 5;
    cfg.softStopSeconds = 5;
    cfg.rpmSensorEnabled = true;
    cfg.pidKp = 0.5f;
    cfg.pidKi = 0.1f;
    cfg.pidKd = 0.01f;
}

