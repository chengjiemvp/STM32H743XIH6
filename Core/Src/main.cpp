/// @file main.cpp
#include "main.hpp"
#include <memory>
#include <stdio.h>
#include "gpio.h"
#include "fmc.h"
#include "usart.h"

#include "bsp_sdram.hpp"
#include "test.hpp"
#include "led.hpp"
#include "Uart.hpp"


extern "C" {
    void SystemClock_Config(void);
    void Error_Handler();
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
}
static void MPU_Config(void);

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
    bsp::sdram::init_sequence(&hsdram1); // wakeup sdram
    MX_USART1_UART_Init();

    // create led obj
    std::unique_ptr<Led> led_pc13_ptr = std::make_unique<Led>(); 
    Uart::init(&huart1);
    Uart::get_instance().begin();
    printf("UART initialized\n");

    // 1. 在循环外定义一个变量来记录上次发送的时间
    uint32_t last_status_update_time = 0;
    // 定义发送间隔，例如 2000 毫秒 (2秒)
    const uint32_t status_update_interval = 2000;

    while (1) {
        // 2. 获取当前时间
        uint32_t current_time = HAL_GetTick();
        // 3. 检查是否到达发送时间
        if (current_time - last_status_update_time >= status_update_interval) {
            // 更新时间戳为当前时间
            last_status_update_time = current_time;
            // 发送系统状态信息
            printf("[STATUS] System is running. Uptime: %lu ms\r\n", current_time);
        }

        // deal with received data via uart
        if (Uart::get_instance().available() > 0) {
            int byte = Uart::get_instance().read();
            if (byte != -1) {
                printf("Received: %c (0x%02X)\r\n", (char)byte, byte);
            }
        }
        
        led_pc13_ptr->flash_irregular();
    }
}

/// @brief  uart receive callback
/// @param  huart: uart handle pointer, UART_HandleTypeDef structure
/// @retval None
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        Uart::get_instance().isr_handler();
    }
}

/// @brief System Clock Configuration
/// @retval None
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    // Supply configuration update enable
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    // Configure the main internal regulator output voltage
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // Initializes the RCC Oscillators according to the specified parameters
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2|RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

/// @brief static function, also used in this source file
void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    // Disables the MPU
    HAL_MPU_Disable();

    // Initializes and configures the Region and the memory to be protected
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0xC0000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

    // intializes for region 1, internal sdram-DTCMRAM
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = 0x20000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    // Enables the MPU
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/// @brief  This function is executed in case of error occurrence.
/// @retval None
void Error_Handler(void) {
    // User can add his own implementation to report the HAL error return state
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
/// @brief  Reports the name of the source file and the source line number
///         where the assert_param error has occurred.
/// @param  file: pointer to the source file name
/// @param  line: assert_param error line source number
/// @retval None
void assert_failed(uint8_t *file, uint32_t line) {
    // User can add his own implementation to report the file name and line number,
    // ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)
}
#endif // USE_FULL_ASSERT