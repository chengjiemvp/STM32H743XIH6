#include "uart.hpp"
#include <stdio.h>

/// @brief initialize static variables
Uart* Uart::instance_ = nullptr;
bool Uart::initialized_ = false;
unsigned char Uart::instance_storage_[256];

Uart::Uart(UART_HandleTypeDef* huart) : tick_count_(0), huart_(huart), 
    rx_buffer_(), rx_data_(0), rx_callback_(nullptr)  {}

/// @brief start UART receive interrupt
void Uart::begin() {
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
    if (rx_callback_) {
        rx_callback_(rx_data_);
    } // if rx_callback_ is set, call it with received byte
    HAL_UART_Receive_IT(huart_, &rx_data_, 1);
}

/// @brief set receive callback
void Uart::set_rx_callback(UartRxCallback callback) {
    rx_callback_ = callback;
}