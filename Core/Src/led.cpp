#include <stdlib.h>
#include "led.hpp"

Led::Led() : is_on_(false), counter_(0), current_duration_(0) {
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

void Led::flash_irregular() {
    this->on();
    HAL_Delay(500);
    this->off();
    HAL_Delay(500);
    this->on();
    HAL_Delay(100);
    this->off();
    HAL_Delay(100);
    this->on();
    HAL_Delay(100);
    this->off();
    HAL_Delay(100);
    this->on();
}

// 状态机的核心：每毫秒被调用一次
void Led::update() {
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
            current_duration_ = (rand() % 500) + 400;
        }
        
        // 重置计数器
        counter_ = current_duration_;
    }
}