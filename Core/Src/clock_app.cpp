#include "clock_app.hpp"
#include <stdio.h>
#include <math.h>

// Sin/Cos查找表（每6度一个值，0-354度，共60个值）
static const int16_t sin_table[60] = {
    0, 105, 208, 309, 407, 500, 588, 669, 743, 809,
    866, 914, 951, 978, 995, 1000, 995, 978, 951, 914,
    866, 809, 743, 669, 588, 500, 407, 309, 208, 105,
    0, -105, -208, -309, -407, -500, -588, -669, -743, -809,
    -866, -914, -951, -978, -995, -1000, -995, -978, -951, -914,
    -866, -809, -743, -669, -588, -500, -407, -309, -208, -105
};

ClockApp::ClockApp(ST7789* lcd)
    : current_buffer_idx_(0), lcd_(lcd), is_running_(false), 
      elapsed_ms_(0), last_update_tick_(0), last_cpu_calc_tick_(0),
      busy_time_us_(0), cpu_usage_(0.0f) {
    // 使用SDRAM的两个缓冲区
    buffer_[0] = (uint16_t*)0xC0000000;
    buffer_[1] = (uint16_t*)(0xC0000000 + WIDTH * HEIGHT * 2);
}

void ClockApp::start() {
    if (!is_running_) {
        is_running_ = true;
        last_update_tick_ = HAL_GetTick();
        printf("[WAT] Started\r\n");
    }
}

void ClockApp::stop() {
    if (is_running_) {
        is_running_ = false;
        printf("[WAT] Stopped at %u.%03u s\r\n", 
               (unsigned int)(elapsed_ms_ / 1000), 
               (unsigned int)(elapsed_ms_ % 1000));
    }
}

void ClockApp::reset() {
    elapsed_ms_ = 0;
    last_update_tick_ = HAL_GetTick();
    printf("[WAT] Reset\r\n");
}

int16_t ClockApp::get_sin(uint16_t angle) {
    angle = angle % 360;
    uint16_t index = angle / 6;
    return sin_table[index];
}

int16_t ClockApp::get_cos(uint16_t angle) {
    return get_sin(angle + 90);
}

void ClockApp::set_pixel(uint16_t* fb, uint16_t x, uint16_t y, uint16_t color) {
    if (x >= WIDTH || y >= HEIGHT) return;
    fb[y * WIDTH + x] = color;
}

