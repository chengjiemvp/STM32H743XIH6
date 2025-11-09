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


std::unique_ptr<Led> led_pc13_ptr; // only defined now, initialized in main()

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

    led_pc13_ptr = std::make_unique<Led>(); // now initialize led after gpio init
    Uart::init(&huart1);
    Uart::get_instance().begin(); // start uart receive interrupt
    printf("UART initialized\n");
    HAL_TIM_Base_Start_IT(&htim6); // start TIM6 interrupt, serial status
    HAL_TIM_Base_Start_IT(&htim7); // start TIM7 interrupt, led upate


    ST7789 lcd(&hspi5, GPIOH, GPIO_PIN_5, GPIOJ, GPIO_PIN_11, GPIOH, GPIO_PIN_6 /* 没有 RST 就不传 */);
    lcd.init_basic();
    lcd.fill_screen(0x0000);  // 先清黑
    printf("ST7789 init + clear done.\r\n");

    uint32_t last_change = HAL_GetTick();
    int color_index = 0;
    const uint16_t colors[] = { 0xF800, 0x07E0, 0x001F }; // R,G,B
    const uint32_t hold_ms = 800; // 每种颜色保持时间

    while (1) {

        if (Uart::get_instance().available() > 0) {
            int byte = Uart::get_instance().read();
            if (byte != -1) {
                printf("Received: %c (0x%02X)\r\n", (char)byte, byte);
            }
        } // deal with received data via uart

        uint32_t now = HAL_GetTick();
        if (now - last_change >= hold_ms) {
            lcd.fill_screen(colors[color_index]);
            printf("Screen color index %d\r\n", color_index);
            color_index = (color_index + 1) % 3;
            last_change = now;
        }

    }
}