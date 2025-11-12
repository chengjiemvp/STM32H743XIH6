#pragma once
#include "main.hpp"
#include <cstdint>


class Led {
    public:
        Led();
        void flash_irregular(); // will not in use
        void breathing();       // breathing light effect (software PWM)

    private:
        void on();
        void off();
        void toggle();
        void set_brightness(uint8_t brightness);  // 0-100

        bool is_on_;
        uint32_t counter_;
        uint32_t current_duration_;
        GPIO_TypeDef* port_;
        uint16_t pin_;
        
        // Breathing light variables
        uint8_t brightness_;        // current brightness (0-100)
        int8_t brightness_step_;    // increment/decrement step (-1 or 1)
        uint32_t pwm_counter_;      // for software PWM
        uint32_t pwm_period_;       // PWM period in milliseconds
};