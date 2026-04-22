#include "pid_controller.h"

PIDController g_pidController;

PIDController::PIDController() {
    kp = 0.5f;
    ki = 0.1f;
    kd = 0.01f;
    integral = 0.0f;
    previousError = 0.0f;
    lastTime = 0;
    outputMin = 0.0f;
    outputMax = 100.0f;
}

void PIDController::setTunings(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

void PIDController::setOutputLimits(float min, float max) {
    outputMin = min;
    outputMax = max;
}

void PIDController::reset() {
    integral = 0.0f;
    previousError = 0.0f;
    lastTime = millis();
}

float PIDController::compute(float setpoint, float input) {
    uint32_t now = millis();
    float dt = (now - lastTime) / 1000.0f; // Convert to seconds
    
    if (dt <= 0.0f) {
        return outputMin;
    }
    
    lastTime = now;
    
    // Calculate error
    float error = setpoint - input;
    
    // Proportional term
    float pTerm = kp * error;
    
    // Integral term
    integral += error * dt;
    float iTerm = ki * integral;
    
    // Derivative term
    float derivative = (error - previousError) / dt;
    float dTerm = kd * derivative;
    
    // Calculate output
    float output = pTerm + iTerm + dTerm;
    
    // Clamp output
    if (output > outputMax) {
        output = outputMax;
        // Anti-windup: stop integral accumulation when saturated
        integral -= error * dt;
    } else if (output < outputMin) {
        output = outputMin;
        integral -= error * dt;
    }
    
    previousError = error;
    
    return output;
}
