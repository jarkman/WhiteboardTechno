/**
 * @file wakeup.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief TimerCAM RTC Wakeup Test
 * @version 0.1
 * @date 2024-01-02
 *
 *
 * @Hardwares: TimerCAM
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * TimerCam-arduino: https://github.com/m5stack/TimerCam-arduino
 */

#include "M5TimerCAM.h"

void led_breathe(int ms) {
    for (int16_t i = 0; i < 255; i++) {
        TimerCAM.Power.setLed(i);
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

    for (int16_t i = 255; i >= 0; i--) {
        TimerCAM.Power.setLed(i);
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

void setup() {
    TimerCAM.begin(true);
    Serial.println("Wake up!!!");
    led_breathe(10);
    // sleep after 5s wakeup!
    TimerCAM.Power.timerSleep(5);
}

void loop() {
}
