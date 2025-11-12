#include <stdlib.h>
#include <stdio.h>
#include "led.hpp"

Led::Led() : is_on_(false), counter_(0), current_duration_(0), 
             brightness_(50), brightness_step_(1), pwm_counter_(0), pwm_period_(5) {
    off();
}

void Led::on() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    is_on_ = true;
}

void Led::off() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    is_on_ = false;
}

void Led::toggle() {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    is_on_ = !is_on_;
}

void Led::set_brightness(uint8_t brightness) {
    // Clamp brightness to 0-100 range
    if (brightness > 100) {
        brightness = 100;
    }
    brightness_ = brightness;
}

// Breathing light effect using software PWM
// Called once per millisecond from HAL_TIM_PeriodElapsedCallback
void Led::breathing() {
    // Software PWM implementation
    // pwm_period_ = 10ms, update brightness every 10ms
    pwm_counter_++;
    
    if (pwm_counter_ >= pwm_period_) {
        pwm_counter_ = 0;
        
        // Update brightness (0-100)
        brightness_ += brightness_step_;
        
        // Reverse direction at min/max brightness
        if (brightness_ >= 100) {
            brightness_ = 100;
            brightness_step_ = -1;  // start decreasing
        } else if (brightness_ <= 0) {
            brightness_ = 0;
            brightness_step_ = 1;   // start increasing
        }
    }
    
    // Simulate PWM with ON/OFF ratio based on brightness
    // For example: brightness 50 means LED is on 50% of the time
    if (pwm_counter_ < (pwm_period_ * brightness_ / 100)) {
        on();
    } else {
        off();
    }
}

// 状态机的核心：每毫秒被调用一次
void Led::flash_irregular() {
    // 计时器减一
    if (counter_ > 0) {
        counter_--;
    }

    // 如果当前状态的持续时间已到
    if (counter_ == 0) {
        // 翻转 LED 状态
        toggle();

        // 为下一个状态设置一个新的、随机的持续时间
        if (is_on_) {
            // 如果现在是亮灯状态，设置一个较短的亮灯时间 (50-150ms)
            current_duration_ = (rand() % 100) + 50;
        } else {
            // 如果现在是灭灯状态，设置一个较长的灭灯时间 (400-900ms)
            current_duration_ = (rand() % 500) + 700;
        }
        
        // 重置计数器
        counter_ = current_duration_;
    }
}
