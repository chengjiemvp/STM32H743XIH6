#pragma once

#include "stm32h7xx_hal.h"
#include <cstdint>

class ST7789 {
    public:
        ST7789(SPI_HandleTypeDef* hspi,
            GPIO_TypeDef* dc_port, uint16_t dc_pin,
            GPIO_TypeDef* bl_port, uint16_t bl_pin);

        void init_basic();
        void fill_screen(uint16_t color);
        void fill_screen_dma(uint16_t color);  // ⭐ DMA纯色填充
        void transmit_buffer_dma(uint16_t* buffer);  // ⭐ DMA传输framebuffer
        void update_from_buffer(uint16_t* buffer);  // ⭐ 轮询传输framebuffer
        void display_test_colors();
        // color cycle animation
        void color_cycle_loop();
        void clock_color_display();  // ⭐ 颜色时钟显示
        
        // 辅助函数：RGB565颜色混合
        static uint16_t blend_color(uint16_t color1, uint16_t color2, uint8_t ratio);
        static uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
        
        // DMA回调
        void dma_tx_cplt_callback();

    private:
        void write_cmd(uint8_t cmd);
        void write_data_8bit(uint8_t data);
        void write_data_16bit(uint16_t data);
        void write_data_buf(uint16_t* buf, uint16_t size);
        void set_addr_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
        void spi_set_datasize(uint16_t datasize);

        SPI_HandleTypeDef* hspi_;
        GPIO_TypeDef* dc_port_;
        uint16_t dc_pin_;
        GPIO_TypeDef* bl_port_;
        uint16_t bl_pin_;
        
        // 双缓冲相关
        uint16_t* current_buffer_;
        volatile bool is_transmitting_;
        
        // DMA传输状态
        uint8_t dma_chunk_count_;     // 当前chunk计数器(0或1)
        uint16_t* dma_next_ptr_;      // 下一个chunk的指针
};
