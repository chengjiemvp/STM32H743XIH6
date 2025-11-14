/// @file Uart.hpp
/// @brief UART singleton class with interrupt-driven reception and ring buffer
///
/// UART RECEIVE CALLBACK FLOW (完整调用链):
/// ==========================================
/// When a byte is received by USART1 hardware, the following chain executes:
///
/// 1. Hardware Interrupt Triggered (USART1 RX完成)
///    ↓
/// 2. USART1_IRQHandler() [stm32h7xx_it.c] - 中断向量表入口
///    ↓
/// 3. HAL_UART_IRQHandler(&huart1) [HAL库] - HAL中断处理
///    ↓
/// 4. HAL_UART_RxCpltCallback(&huart1) [app_callbacks.cpp] - C/C++桥接层
///    ↓
/// 5. Uart::get_instance().isr_handler() [Uart.cpp] - C++类处理 (本文件)
///    ↓
/// 6. rx_buffer_.push(rx_data_) - 数据存入环形缓冲区
///    ↓
/// 7. rx_callback_(rx_data_) - 调用用户回调函数指针
///    ↓
/// 8. uart_rx_callback(byte) [app_callbacks.cpp] - 用户自定义处理
///    ↓
/// 9. printf("%c", byte) - 打印字符（当前实现）
///
/// USAGE PATTERN (使用方法):
/// ========================
/// // In main.cpp:
/// Uart::init(&huart1);                                  // 1. 初始化单例
/// Uart::get_instance().set_rx_callback(uart_rx_callback); // 2. 设置回调
/// Uart::get_instance().begin();                         // 3. 启动接收中断
///
/// TWO WAYS TO ACCESS DATA (两种数据访问方式):
/// =========================================
/// 1. Callback (实时处理): isr_handler() → rx_callback_() → immediate processing
/// 2. Polling (轮询读取): isr_handler() → rx_buffer_ → available() + read()
///
/// @see docs/UART_CALLBACK_FLOW.md for detailed explanation

#pragma  once
#include "main.hpp"
#include "RingBuffer.hpp"
#include <cstddef>
#include <stdio.h>

// define serial buffer size
constexpr size_t UART_RX_BUFFER_SIZE {128};
using UartRxCallback = void (*)(uint8_t byte);

void uart_rx_callback(uint8_t byte); // forward declaraion

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

        // set receive callback
        void set_rx_callback(UartRxCallback callback);

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
        UartRxCallback rx_callback_;
};
