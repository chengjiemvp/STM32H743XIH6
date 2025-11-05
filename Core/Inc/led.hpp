#pragma once
#include "main.hpp"
#include <cstdint>


class Led {
    public:
        Led(GPIO_TypeDef* led_port = GPIOC, uint16_t pin = GPIO_PIN_13);
        void on();
        void off();
        void toggle();
        void flash_irregular();
    private:
        GPIO_TypeDef* port_;
        uint16_t pin_;
};