void ClockApp::draw_line(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx - dy;
    
    while (true) {
        set_pixel(fb, x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void ClockApp::draw_thick_line(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, uint16_t color) {
    for (uint16_t t = 0; t < thickness; t++) {
        int16_t offset = t - thickness / 2;
        int16_t dx = x1 - x0;
        int16_t dy = y1 - y0;
        float len = sqrt(dx * dx + dy * dy);
        if (len == 0) continue;
        
        int16_t ox = (int16_t)(-dy * offset / len);
        int16_t oy = (int16_t)(dx * offset / len);
        
        draw_line(fb, x0 + ox, y0 + oy, x1 + ox, y1 + oy, color);
    }
}

void ClockApp::draw_circle(uint16_t* fb, uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    int16_t x = r;
    int16_t y = 0;
    int16_t err = 0;
    
    while (x >= y) {
        set_pixel(fb, cx + x, cy + y, color);
        set_pixel(fb, cx + y, cy + x, color);
        set_pixel(fb, cx - y, cy + x, color);
        set_pixel(fb, cx - x, cy + y, color);
        set_pixel(fb, cx - x, cy - y, color);
        set_pixel(fb, cx - y, cy - x, color);
        set_pixel(fb, cx + y, cy - x, color);
        set_pixel(fb, cx + x, cy - y, color);
        
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void ClockApp::fill_circle(uint16_t* fb, uint16_t cx, uint16_t cy, uint16_t r, uint16_t color) {
    for (int16_t y = -r; y <= r; y++) {
        for (int16_t x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                set_pixel(fb, cx + x, cy + y, color);
            }
        }
    }
}

void ClockApp::draw_to_buffer(uint16_t* fb) {
    // 1. 填充背景（深墨黑）
    uint16_t bg_color = ST7789::rgb_to_rgb565(12, 12, 12);
    uint32_t bg_color32 = (bg_color << 16) | bg_color;
    uint32_t* ptr32 = (uint32_t*)fb;
    for (uint32_t i = 0; i < (WIDTH * HEIGHT) / 2; i++) {
        ptr32[i] = bg_color32;
    }
    
    // 2. 绘制表盘装饰圈（香槟金）
    for (uint8_t i = 0; i < 3; i++) {
        draw_circle(fb, CENTER_X, CENTER_Y, RADIUS + i, ST7789::rgb_to_rgb565(200, 170, 120));
    }
    for (uint8_t i = 0; i < 2; i++) {
        draw_circle(fb, CENTER_X, CENTER_Y, RADIUS - 5 + i, ST7789::rgb_to_rgb565(150, 130, 100));
    }
    
    // 3. 60个刻度
    for (uint8_t i = 0; i < 60; i++) {
        float angle_rad = (i * 6.0f - 90.0f) * 3.14159f / 180.0f;
        uint16_t color;
        uint16_t thickness;
        int16_t inner_radius;
        
        if (i % 5 == 0) {
            // 大刻度（玫瑰金）
            color = ST7789::rgb_to_rgb565(220, 150, 130);
            thickness = 3;
            inner_radius = RADIUS - 15;
        } else {
            // 小刻度（暗金）
            color = ST7789::rgb_to_rgb565(120, 100, 80);
            thickness = 1;
            inner_radius = RADIUS - 8;
        }
        
        int16_t x1 = CENTER_X + (int16_t)(inner_radius * cosf(angle_rad));
        int16_t y1 = CENTER_Y + (int16_t)(inner_radius * sinf(angle_rad));
        int16_t x2 = CENTER_X + (int16_t)((RADIUS - 2) * cosf(angle_rad));
        int16_t y2 = CENTER_Y + (int16_t)((RADIUS - 2) * sinf(angle_rad));
        
        if (thickness > 1) {
            draw_thick_line(fb, x1, y1, x2, y2, thickness, color);
        } else {
            draw_line(fb, x1, y1, x2, y2, color);
        }
    }
    
    // 4. 计算指针角度（平滑移动）
    // 秒针（玫瑰金，长）
    float sec_angle_rad = ((elapsed_ms_ % 60000) * 360.0f / 60000.0f - 90.0f) * 3.14159f / 180.0f;
    int16_t sec_x = CENTER_X + (int16_t)(88 * cosf(sec_angle_rad));
    int16_t sec_y = CENTER_Y + (int16_t)(88 * sinf(sec_angle_rad));
    draw_thick_line(fb, CENTER_X, CENTER_Y, sec_x, sec_y, 2, ST7789::rgb_to_rgb565(220, 150, 130));
    
    // 分钟指针（香槟金，中）- 跳动形式
    uint32_t total_seconds = elapsed_ms_ / 1000;
    float min_angle_rad = (total_seconds * 6.0f - 90.0f) * 3.14159f / 180.0f;
    int16_t min_x = CENTER_X + (int16_t)(70 * cosf(min_angle_rad));
    int16_t min_y = CENTER_Y + (int16_t)(70 * sinf(min_angle_rad));
    draw_thick_line(fb, CENTER_X, CENTER_Y, min_x, min_y, 3, ST7789::rgb_to_rgb565(200, 170, 120));
    
    // 毫秒指针（银灰，细长）
    float ms_angle_rad = ((elapsed_ms_ % 1000) * 360.0f / 1000.0f - 90.0f) * 3.14159f / 180.0f;
    int16_t ms_x = CENTER_X + (int16_t)(95 * cosf(ms_angle_rad));
    int16_t ms_y = CENTER_Y + (int16_t)(95 * sinf(ms_angle_rad));
    draw_line(fb, CENTER_X, CENTER_Y, ms_x, ms_y, ST7789::rgb_to_rgb565(180, 180, 180));
    
    // 中心装饰（玫瑰金+珍珠白）
    fill_circle(fb, CENTER_X, CENTER_Y, 5, ST7789::rgb_to_rgb565(220, 150, 130));
    fill_circle(fb, CENTER_X, CENTER_Y, 3, ST7789::rgb_to_rgb565(240, 235, 230));
}

void ClockApp::run() {
    printf("[WAT] Stopwatch ready. Auto-started!\r\n");
    
    // 自动启动秒表
    start();
    
    // 初始化DWT用于微秒级计时
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // 初始显示
    uint16_t* back_buffer = buffer_[current_buffer_idx_];
    draw_to_buffer(back_buffer);
    lcd_->fill_screen_dma(ST7789::rgb_to_rgb565(15, 25, 45));
    HAL_Delay(100);
    
    uint32_t last_draw = HAL_GetTick();
    last_cpu_calc_tick_ = HAL_GetTick();
    busy_time_us_ = 0;
    
    while (1) {
        uint32_t now = HAL_GetTick();
        
        // 更新已过时间
        if (is_running_) {
            uint32_t delta = now - last_update_tick_;
            elapsed_ms_ += delta;
            last_update_tick_ = now;
        }
        
        // 每10ms更新一次显示（100 FPS）
        if (now - last_draw >= 10) {
            last_draw = now;
            
            // ===== 开始测量CPU时间 =====
            uint32_t work_start = DWT->CYCCNT;
            
            // 在后台缓冲区绘制
            back_buffer = buffer_[current_buffer_idx_];
            draw_to_buffer(back_buffer);
            
            // 使用DMA传输framebuffer（异步）
            lcd_->transmit_buffer_dma(back_buffer);
            
            // ===== 结束测量 =====
            uint32_t work_end = DWT->CYCCNT;
            uint32_t work_cycles = work_end - work_start;
            uint32_t work_us = work_cycles / 480;  // 480MHz CPU
            busy_time_us_ += work_us;
            
            // 切换缓冲区
            current_buffer_idx_ = 1 - current_buffer_idx_;
        }
        
        // 每秒计算一次CPU占用率
        if (now - last_cpu_calc_tick_ >= 1000) {
            uint32_t period_ms = now - last_cpu_calc_tick_;
            
            // CPU占用率 = (忙碌微秒数 / 1000) / 周期毫秒数 * 100
            //          = (忙碌us * 100) / (周期ms * 1000)
            //          = 忙碌us / (周期ms * 10)
            // 为了小数点后1位精度，先乘10：
            uint32_t cpu_percent_x10 = (busy_time_us_ * 10) / (period_ms * 10);
            
            // 计算这段时间内实际执行的帧数
            uint32_t frame_count = busy_time_us_ > 0 ? (period_ms / 10) : 0;
            uint32_t avg_frame_us = frame_count > 0 ? (busy_time_us_ / frame_count) : 0;
            
            printf("[WAT] %5u.%03u s | CPU: %u.%u%% | Draw: %u us (%u frames)\r\n", 
                   (unsigned int)(elapsed_ms_ / 1000), 
                   (unsigned int)(elapsed_ms_ % 1000),
                   (unsigned int)(cpu_percent_x10 / 10),
                   (unsigned int)(cpu_percent_x10 % 10),
                   (unsigned int)avg_frame_us,
                   (unsigned int)frame_count);
            
            last_cpu_calc_tick_ = now;
            busy_time_us_ = 0;
        }
        
        HAL_Delay(1);
    }
}
