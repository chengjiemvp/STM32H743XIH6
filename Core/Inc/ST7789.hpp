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
    void display_test_colors();

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
};