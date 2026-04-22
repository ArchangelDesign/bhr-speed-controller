#pragma once

#include <Arduino.h>
#include "global_state.h"

// RPM measurement
void setupRPMMeasurement();
float getCurrentRPM();

// Motor control
void setupMotorControl();
void setMotorPower(float powerPercent);  // 0-100%
void stopMotor();

// Main motor update function
void updateMotorControl();
