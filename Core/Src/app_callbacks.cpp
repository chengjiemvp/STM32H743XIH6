#include <memory>
#include "main.hpp"
#include "uart.hpp"
#include "led.hpp"
#include "ST7789.hpp"

// 声明在 main.cpp 中定义的全局 led_pc13_ptr 指针
// 我们需要用 extern 来告诉编译器，这个变量在别处定义
extern Led* led_pc13_ptr;

///  @brief uart receive callback
void uart_rx_callback(uint8_t byte) {
    printf("%c", byte);
} // user defined uart receive callback

extern "C" { // system callback functions
    ///  @brief  uart receive callback
    ///  @param  huart: uart handle pointer, UART_HandleTypeDef structure
    ///  @retval None
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (huart->Instance == USART1) {
            // Safety check: only call if Uart is initialized
            // This prevents crashes if interrupt fires before init is complete
            if (Uart::is_initialized()) {
                Uart::get_instance().isr_handler();
            }
        }
    }

    ///  @brief  TIM Period Elapsed callback in non-blocking mode
    ///  @param  htim TIM handle
    ///  @retval None
    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

        if (htim->Instance == TIM6) {
            // Reserved for future periodic tasks
        }

        if (htim->Instance == TIM7) {
            if (led_pc13_ptr) {
                led_pc13_ptr->breathing();
            }
        }
    }

}
