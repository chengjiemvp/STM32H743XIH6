/// @file main.cpp
#include "main.hpp"
#include <memory>
#include <stdio.h>
#include "gpio.h"
#include "fmc.h"
#include "usart.h"
#include "tim.h"
#include "spi.h"

#include "bsp_sdram.hpp"
#include "test.hpp"
#include "led.hpp"
#include "uart.hpp"
#include "system_setup.hpp"
#include "ST7789.hpp"


// Use static storage for Led instead of unique_ptr to avoid SDRAM allocation
static unsigned char led_storage[sizeof(Led)];
Led* led_pc13_ptr = nullptr;

/// @brief  application entry point
/// @retval int type 0 reprentes success
int main(void) {
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_FMC_Init();
    bsp::sdram::init_sequence(&hsdram1); // wakeup sdram after fmc init
    MX_USART1_UART_Init();
    MX_TIM6_Init();
    MX_TIM7_Init();
    MX_SPI5_Init();

    // Small delay to ensure UART is ready
    HAL_Delay(100);

    // Initialize LED object using placement new on static storage
    led_pc13_ptr = new (&led_storage) Led();
    
    // Initialize UART singleton (but don't start interrupts yet)
    Uart::init(&huart1);
    
    printf("\r\n=== STM32H743XIH6 Started ===\r\n");
    
    // Initialize LCD and run test BEFORE starting any interrupts
    // This prevents conflicts between SPI (LCD) and UART (printf in interrupts)
    ST7789 lcd(&hspi5, GPIOJ, GPIO_PIN_11, GPIOH, GPIO_PIN_6);
    lcd.init_basic();
    
    // CRITICAL: Start ALL interrupts AFTER LCD initialization is complete
    // This prevents printf() in timer interrupts from interfering with SPI transfers
    
    Uart::get_instance().set_rx_callback(uart_rx_callback); // register UART receive callback
    // Start all interrupts
    Uart::get_instance().begin();
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);
    
    printf("System ready\r\n");

    // 启动颜色循环动画（无限循环，整个屏幕颜色缓慢变化）
    // 注意：这个函数永不返回，UART和LED会在后台中断中继续工作
    lcd.color_cycle_loop();
}
