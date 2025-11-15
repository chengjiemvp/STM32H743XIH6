#include "ST7789.hpp"
#include "uart.hpp"
#include <stdio.h>
#include <cstring>

#define TFT_W 240
#define TFT_H 280
#define PIXEL_COUNT (TFT_W * TFT_H)

// SDRAM 中的双缓冲
#define FRAME_BUFFER_0 ((uint16_t*)0xC0000000)           // 帧缓冲 A
#define FRAME_BUFFER_1 ((uint16_t*)(0xC0000000 + (PIXEL_COUNT * 2)))  // 帧缓冲 B

// DC 引脚控制宏
#define LCD_DC_Command  HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET)
#define LCD_DC_Data     HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET)

ST7789::ST7789(
    SPI_HandleTypeDef* hspi,
    GPIO_TypeDef* dc_port, 
    uint16_t dc_pin,
    GPIO_TypeDef* bl_port, 
    uint16_t bl_pin
) :
    hspi_(hspi),
    dc_port_(dc_port),
    dc_pin_(dc_pin),
    bl_port_(bl_port),
    bl_pin_(bl_pin),
    current_buffer_(FRAME_BUFFER_0),
    is_transmitting_(false),
    dma_chunk_count_(0),
    dma_next_ptr_(nullptr) {}

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
    const uint16_t X_OFFSET = 0;
    const uint16_t Y_OFFSET = 20;
    
    write_cmd(0x2A);
    write_data_16bit(x1 + X_OFFSET);
    write_data_16bit(x2 + X_OFFSET);
    write_cmd(0x2B);
    write_data_16bit(y1 + Y_OFFSET);
    write_data_16bit(y2 + Y_OFFSET);
    write_cmd(0x2C);
}

void ST7789::init_basic() {
    HAL_GPIO_WritePin(bl_port_, bl_pin_, GPIO_PIN_SET);
    HAL_Delay(10);
    write_cmd(0x01);
    HAL_Delay(150);
    HAL_Delay(10);
    write_cmd(0x36);
    write_data_8bit(0x00);
    write_cmd(0x3A);
    write_data_8bit(0x05);
    write_cmd(0xB2);
    write_data_8bit(0x0C);
    write_data_8bit(0x0C);
    write_data_8bit(0x00);
    write_data_8bit(0x33);
    write_data_8bit(0x33);
    write_cmd(0xB7);
    write_data_8bit(0x35);
    write_cmd(0xBB);
    write_data_8bit(0x19);
    write_cmd(0xC0);
    write_data_8bit(0x2C);
    write_cmd(0xC2);
    write_data_8bit(0x01);
    write_cmd(0xC3);
    write_data_8bit(0x12);
    write_cmd(0xC4);
    write_data_8bit(0x20);
    write_cmd(0xC6);
    write_data_8bit(0x0F);
    write_cmd(0xD0);
    write_data_8bit(0xA4);
    write_data_8bit(0xA1);
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
    write_cmd(0x21);
    write_cmd(0x11);
    HAL_Delay(120);
    write_cmd(0x29);
    HAL_Delay(20);
}

