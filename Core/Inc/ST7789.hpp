#pragma once
#include "spi.h"
#include "gpio.h"
#include <cstdint>

class ST7789 {
public:
    ST7789(SPI_HandleTypeDef* hspi,
           GPIO_TypeDef* cs_port,  uint16_t cs_pin,
           GPIO_TypeDef* dc_port,  uint16_t dc_pin,
           GPIO_TypeDef* bl_port,  uint16_t bl_pin,
           GPIO_TypeDef* rst_port = nullptr, uint16_t rst_pin = 0);

    void init_basic();                // 软件复位 + 基础显示初始化
    void software_reset();            // 发送 0x01
    void fill_screen(uint16_t color); // 全屏填充
    void draw_single_test_pixel();    // 单点测试

private:
    void write_cmd(uint8_t cmd);
    void write_data(uint8_t data);
    void write_data_buf(const uint8_t* buf, uint32_t len);
    void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    SPI_HandleTypeDef* hspi_;
    GPIO_TypeDef* cs_port_; uint16_t cs_pin_;
    GPIO_TypeDef* dc_port_; uint16_t dc_pin_;
    GPIO_TypeDef* bl_port_; uint16_t bl_pin_;
    GPIO_TypeDef* rst_port_; uint16_t rst_pin_;
};