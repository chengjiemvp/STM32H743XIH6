#pragma once

#include "main.hpp"
#include "RingBuffer.hpp"
#include <cstddef>
#include <stdio.h>

// define serial buffer size
constexpr size_t UART_RX_BUFFER_SIZE {128};

class Uart {
    public:
        uint8_t tick_count_;

        static Uart& get_instance() {
            while (!initialized_) {
                Error_Handler();
            }
            return *instance_;
        } // get singleton instance

        static void init(UART_HandleTypeDef* huart) {
            if (!initialized_) {
                printf("[DEBUG] Uart::init called with huart=%p\r\n", (void*)huart);
                if (huart != nullptr) {
                    printf("[DEBUG] huart->Instance=%p\r\n", (void*)huart->Instance);
                }
                // Use placement new on static storage instead of dynamic allocation
                // This prevents the instance from being allocated in SDRAM
                instance_ = new (&instance_storage_) Uart(huart);
                initialized_ = true;
                printf("[DEBUG] Uart instance created at %p\r\n", (void*)instance_);
                if (instance_ != nullptr && instance_->huart_ != nullptr) {
                    printf("[DEBUG] instance_->huart_=%p, instance_->huart_->Instance=%p\r\n", 
                           (void*)instance_->huart_, (void*)instance_->huart_->Instance);
                }
            }
        } // initialize singleton instance

        static bool is_initialized() {
            return initialized_;
        } // check if singleton is initialized

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
        
        // static instance pointer and flag
        static Uart* instance_;
        static bool initialized_;
        
        // Static storage for the singleton instance (prevents SDRAM allocation)
        // Use max size estimate for storage
        static unsigned char instance_storage_[256];
        
        UART_HandleTypeDef* huart_;
        RingBuffer<uint8_t, UART_RX_BUFFER_SIZE> rx_buffer_;
        uint8_t rx_data_; // temporary storage for one byte data
};
