#pragma once
#include "spi.h"
#include "gpio.h"
#include <cstdint>

class ST7789 {
    public:
        ST7789(SPI_HandleTypeDef* hspi);
        void init();             // display initialization

    private:
        void write_cmd(uint8_t cmd);
        void write_data(uint8_t data);
        void write_data_buf(const uint8_t* buf, uint32_t len);
        void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

        SPI_HandleTypeDef* hspi_;
};