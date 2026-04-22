#pragma once

#include <Arduino.h>
#include "global_state.h"

class PIDController {
private:
    float kp, ki, kd;
    float integral;
    float previousError;
    uint32_t lastTime;
    float outputMin, outputMax;
    
public:
    PIDController();
    
    void setTunings(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void reset();
    float compute(float setpoint, float input);
};

extern PIDController g_pidController;
