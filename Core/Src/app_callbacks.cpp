#include <memory>
#include "main.hpp"
#include "Uart.hpp"
#include "led.hpp"

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
            uint32_t current_time = HAL_GetTick();
            printf("[STATUS] System is running. Uptime: %lu ms\r\n", (unsigned long)current_time);
        } // TIM6 for serial status

        if (htim->Instance == TIM7) {
            if (led_pc13_ptr) {
                led_pc13_ptr->update();
            }
        } // TIM7 for led update
    }
}