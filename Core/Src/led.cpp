#include "led.hpp"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"

Led::Led(GPIO_TypeDef* led_port, uint16_t pin) : port_(led_port), pin_(pin) {}

void Led::on() {
    // 调用 HAL 库函数来设置引脚为高电平
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
}

void Led::off() {
    // 调用 HAL 库函数来设置引脚为低电平
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
}

void Led::toggle() {
    // 调用 HAL 库函数来翻转引脚电平
    HAL_GPIO_TogglePin(port_, pin_);
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