#include <memory>
#include "main.hpp"
#include "Uart.hpp"
#include "led.hpp"
#include "ST7789.hpp"

// 声明在 main.cpp 中定义的全局 led_pc13_ptr 指针
// 我们需要用 extern 来告诉编译器，这个变量在别处定义
extern std::unique_ptr<Led> led_pc13_ptr;

extern "C" {
    /// @brief  uart receive callback
    /// @param  huart: uart handle pointer, UART_HandleTypeDef structure
    /// @retval None
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (huart->Instance == USART1) {
            Uart::get_instance().isr_handler();
        }
    }

    /// @brief  TIM Period Elapsed callback in non-blocking mode
    /// @param  htim TIM handle
    /// @retval None
    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

        if (htim->Instance == TIM6) {
            // 在这里执行您的周期性任务
            if (Uart::get_instance().tick_count_ == 20) {
                uint32_t current_time = HAL_GetTick();
                printf("[STATUS] System is running. Uptime: %lu ms\r\n", (unsigned long)current_time);
                Uart::get_instance().tick_count_ = 0;
            }
            else {
                Uart::get_instance().tick_count_++;
            }
        } // TIM6 for serial status

        if (htim->Instance == TIM7) {
            if (led_pc13_ptr) {
                led_pc13_ptr->flash_irregular();
            }
        } // TIM7 for led update
    }
}