#include "uart.hpp"
#include <stdio.h>

/// @brief initialize static variables
Uart* Uart::instance_ = nullptr;
bool Uart::initialized_ = false;
unsigned char Uart::instance_storage_[256];

Uart::Uart(UART_HandleTypeDef* huart) : tick_count_(0), huart_(huart), 
    rx_buffer_(), rx_data_(0), rx_callback_(nullptr)  {}

/// @brief Start UART receive interrupt (enables continuous reception)
/// @note This must be called after Uart::init() to start receiving data
/// @note Call chain starts here: begin() → HAL_UART_Receive_IT() → hardware waits for data
///       → USART1_IRQHandler → ... → isr_handler()
void Uart::begin() {
    HAL_UART_Receive_IT(huart_, &rx_data_, 1);  // Start interrupt-driven reception of 1 byte
}

/// @brief Check how many bytes are available to read from ring buffer
/// @return Number of bytes in the buffer
/// @note This is safe to call from main loop (non-ISR context)
/// @note Alternative to callback: poll with available() and read with read()
size_t Uart::available() {
    return rx_buffer_.count();
}

/// @brief Read a byte from ring buffer, return -1 if no data
/// @return Received byte (0-255) or -1 if buffer is empty
/// @note This is safe to call from main loop (non-ISR context)
/// @note Complements the callback mechanism for deferred processing
int Uart::read() {
    uint8_t byte;
    if (rx_buffer_.pop(byte)) {
        return byte;
    }
    return -1; // no data
}

/// @brief UART interrupt service routine handler (Layer 5 in call chain)
/// @note This is called from HAL_UART_RxCpltCallback() when a byte is received
/// @note Runs in interrupt context - keep fast!
/// @note Three critical actions:
///       1. Store received byte in ring buffer for later processing
///       2. Call user callback for immediate processing (optional)
///       3. Re-enable interrupt reception for next byte
///
/// Call chain: ... → HAL_UART_RxCpltCallback → isr_handler (HERE)
///             → rx_buffer.push + rx_callback() + HAL_UART_Receive_IT
void Uart::isr_handler() {
    // Action 1: Store in ring buffer (can be read later from main loop)
    rx_buffer_.push(rx_data_);  // rx_data_ was filled by HAL_UART_Receive_IT
    
    // Action 2: Call user callback if registered (immediate processing)
    if (rx_callback_) {
        rx_callback_(rx_data_);  // Call user function like uart_rx_callback()
    }
    
    // Action 3: Restart interrupt reception for next byte (continuous reception)
    HAL_UART_Receive_IT(huart_, &rx_data_, 1);  // Wait for next byte
}

/// @brief Set user-defined receive callback function
/// @param callback Function pointer to call when a byte is received
/// @note The callback runs in ISR context - keep it short and fast!
/// @note Set in main.cpp: set_rx_callback(uart_rx_callback)
void Uart::set_rx_callback(UartRxCallback callback) {
    rx_callback_ = callback;
}