// ========== 双缓冲 + 同步版本 ==========
void ST7789::fill_screen(uint16_t color) {
    // 等待之前的传输完成
    while (is_transmitting_) {
        HAL_Delay(1);
    }
    
    // 获取要填充的缓冲区（与当前显示的不同）
    uint16_t* fill_buffer = (current_buffer_ == FRAME_BUFFER_0) ? FRAME_BUFFER_1 : FRAME_BUFFER_0;
    
    // 1. 快速填充背缓冲（32 位写入）
    uint32_t color32 = (color << 16) | color;
    uint32_t* ptr32 = (uint32_t*)fill_buffer;
    for (uint32_t i = 0; i < PIXEL_COUNT / 2; i++) {
        ptr32[i] = color32;
    }
    
    // 2. 清除 D-Cache
    SCB_CleanInvalidateDCache_by_Addr((uint32_t*)fill_buffer, PIXEL_COUNT * 2);
    // 3. 设置显示窗口
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);
    // 4. DC 切到数据模式
    LCD_DC_Data;
    HAL_Delay(1);
    // 5. 16 位 SPI 宽度
    spi_set_datasize(SPI_DATASIZE_16BIT);
    // 6. 标记开始传输
    is_transmitting_ = true;
    // 7. 发送缓冲数据
    #define LARGE_CHUNK 4096
    uint32_t remaining = PIXEL_COUNT;
    uint16_t* ptr = fill_buffer;
    
    while (remaining > 0) {
        uint32_t chunk = (remaining > LARGE_CHUNK) ? LARGE_CHUNK : remaining;
        HAL_SPI_Transmit(hspi_, (uint8_t*)ptr, chunk, 10000);
        ptr += chunk;
        remaining -= chunk;
    }
    
    // 8. 标记传输完成
    is_transmitting_ = false;
    // 10. 切换当前缓冲指针
    current_buffer_ = fill_buffer;
    // 11. 切回 8 位
    spi_set_datasize(SPI_DATASIZE_8BIT);
}

// ========== 从外部buffer更新屏幕 ==========
void ST7789::update_from_buffer(uint16_t* buffer) {
    while (is_transmitting_) {
        HAL_Delay(1);
    }
    
    // 直接在SDRAM上做字节交换（原地修改）
    for (uint32_t i = 0; i < PIXEL_COUNT; i++) {
        buffer[i] = __REV16(buffer[i]);  // 字节交换
    }
    
    // 清理D-Cache
    SCB_CleanDCache_by_Addr((uint32_t*)buffer, PIXEL_COUNT * 2);
    
    // 设置地址窗口
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);
    LCD_DC_Data;
    HAL_Delay(1);
    
    // 切换到16位模式
    spi_set_datasize(SPI_DATASIZE_16BIT);
    
    is_transmitting_ = true;
    
    // 使用轮询模式传输
    #define LARGE_CHUNK 4096
    uint32_t remaining = PIXEL_COUNT;
    uint16_t* ptr = buffer;
    
    while (remaining > 0) {
        uint32_t chunk = (remaining > LARGE_CHUNK) ? LARGE_CHUNK : remaining;
        HAL_SPI_Transmit(hspi_, (uint8_t*)ptr, chunk, 10000);
        ptr += chunk;
        remaining -= chunk;
    }
    
    is_transmitting_ = false;
    
    // 恢复buffer的字节序（以便下次绘制）
    for (uint32_t i = 0; i < PIXEL_COUNT; i++) {
        buffer[i] = __REV16(buffer[i]);
    }
    
    // 切回8位模式
    spi_set_datasize(SPI_DATASIZE_8BIT);
}

// ========== DMA版本 ==========
void ST7789::fill_screen_dma(uint16_t color) {
    while (is_transmitting_) {
        HAL_Delay(1);
    }
    
    uint16_t* fill_buffer = (current_buffer_ == FRAME_BUFFER_0) ? FRAME_BUFFER_1 : FRAME_BUFFER_0;
    
    // 填充缓冲区
    uint32_t color32 = (color << 16) | color;
    uint32_t* ptr32 = (uint32_t*)fill_buffer;
    for (uint32_t i = 0; i < PIXEL_COUNT / 2; i++) {
        ptr32[i] = color32;
    }
    
    // 清除D-Cache
    SCB_CleanDCache_by_Addr((uint32_t*)fill_buffer, PIXEL_COUNT * 2);
    
    // 设置地址窗口
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);
    LCD_DC_Data;
    HAL_Delay(1);
    
    // 切换到16位模式
    spi_set_datasize(SPI_DATASIZE_16BIT);
    
    is_transmitting_ = true;
    dma_chunk_count_ = 0;
    
    // 分成两个chunk传输
    // 总共67200个像素 = 67200个halfword
    // 每个chunk传输33600个halfword
    uint16_t* ptr = fill_buffer;
    uint32_t chunk_size = 33600;  // 半帧的halfword数量
    dma_next_ptr_ = ptr + chunk_size;
    
    // 启动第一个chunk的DMA传输
    HAL_SPI_Transmit_DMA(hspi_, (uint8_t*)ptr, chunk_size);
}

