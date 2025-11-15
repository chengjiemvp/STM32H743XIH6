#pragma once

#include "stm32h7xx_hal.h"
#include "ST7789.hpp"
#include <cstdint>

/// @brief 基于DMA双缓冲的秒表应用（平滑指针）
class ClockApp {
public:
    ClockApp(ST7789* lcd);
    
    void start();   // 启动秒表
    void stop();    // 停止秒表
    void reset();   // 重置秒表
    void run();     // 主循环
    
private:
    static constexpr uint16_t WIDTH = 240;
    static constexpr uint16_t HEIGHT = 280;
    static constexpr uint16_t CENTER_X = 120;
    static constexpr uint16_t CENTER_Y = 140;
    static constexpr uint16_t RADIUS = 100;
    
    // 双缓冲区（在SDRAM中）
    uint16_t* buffer_[2];
    uint8_t current_buffer_idx_;
    
    // 静态表盘缓冲区（暂时仍在SDRAM，内存配置待优化）
    uint16_t* static_dial_;
    
    ST7789* lcd_;
    
    // 秒表状态
    bool is_running_;
    uint32_t elapsed_ms_;  // 已过时间（毫秒）
    uint32_t last_update_tick_;
    
    // CPU占用率统计
    uint32_t last_cpu_calc_tick_;
    uint32_t busy_time_us_;
    float cpu_usage_;
    
    // 绘制函数
    void draw_to_buffer(uint16_t* fb);
    void render_static_dial();  // 预渲染静态表盘（只调用一次）
    void draw_pointers_only(uint16_t* fb);  // 只绘制指针到缓冲区
    
    // 图形绘制基础函数
    void set_pixel(uint16_t* fb, uint16_t x, uint16_t y, uint16_t color);
    void set_pixel_aa(uint16_t* fb, uint16_t x, uint16_t y, uint16_t color, uint8_t alpha);
    void draw_line(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void draw_line_aa(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void draw_thick_line(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, uint16_t color);
    void draw_thick_line_aa(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, uint16_t color);
    void draw_circle(uint16_t* fb, uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
    void fill_circle(uint16_t* fb, uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
    
    // 辅助函数
    uint16_t blend_color(uint16_t fg, uint16_t bg, uint8_t alpha);
    
    // 查表sin/cos
    int16_t get_sin(uint16_t angle);
    int16_t get_cos(uint16_t angle);
};
