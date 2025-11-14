#include <memory>
#include "main.hpp"
#include "uart.hpp"
#include "led.hpp"
#include "ST7789.hpp"

// 声明在 main.cpp 中定义的全局 led_pc13_ptr 指针
// 我们需要用 extern 来告诉编译器，这个变量在别处定义
extern Led* led_pc13_ptr;

///  @brief User-defined UART receive callback (Layer 8 in call chain)
///  @param byte The received byte from UART
///  @note This function is called from Uart::isr_handler() in interrupt context
///  @note Keep this function short and fast - it runs in ISR context
void uart_rx_callback(uint8_t byte) {
    printf("%c", byte);  // Print received character to console
}

extern "C" { // system callback functions - C/C++ bridge layer
    ///  @brief HAL UART receive complete callback (Layer 4 in call chain)
    ///  @param huart: uart handle pointer, UART_HandleTypeDef structure
    ///  @retval None
    ///  @note This function is called by HAL_UART_IRQHandler() when a byte is received
    ///  @note The 'extern "C"' allows this C++ function to be called from C code (HAL library)
    ///  @note This overrides the __weak version in stm32h7xx_hal_uart.c
    ///
    ///  Call chain: USART1_IRQHandler → HAL_UART_IRQHandler → HAL_UART_RxCpltCallback (HERE)
    ///              → Uart::isr_handler → user callback
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (huart->Instance == USART1) {
            // Safety check: only call if Uart singleton is initialized
            // This prevents crashes if interrupt fires before init is complete
            // See initialization order in main.cpp: init() before begin()
            if (Uart::is_initialized()) {
                Uart::get_instance().isr_handler();  // Delegate to C++ class method
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
