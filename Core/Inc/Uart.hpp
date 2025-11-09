#pragma once

#include "main.hpp"
#include "RingBuffer.hpp"
#include <cstddef>

// define serial buffer size
constexpr size_t UART_RX_BUFFER_SIZE {128};

class Uart {
    public:
        uint8_t tick_count_;

        static Uart& get_instance() {
            while (instance_ == nullptr) {
            }
            return *instance_;
        } // get singleton instance

        static void init(UART_HandleTypeDef* huart) {
            if (instance_ == nullptr) {
                // create instance via new
                instance_ = new Uart(huart);
            }
        } // initialize singleton instance

        // start UART receive interrupt
        void begin();

        // check how many bytes are available to read
        size_t available();

        // read a byte from buffer
        int read();

        // UART interrupt service routine handler
        void isr_handler();

    private:
        // constructor is made private to prevent direct instance creation from outside
        Uart(UART_HandleTypeDef* huart);
        // static instance pointer
        static Uart* instance_;
        UART_HandleTypeDef* huart_;
        RingBuffer<uint8_t, UART_RX_BUFFER_SIZE> rx_buffer_;
        uint8_t rx_data_; // temporary storage for one byte data
};