#include "power_management.h"
#include "motor_control.h"
#include "display.h"

void setupPowerManagement() {
    // Configure BOOT button (GPIO 0) as input with pull-up
    pinMode(BOOT_BUTTON, INPUT_PULLUP);
    
    // Check if we just woke up from deep sleep
    checkWakeupReason();
}

void checkWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by BOOT button");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            Serial.println("Power on or reset");
            break;
        default:
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
}

void enterDeepSleep() {
    Serial.println("Preparing for deep sleep...");
    
    // Stop motor if running
    stopMotor();
    delay(100);  // Give motor time to stop
    
    // Show sleep message on display
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Going to sleep...", 160, 100, 4);
    tft.drawString("Press BOOT button", 160, 140, 2);
    tft.drawString("to wake up", 160, 165, 2);
    delay(2000);
    
    // Turn off display backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
    
    // Configure wake-up source: BOOT button (GPIO 0, active LOW)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BOOT_BUTTON, LOW);
    
    Serial.println("Entering deep sleep NOW");
    Serial.flush();
    delay(100);
    
    // Enter deep sleep
    esp_deep_sleep_start();
}
