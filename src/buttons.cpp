#include "buttons.h"

#ifdef ARDUINO_NANO_AVR

#include "motor_control.h"
#include "pid_controller.h"

// Button state tracking
static ButtonState btnStartStop = {false, false, 0, 0, HIGH};
static ButtonState btnSpeedUp = {false, false, 0, 0, HIGH};
static ButtonState btnSpeedDown = {false, false, 0, 0, HIGH};

// Speed adjustment increments
const float POWER_STEP = 1.0f;      // 1% per button press
const float RPM_STEP = 100.0f;      // 100 RPM per button press

void setupButtons() {
    // Configure button pins as inputs with internal pull-ups
    pinMode(BUTTON_START_STOP, INPUT_PULLUP);
    pinMode(BUTTON_SPEED_UP, INPUT_PULLUP);
    pinMode(BUTTON_SPEED_DOWN, INPUT_PULLUP);
    
    Serial.println("Button interface initialized");
    Serial.println("  Pin 3: Start/Stop");
    Serial.println("  Pin 4: Speed Up");
    Serial.println("  Pin 5: Speed Down");
}

// Check button state with debouncing and hold detection
static bool checkButton(ButtonState& btn, uint8_t pin, uint32_t now) {
    bool currentState = digitalRead(pin);
    bool buttonPressed = false;
    
    // Detect press (transition from HIGH to LOW)
    if (currentState == LOW && btn.lastState == HIGH) {
        if (now - btn.pressTime > BUTTON_DEBOUNCE_MS) {
            btn.pressed = true;
            btn.pressTime = now;
            btn.lastRepeatTime = now;
            buttonPressed = true;
        }
    }
    // Detect release
    else if (currentState == HIGH && btn.lastState == LOW) {
        btn.pressed = false;
        btn.held = false;
    }
    // Detect hold and repeat
    else if (btn.pressed && currentState == LOW) {
        if (!btn.held && (now - btn.pressTime > BUTTON_HOLD_MS)) {
            btn.held = true;
            btn.lastRepeatTime = now;
            buttonPressed = true;
        }
        else if (btn.held && (now - btn.lastRepeatTime > BUTTON_REPEAT_MS)) {
            btn.lastRepeatTime = now;
            buttonPressed = true;
        }
    }
    
    btn.lastState = currentState;
    return buttonPressed;
}

void handleButtons() {
    uint32_t now = millis();
    
    // Handle Start/Stop button
    if (checkButton(btnStartStop, BUTTON_START_STOP, now)) {
        if (g_state.motorState == MOTOR_STOPPED) {
            // Start motor
            g_state.motorState = MOTOR_STARTING;
            g_state.motorStartTime = now;
            g_pidController.reset();
            
            Serial.println("Motor STARTED");
            Serial.print("  Mode: ");
            Serial.println(g_state.config.mode == MODE_FIXED_POWER ? "Fixed Power" : "PID Speed Control");
            Serial.print("  Target: ");
            if (g_state.config.mode == MODE_FIXED_POWER) {
                Serial.print(g_state.config.targetPower);
                Serial.println("%");
            } else {
                Serial.print(g_state.config.targetRPM);
                Serial.println(" RPM");
            }
        } else {
            // Stop motor
            stopMotor();
            g_state.motorState = MOTOR_STOPPED;
            g_state.currentPower = 0.0f;
            
            Serial.println("Motor STOPPED");
        }
    }
    
    // Handle Speed Up button
    if (checkButton(btnSpeedUp, BUTTON_SPEED_UP, now)) {
        if (g_state.config.mode == MODE_FIXED_POWER) {
            g_state.config.targetPower = constrain(g_state.config.targetPower + POWER_STEP, 0.0f, 100.0f);
            Serial.print("Target Power: ");
            Serial.print(g_state.config.targetPower);
            Serial.println("%");
        } else {
            g_state.config.targetRPM += RPM_STEP;
            Serial.print("Target RPM: ");
            Serial.println(g_state.config.targetRPM);
        }
    }
    
    // Handle Speed Down button
    if (checkButton(btnSpeedDown, BUTTON_SPEED_DOWN, now)) {
        if (g_state.config.mode == MODE_FIXED_POWER) {
            g_state.config.targetPower = constrain(g_state.config.targetPower - POWER_STEP, 0.0f, 100.0f);
            Serial.print("Target Power: ");
            Serial.print(g_state.config.targetPower);
            Serial.println("%");
        } else {
            g_state.config.targetRPM = max(0.0f, g_state.config.targetRPM - RPM_STEP);
            Serial.print("Target RPM: ");
            Serial.println(g_state.config.targetRPM);
        }
    }
}

#endif // ARDUINO_NANO_AVR
