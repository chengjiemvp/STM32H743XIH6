#pragma once
#include "main.hpp"
#include <cstdint>


class Led {
    public:
        Led();
        void flash_irregular(); // will not in use
        void update();
    private:
        void on();
        void off();
        void toggle();

        bool is_on_;
        uint32_t counter_;
        uint32_t current_duration_; // ??
        GPIO_TypeDef* port_;
        uint16_t pin_;
};