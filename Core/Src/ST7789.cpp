#include "ST7789.hpp"
#include <stdio.h>
#include <cstring>

#define TFT_W 240
#define TFT_H 280

// DC 引脚控制宏
#define LCD_DC_Command  HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET)
#define LCD_DC_Data     HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET)

ST7789::ST7789(SPI_HandleTypeDef* hspi,
               GPIO_TypeDef* dc_port, uint16_t dc_pin,
               GPIO_TypeDef* bl_port, uint16_t bl_pin)
    : hspi_(hspi), dc_port_(dc_port), dc_pin_(dc_pin),
      bl_port_(bl_port), bl_pin_(bl_pin) {}

void ST7789::spi_set_datasize(uint16_t datasize) {
    hspi_->Init.DataSize = datasize;
    HAL_SPI_Init(hspi_);
}

void ST7789::write_cmd(uint8_t cmd) {
    LCD_DC_Command;
    HAL_SPI_Transmit(hspi_, (uint8_t*)&cmd, 1, 100);
}

void ST7789::write_data_8bit(uint8_t data) {
    LCD_DC_Data;
    HAL_SPI_Transmit(hspi_, (uint8_t*)&data, 1, 100);
}

void ST7789::write_data_16bit(uint16_t data) {
    uint8_t buf[2];
    LCD_DC_Data;
    
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    
    HAL_SPI_Transmit(hspi_, buf, 2, 100);
}

void ST7789::set_addr_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // 设置列地址 (CASET - 0x2A)
    write_cmd(0x2A);
    write_data_16bit(x1);
    write_data_16bit(x2);
    
    // 设置行地址 (RASET - 0x2B)
    write_cmd(0x2B);
    write_data_16bit(y1);
    write_data_16bit(y2);
    
    // 开始写入显存 (RAMWR - 0x2C)
    write_cmd(0x2C);
}

void ST7789::init_basic() {
    // 打开背光
    HAL_GPIO_WritePin(bl_port_, bl_pin_, GPIO_PIN_SET);
    HAL_Delay(10);
    
    // 软件复位
    write_cmd(0x01);
    HAL_Delay(150);
    
    // 等待屏幕复位完成
    HAL_Delay(10);
    
    // =============== 完整的初始化序列（对标参考代码） ===============
    
    // 显存访问控制 (0x36) - 设置访问显存的方式
    write_cmd(0x36);
    write_data_8bit(0x00);  // 从上到下、从左到右，RGB 像素格式
    
    // 接口像素格式 (0x3A) - 设置使用 16 位色
    write_cmd(0x3A);
    write_data_8bit(0x05);  // 16 位 RGB565 色
    
    // ============== 分压比设置 (0xB2) ==============
    write_cmd(0xB2);
    write_data_8bit(0x0C);
    write_data_8bit(0x0C);
    write_data_8bit(0x00);
    write_data_8bit(0x33);
    write_data_8bit(0x33);
    
    // ============== 栅极电压设置 (0xB7) ==============
    write_cmd(0xB7);
    write_data_8bit(0x35);
    
    // ============== 公共电压设置 (0xBB) ==============
    write_cmd(0xBB);
    write_data_8bit(0x19);
    
    // ============== (0xC0) ==============
    write_cmd(0xC0);
    write_data_8bit(0x2C);
    
    // ============== VDV 和 VRH 来源设置 (0xC2) ==============
    write_cmd(0xC2);
    write_data_8bit(0x01);
    
    // ============== VRH 电压设置 (0xC3) ==============
    write_cmd(0xC3);
    write_data_8bit(0x12);
    
    // ============== VDV 电压设置 (0xC4) ==============
    write_cmd(0xC4);
    write_data_8bit(0x20);
    
    // ============== 帧率控制 (0xC6) ==============
    write_cmd(0xC6);
    write_data_8bit(0x0F);
    
    // ============== 电源控制 (0xD0) ==============
    write_cmd(0xD0);
    write_data_8bit(0xA4);
    write_data_8bit(0xA1);
    
    // ============== 正极电压伽马值设定 (0xE0) ==============
    write_cmd(0xE0);
    write_data_8bit(0xD0);
    write_data_8bit(0x04);
    write_data_8bit(0x0D);
    write_data_8bit(0x11);
    write_data_8bit(0x13);
    write_data_8bit(0x2B);
    write_data_8bit(0x3F);
    write_data_8bit(0x54);
    write_data_8bit(0x4C);
    write_data_8bit(0x18);
    write_data_8bit(0x0D);
    write_data_8bit(0x0B);
    write_data_8bit(0x1F);
    write_data_8bit(0x23);
    
    // ============== 负极电压伽马值设定 (0xE1) ==============
    write_cmd(0xE1);
    write_data_8bit(0xD0);
    write_data_8bit(0x04);
    write_data_8bit(0x0C);
    write_data_8bit(0x11);
    write_data_8bit(0x13);
    write_data_8bit(0x2C);
    write_data_8bit(0x3F);
    write_data_8bit(0x44);
    write_data_8bit(0x51);
    write_data_8bit(0x2F);
    write_data_8bit(0x1F);
    write_data_8bit(0x1F);
    write_data_8bit(0x20);
    write_data_8bit(0x23);
    
    // ============== 显示反演 (0x21) ==============
    write_cmd(0x21);
    
    // ============== 退出休眠 (0x11) ==============
    write_cmd(0x11);
    HAL_Delay(120);  // 关键延迟
    
    // ============== 打开显示 (0x29) ==============
    write_cmd(0x29);
    HAL_Delay(20);
    
    printf("ST7789 init done\r\n");
}

void ST7789::fill_screen(uint16_t color) {
    // 1. 设置地址窗口（包括发送 0x2C 命令进入数据模式）
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);
    
    // 2. 确保 DC 在数据模式（0x2C 后 DC 会被设为低，所以要重新设为高）
    LCD_DC_Data;
    
    // 3. 切换为 16 位数据宽度以提高效率
    spi_set_datasize(SPI_DATASIZE_16BIT);
    
    // 4. 直接发送颜色数据到屏幕显存
    // 使用循环发送每个像素（240 * 280 = 67200 像素）
    uint32_t pixel_count = TFT_W * TFT_H;
    for (uint32_t i = 0; i < pixel_count; i++) {
        HAL_SPI_Transmit(hspi_, (uint8_t*)&color, 1, 1000);
    }
    
    // 5. 切换回 8 位数据宽度（用于命令传输）
    spi_set_datasize(SPI_DATASIZE_8BIT);
    
    printf("fill_screen done, color=0x%04X\r\n", color);
}

void ST7789::display_test_colors() {
    // 红色
    fill_screen(0xF800);
    HAL_Delay(800);
    
    // 绿色
    fill_screen(0x07E0);
    HAL_Delay(800);
    
    // 蓝色
    fill_screen(0x001F);
    HAL_Delay(800);
}