// DMA完成回调
void ST7789::dma_tx_cplt_callback() {
    if (dma_chunk_count_ == 0) {
        // 第一个chunk完成，启动第二个
        dma_chunk_count_++;
        HAL_SPI_Transmit_DMA(hspi_, (uint8_t*)dma_next_ptr_, 33600);
    } else {
        // 全部完成
        dma_chunk_count_ = 0;
        is_transmitting_ = false;
        current_buffer_ = (current_buffer_ == FRAME_BUFFER_0) ? FRAME_BUFFER_1 : FRAME_BUFFER_0;
        
        // 切回8位模式
        spi_set_datasize(SPI_DATASIZE_8BIT);
    }
}

// ========== DMA传输外部framebuffer ==========
void ST7789::transmit_buffer_dma(uint16_t* buffer) {
    while (is_transmitting_) {
        HAL_Delay(1);
    }
    
    // 清除D-Cache
    SCB_CleanDCache_by_Addr((uint32_t*)buffer, PIXEL_COUNT * 2);
    
    // 设置地址窗口
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);
    LCD_DC_Data;
    HAL_Delay(1);
    
    // 切换到16位模式
    spi_set_datasize(SPI_DATASIZE_16BIT);
    
    is_transmitting_ = true;
    dma_chunk_count_ = 0;
    
    // 分成两个chunk传输
    uint16_t* ptr = buffer;
    uint32_t chunk_size = 33600;
    dma_next_ptr_ = ptr + chunk_size;
    
    // 启动第一个chunk的DMA传输
    HAL_SPI_Transmit_DMA(hspi_, (uint8_t*)ptr, chunk_size);
}

void ST7789::display_test_colors() {
    fill_screen(0xF800);  // 红
    HAL_Delay(800);
    
    fill_screen(0x07E0);  // 绿
    HAL_Delay(800);
    
    fill_screen(0x001F);  // 蓝
    HAL_Delay(800);
}

// ========== 颜色辅助函数 ==========
uint16_t ST7789::rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t ST7789::blend_color(uint16_t color1, uint16_t color2, uint8_t ratio) {
    // 提取RGB分量
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    // 线性插值
    uint8_t r = (r1 * (255 - ratio) + r2 * ratio) / 255;
    uint8_t g = (g1 * (255 - ratio) + g2 * ratio) / 255;
    uint8_t b = (b1 * (255 - ratio) + b2 * ratio) / 255;
    
    return (r << 11) | (g << 5) | b;
}

