/// @file main.cpp
#include "main.hpp"
#include <memory>
#include <stdio.h>
#include "gpio.h"
#include "dma.h"  // ⭐ 添加DMA头文件
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


// use static storage for Led instead of unique_ptr to avoid SDRAM allocation
static unsigned char led_storage[sizeof(Led)];
Led* led_pc13_ptr = nullptr;

// ⭐ LCD全局指针，用于DMA回调
static unsigned char lcd_storage[sizeof(ST7789)];
ST7789* g_lcd_ptr = nullptr;

/// @brief  application entry point
/// @retval int type 0 reprentes success
int main(void) {
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    SystemClock_Config();

    // initial all hardware peripherals
    MX_GPIO_Init();
    MX_DMA_Init();  // ⭐ 关键：必须在SPI初始化之前调用，启用DMA时钟
    MX_FMC_Init();
    // wakeup sdram after fmc init
    bsp::sdram::init_sequence(&hsdram1);
    MX_USART1_UART_Init();
    MX_TIM6_Init();
    MX_TIM7_Init();
    MX_SPI5_Init();

    // small delay to ensure UART is ready
    HAL_Delay(100);

    // Initialize LED object using placement new on static storage
    led_pc13_ptr = new (&led_storage) Led();
    // Initialize UART singleton (but don't start interrupts yet)
    Uart::init(&huart1);
    printf("[%s] %s", "LOG", "STM32H743XIH6 started\r\n");
    // ⭐ Initialize LCD using placement new for DMA callback access
    g_lcd_ptr = new (&lcd_storage) ST7789(&hspi5, GPIOJ, GPIO_PIN_11, GPIOH, GPIO_PIN_6);
    g_lcd_ptr->init_basic();

    // register UART receive callback
    Uart::get_instance().set_rx_callback(uart_rx_callback); 
    // start all interrupts
    Uart::get_instance().begin();
    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);
    printf("[%s] %s", "LOG", "system ready\r\n");

    // start lcd cycle loop
    g_lcd_ptr->color_cycle_loop();
}
