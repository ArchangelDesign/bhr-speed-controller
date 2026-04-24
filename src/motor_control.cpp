#include "motor_control.h"
#include "pid_controller.h"
#include "process_timer.h"

// RPM measurement variables
volatile uint32_t rpmPulseCount = 0;
volatile uint32_t lastRpmPulseTime = 0;
uint32_t lastRpmCalculationTime = 0;
float measuredRPM = 0.0f;

// RPM sensor interrupt handler
void IRAM_ATTR rpmPulseISR() {
    rpmPulseCount++;
    lastRpmPulseTime = micros();
}

void setupRPMMeasurement() {
    if (g_state.config.rpmSensorEnabled) {
        pinMode(RPM_INPUT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(RPM_INPUT), rpmPulseISR, FALLING);
    }
    lastRpmCalculationTime = millis();
}

float getCurrentRPM() {
    if (!g_state.config.rpmSensorEnabled) {
        return 0.0f;
    }
    
    uint32_t now = millis();
    uint32_t elapsed = now - lastRpmCalculationTime;
    
    // Update RPM calculation every 100ms
    if (elapsed >= 100) {
        noInterrupts();
        uint32_t pulses = rpmPulseCount;
        rpmPulseCount = 0;
        interrupts();
        
        // Calculate RPM: (pulses / elapsed_seconds) * 60 / pulses_per_revolution
        // Assuming 1 pulse per revolution
        float elapsedSeconds = elapsed / 1000.0f;
        if (elapsedSeconds > 0) {
            measuredRPM = (pulses / elapsedSeconds) * 60.0f;
        }
        
        lastRpmCalculationTime = now;
    }
    
    // If no pulses for 2 seconds, assume stopped
    if (micros() - lastRpmPulseTime > 2000000) {
        measuredRPM = 0.0f;
    }
    
    return measuredRPM;
}

void setupMotorControl() {
    // Setup PWM on PID_OUTPUT pin
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(PID_OUTPUT, PWM_CHANNEL);
#if INVERSE_OUTPUT == 1
    ledcWrite(PWM_CHANNEL, 255);  // Start with motor off (inverted: HIGH = OFF)
#else
    ledcWrite(PWM_CHANNEL, 0);    // Start with motor off (normal: LOW = OFF)
#endif
}

void setMotorPower(float powerPercent) {
    // Clamp power to 0-100%
    if (powerPercent < 0.0f) powerPercent = 0.0f;
    if (powerPercent > 100.0f) powerPercent = 100.0f;
    
    // Convert percentage to PWM duty cycle (0-255 for 8-bit)
    uint8_t pwmValue = (uint8_t)((powerPercent / 100.0f) * 255.0f);
    
#if INVERSE_OUTPUT == 1
    // Inverted logic: LOW = motor ON, HIGH = motor OFF
    uint8_t dutyCycle = 255 - pwmValue;
#else
    // Normal logic: HIGH = motor ON, LOW = motor OFF
    uint8_t dutyCycle = pwmValue;
#endif
    
    ledcWrite(PWM_CHANNEL, dutyCycle);
    
    g_state.currentPower = powerPercent;
}

void stopMotor() {
    setMotorPower(0.0f);
    g_state.motorState = MOTOR_STOPPED;
    pauseProcessTimer();
}

void updateMotorControl() {
    // Update current RPM
    g_state.currentRPM = getCurrentRPM();
    
    if (g_state.motorState == MOTOR_STOPPED) {
        setMotorPower(0.0f);
        return;
    }
    
    uint32_t now = millis();
    uint32_t runningTime = now - g_state.motorStartTime;
    
    // Check timeout
    if (g_state.config.timeoutMinutes > 0) {
        uint32_t timeoutMs = g_state.config.timeoutMinutes * 60000UL;
        if (runningTime >= timeoutMs) {
            stopMotor();
            return;
        }
    }
    
    float targetPower = 0.0f;
    
    // Handle soft start
    if (g_state.motorState == MOTOR_STARTING) {
        if (g_state.config.softStartSeconds > 0) {
            float softStartProgress = runningTime / (g_state.config.softStartSeconds * 1000.0f);
            if (softStartProgress >= 1.0f) {
                g_state.motorState = MOTOR_RUNNING;
                softStartProgress = 1.0f;
            }
            
            // Calculate target power based on mode
            if (g_state.config.mode == MODE_FIXED_POWER) {
                targetPower = g_state.config.targetPower * softStartProgress;
            } else {
                // In PID mode, ramp up the setpoint
                float currentSetpoint = g_state.config.targetRPM * softStartProgress;
                targetPower = g_pidController.compute(currentSetpoint, g_state.currentRPM);
            }
        } else {
            // No soft start, go directly to running
            g_state.motorState = MOTOR_RUNNING;
        }
    }
    
    // Handle normal running
    if (g_state.motorState == MOTOR_RUNNING) {
        if (g_state.config.mode == MODE_FIXED_POWER) {
            targetPower = g_state.config.targetPower;
        } else {
            // PID mode
            if (g_state.config.rpmSensorEnabled) {
                targetPower = g_pidController.compute(g_state.config.targetRPM, g_state.currentRPM);
            } else {
                // No RPM sensor, fall back to fixed power
                targetPower = g_state.config.targetPower;
            }
        }
    }
    
    setMotorPower(targetPower);
}