// ========== 颜色循环动画（无限循环）==========
void ST7789::color_cycle_loop() {
    // ⭐ 选择传输模式：true=DMA，false=轮询
    const bool use_dma = true;  // 启用DMA测试
    
    // 使用更高精度的色调值（0-3600，即0.1度精度）
    uint32_t hue_x10 = 0;  // 色调 × 10
    uint32_t frame_count = 0;
    uint32_t last_fps_print = HAL_GetTick();
    
    while (1) {
        uint32_t frame_start = HAL_GetTick();
        
        // 改进的HSV到RGB转换，使用高精度计算获得更平滑的过渡
        // hue_x10: 0-3599 (0.1度精度)
        uint8_t r, g, b;
        
        // 使用600度单位（0-5999），每个区域1000个单位
        uint32_t hue_scaled = (hue_x10 * 10) / 6;  // 0-5999
        uint16_t region = hue_scaled / 1000;         // 0-5
        uint16_t remainder = hue_scaled % 1000;      // 0-999
        
        // 平滑插值计算（0-255范围）
        uint16_t rising = (remainder * 255) / 999;
        uint16_t falling = 255 - rising;
        
        switch (region) {
            case 0:  r = 255; g = rising; b = 0; break;
            case 1:  r = falling; g = 255; b = 0; break;
            case 2:  r = 0; g = 255; b = rising; break;
            case 3:  r = 0; g = falling; b = 255; break;
            case 4:  r = rising; g = 0; b = 255; break;
            default: r = 255; g = 0; b = falling; break;
        }
        
        // 转换为RGB565并填充整个屏幕
        uint16_t color = rgb_to_rgb565(r, g, b);
        
        // ⭐ 根据配置选择传输模式
        if (use_dma) {
            fill_screen_dma(color);
        } else {
            fill_screen(color);
        }
        
        uint32_t frame_end = HAL_GetTick();
        uint32_t frame_time = frame_end - frame_start;
        
        // 极慢增加色调（每帧0.1度），完全消除撕裂视觉
        hue_x10 += 5;  // 每帧增加0.1度 × N
        if (hue_x10 >= 3600) {
            hue_x10 = 0;
        }
        
        frame_count++;
        
        // 每5秒打印一次FPS统计
        if (frame_end - last_fps_print >= 20000) {
            uint32_t elapsed = frame_end - last_fps_print;
            uint32_t fps_x10 = (frame_count * 10000) / elapsed;  // FPS * 10
            printf("[FPS] %lu.%lu fps, frame_time=%lums, frames=%lu, hue=%lu.%lu\r\n", 
                   (unsigned long)(fps_x10 / 10), 
                   (unsigned long)(fps_x10 % 10),
                   (unsigned long)frame_time,
                   (unsigned long)frame_count,
                   (unsigned long)(hue_x10 / 10),
                   (unsigned long)(hue_x10 % 10));
            frame_count = 0;
            last_fps_print = frame_end;
        }
    }
}

// ========== 颜色时钟显示 ==========
void ST7789::clock_color_display() {
    // ⭐ 设置初始时间为12:30:45（更容易看到颜色）
    uint8_t hours = 12;
    uint8_t minutes = 30;
    uint8_t seconds = 45;
    
    uint32_t last_update = HAL_GetTick();
    uint32_t last_print = HAL_GetTick();
    
    printf("[CLOCK] Color Clock Started!\r\n");
    printf("[CLOCK] Initial time: %02d:%02d:%02d\r\n", hours, minutes, seconds);
    printf("[CLOCK] Screen color represents time:\r\n");
    printf("[CLOCK]   Red   = Hours   (0-23)\r\n");
    printf("[CLOCK]   Green = Minutes (0-59)\r\n");
    printf("[CLOCK]   Blue  = Seconds (0-59)\r\n\r\n");
    
    // 立即显示一次
    uint8_t r = (hours * 255) / 23;
    uint8_t g = (minutes * 255) / 59;
    uint8_t b = (seconds * 255) / 59;
    uint16_t color = rgb_to_rgb565(r, g, b);
    printf("[CLOCK] First frame - Color: R=%d G=%d B=%d (0x%04X)\r\n", r, g, b, color);
    fill_screen_dma(color);
    
    // ⭐ 等待第一帧传输完成
    while (is_transmitting_) {
        HAL_Delay(1);
    }
    printf("[CLOCK] First frame transmitted successfully!\r\n");
    
    while (1) {
        uint32_t now = HAL_GetTick();
        
        // 每秒更新
        if (now - last_update >= 1000) {
            last_update = now;
            
            // 时间递增
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                    if (hours >= 24) {
                        hours = 0;
                    }
                }
            }
            
            // 映射时间到颜色
            uint8_t r = (hours * 255) / 23;      // 0-23 → 0-255
            uint8_t g = (minutes * 255) / 59;    // 0-59 → 0-255
            uint8_t b = (seconds * 255) / 59;    // 0-59 → 0-255
            
            uint16_t color = rgb_to_rgb565(r, g, b);
            fill_screen_dma(color);
            
            // ⭐ 等待DMA传输完成（关键！）
            while (is_transmitting_) {
                HAL_Delay(1);
            }
            
            // 每5秒打印一次时间
            if (now - last_print >= 5000) {
                last_print = now;
                printf("[CLOCK] %02d:%02d:%02d - Color: R=%d G=%d B=%d\r\n",
                       hours, minutes, seconds, r, g, b);
            }
        }
        
        HAL_Delay(10);
    }
}
