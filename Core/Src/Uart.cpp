#include "uart.hpp"

/// @brief initialize static instance pointer
Uart* Uart::instance_ = nullptr;

Uart::Uart(UART_HandleTypeDef* huart) : huart_(huart), rx_buffer_(), rx_data_(0) {}

/// @brief start UART receive interrupt
void Uart::begin() {
    // start UART receive interrupt
    HAL_UART_Receive_IT(huart_, &rx_data_, 1);
}

/// @brief check how many bytes are available to read
size_t Uart::available() {
    return rx_buffer_.count();
}

/// @brief read a byte from buffer, return -1 if no data
int Uart::read() {
    uint8_t byte;
    if (rx_buffer_.pop(byte)) {
        return byte;
    }
    return -1; // no data
}

/// @brief UART interrupt service routine handler
void Uart::isr_handler() {
    rx_buffer_.push(rx_data_);
    HAL_UART_Receive_IT(huart_, &rx_data_, 1);
}