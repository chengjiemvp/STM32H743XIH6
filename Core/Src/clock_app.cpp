#include "clock_app.hpp"
#include <stdio.h>
#include <math.h>
#include <string.h>  // for memcpy

// 静态表盘缓冲区：放回SDRAM（妥协方案）
// 内部SRAM配置复杂，暂时使用SDRAM
// TODO: 未来优化链接脚本以使用512KB AXI SRAM

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
    // 使用SDRAM的三个缓冲区
    buffer_[0] = (uint16_t*)0xC0000000;
    buffer_[1] = (uint16_t*)(0xC0000000 + WIDTH * HEIGHT * 2);
    static_dial_ = (uint16_t*)(0xC0000000 + WIDTH * HEIGHT * 4);  // 第3块buffer
    
    printf("[WAT] Buffers: [0]=0x%08X [1]=0x%08X Static=0x%08X\r\n",
           (unsigned int)buffer_[0], (unsigned int)buffer_[1], (unsigned int)static_dial_);
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

// RGB565颜色混合（alpha: 0-255）
uint16_t ClockApp::blend_color(uint16_t fg, uint16_t bg, uint8_t alpha) {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    
    // 解码RGB565
    uint8_t fg_r = (fg >> 11) & 0x1F;
    uint8_t fg_g = (fg >> 5) & 0x3F;
    uint8_t fg_b = fg & 0x1F;
    
    uint8_t bg_r = (bg >> 11) & 0x1F;
    uint8_t bg_g = (bg >> 5) & 0x3F;
    uint8_t bg_b = bg & 0x1F;
    
    // Alpha混合
    uint8_t r = (fg_r * alpha + bg_r * (255 - alpha)) / 255;
    uint8_t g = (fg_g * alpha + bg_g * (255 - alpha)) / 255;
    uint8_t b = (fg_b * alpha + bg_b * (255 - alpha)) / 255;
    
    return (r << 11) | (g << 5) | b;
}

// 抗锯齿像素（带alpha通道）
void ClockApp::set_pixel_aa(uint16_t* fb, uint16_t x, uint16_t y, uint16_t color, uint8_t alpha) {
    if (x >= WIDTH || y >= HEIGHT) return;
    uint16_t bg = fb[y * WIDTH + x];
    fb[y * WIDTH + x] = blend_color(color, bg, alpha);
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

// 简化抗锯齿直线（在普通线条基础上添加边缘柔化）
void ClockApp::draw_line_aa(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // 先绘制实心线条
    draw_line(fb, x0, y0, x1, y1, color);
    
    // 计算垂直方向，在边缘添加半透明像素
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1) return;
    
    // 垂直单位向量
    float nx = -dy / len;
    float ny = dx / len;
    
    // Bresenham遍历主线，在两侧添加淡化像素
    int16_t px = x0, py = y0;
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = abs(dx) - abs(dy);
    
    while (true) {
        // 两侧添加30%透明度的边缘像素
        int16_t ox1 = px + (int16_t)nx;
        int16_t oy1 = py + (int16_t)ny;
        int16_t ox2 = px - (int16_t)nx;
        int16_t oy2 = py - (int16_t)ny;
        
        set_pixel_aa(fb, ox1, oy1, color, 80);
        set_pixel_aa(fb, ox2, oy2, color, 80);
        
        if (px == x1 && py == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -abs(dy)) {
            err -= abs(dy);
            px += sx;
        }
        if (e2 < abs(dx)) {
            err += abs(dx);
            py += sy;
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

// 抗锯齿粗线条（简化版：普通粗线+边缘柔化）
void ClockApp::draw_thick_line_aa(uint16_t* fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t thickness, uint16_t color) {
    if (thickness == 1) {
        draw_line_aa(fb, x0, y0, x1, y1, color);
        return;
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len == 0) return;
    
    // 垂直方向单位向量
    float nx = -dy / len;
    float ny = dx / len;
    
    // 绘制实心粗线（中间部分）
    for (int16_t t = 0; t < thickness; t++) {
        float offset = t - thickness / 2.0f + 0.5f;
        int16_t ox = (int16_t)(nx * offset);
        int16_t oy = (int16_t)(ny * offset);
        draw_line(fb, x0 + ox, y0 + oy, x1 + ox, y1 + oy, color);
    }
    
    // 在外侧边缘添加半透明柔化
    float edge_offset1 = thickness / 2.0f + 0.5f;
    float edge_offset2 = -thickness / 2.0f - 0.5f;
    
    int16_t ox1 = (int16_t)(nx * edge_offset1);
    int16_t oy1 = (int16_t)(ny * edge_offset1);
    int16_t ox2 = (int16_t)(nx * edge_offset2);
    int16_t oy2 = (int16_t)(ny * edge_offset2);
    
    // 边缘线用60%透明度
    int16_t px = x0, py = y0;
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = abs(dx) - abs(dy);
    
    while (true) {
        set_pixel_aa(fb, px + ox1, py + oy1, color, 100);
        set_pixel_aa(fb, px + ox2, py + oy2, color, 100);
        
        if (px == x1 && py == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -abs(dy)) {
            err -= abs(dy);
            px += sx;
        }
        if (e2 < abs(dx)) {
            err += abs(dx);
            py += sy;
        }
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

// 预渲染静态表盘（只调用一次）
void ClockApp::render_static_dial() {
    printf("[WAT] Rendering static dial...\r\n");
    
    // 1. 填充背景（深墨黑）
    uint16_t bg_color = ST7789::rgb_to_rgb565(12, 12, 12);
    uint32_t bg_color32 = (bg_color << 16) | bg_color;
    uint32_t* ptr32 = (uint32_t*)static_dial_;
    for (uint32_t i = 0; i < (WIDTH * HEIGHT) / 2; i++) {
        ptr32[i] = bg_color32;
    }
    
    // 2. 绘制表盘装饰圈（香槟金）
    for (uint8_t i = 0; i < 3; i++) {
        draw_circle(static_dial_, CENTER_X, CENTER_Y, RADIUS + i, ST7789::rgb_to_rgb565(200, 170, 120));
    }
    for (uint8_t i = 0; i < 2; i++) {
        draw_circle(static_dial_, CENTER_X, CENTER_Y, RADIUS - 5 + i, ST7789::rgb_to_rgb565(150, 130, 100));
    }
    
    // 3. 60个刻度（抗锯齿）
    for (uint8_t i = 0; i < 60; i++) {
        float angle_rad = (i * 6.0f - 90.0f) * 3.14159f / 180.0f;
        uint16_t color;
        uint16_t thickness;
        int16_t inner_radius;
        
        if (i % 5 == 0) {
            // 大刻度（玫瑰金，加粗到4px）
            color = ST7789::rgb_to_rgb565(220, 150, 130);
            thickness = 4;
            inner_radius = RADIUS - 15;
        } else {
            // 小刻度（暗金，加粗到2px）
            color = ST7789::rgb_to_rgb565(120, 100, 80);
            thickness = 2;
            inner_radius = RADIUS - 8;
        }
        
        int16_t x1 = CENTER_X + (int16_t)(inner_radius * cosf(angle_rad));
        int16_t y1 = CENTER_Y + (int16_t)(inner_radius * sinf(angle_rad));
        int16_t x2 = CENTER_X + (int16_t)((RADIUS - 2) * cosf(angle_rad));
        int16_t y2 = CENTER_Y + (int16_t)((RADIUS - 2) * sinf(angle_rad));
        
        draw_thick_line_aa(static_dial_, x1, y1, x2, y2, thickness, color);
    }
    
    printf("[WAT] Static dial rendered!\r\n");
}

// 只绘制指针到缓冲区（基于静态表盘）
void ClockApp::draw_pointers_only(uint16_t* fb) {
    // 1. 从静态表盘快速复制（使用memcpy，编译器优化）
    uint32_t copy_start = DWT->CYCCNT;
    memcpy(fb, static_dial_, WIDTH * HEIGHT * sizeof(uint16_t));
    uint32_t copy_end = DWT->CYCCNT;
    uint32_t copy_cycles = copy_end - copy_start;
    
    // 2. 秒针（玫瑰金，长，3px）
    uint32_t ptr_start = DWT->CYCCNT;
    float sec_angle_rad = ((elapsed_ms_ % 60000) * 360.0f / 60000.0f - 90.0f) * 3.14159f / 180.0f;
    int16_t sec_x = CENTER_X + (int16_t)(88 * cosf(sec_angle_rad));
    int16_t sec_y = CENTER_Y + (int16_t)(88 * sinf(sec_angle_rad));
    draw_thick_line_aa(fb, CENTER_X, CENTER_Y, sec_x, sec_y, 3, ST7789::rgb_to_rgb565(220, 150, 130));
    
    // 3. 分钟指针（香槟金，中，5px）- 跳动形式
    uint32_t total_seconds = elapsed_ms_ / 1000;
    float min_angle_rad = (total_seconds * 6.0f - 90.0f) * 3.14159f / 180.0f;
    int16_t min_x = CENTER_X + (int16_t)(70 * cosf(min_angle_rad));
    int16_t min_y = CENTER_Y + (int16_t)(70 * sinf(min_angle_rad));
    draw_thick_line_aa(fb, CENTER_X, CENTER_Y, min_x, min_y, 5, ST7789::rgb_to_rgb565(200, 170, 120));
    
    // 4. 毫秒指针（银灰，3px粗，带运动模糊轨迹）
    float ms_angle_rad = ((elapsed_ms_ % 1000) * 360.0f / 1000.0f - 90.0f) * 3.14159f / 180.0f;
    
    // 绘制3帧运动模糊轨迹（减少撕裂感）
    uint16_t ms_color = ST7789::rgb_to_rgb565(180, 180, 180);
    for (int8_t trail = 2; trail >= 0; trail--) {
        float trail_angle = ms_angle_rad - (trail * 0.05f); // 向后偏移
        int16_t trail_len = 90 - trail * 5; // 渐短
        int16_t trail_x = CENTER_X + (int16_t)(trail_len * cosf(trail_angle));
        int16_t trail_y = CENTER_Y + (int16_t)(trail_len * sinf(trail_angle));
        
        if (trail == 0) {
            // 主指针：实心3px
            draw_thick_line_aa(fb, CENTER_X, CENTER_Y, trail_x, trail_y, 3, ms_color);
        } else {
            // 轨迹：半透明细线
            uint8_t alpha = (3 - trail) * 50; // 50, 100透明度
            int16_t px = CENTER_X, py = CENTER_Y;
            int16_t dx = trail_x - CENTER_X;
            int16_t dy = trail_y - CENTER_Y;
            int16_t sx = (dx > 0) ? 1 : -1;
            int16_t sy = (dy > 0) ? 1 : -1;
            int16_t err = abs(dx) - abs(dy);
            
            while (true) {
                set_pixel_aa(fb, px, py, ms_color, alpha);
                if (px == trail_x && py == trail_y) break;
                
                int16_t e2 = 2 * err;
                if (e2 > -abs(dy)) {
                    err -= abs(dy);
                    px += sx;
                }
                if (e2 < abs(dx)) {
                    err += abs(dx);
                    py += sy;
                }
            }
        }
    }
    uint32_t ptr_end = DWT->CYCCNT;
    uint32_t ptr_cycles = ptr_end - ptr_start;
    
    // 5. 中心装饰（玫瑰金+珍珠白）
    uint32_t circle_start = DWT->CYCCNT;
    fill_circle(fb, CENTER_X, CENTER_Y, 5, ST7789::rgb_to_rgb565(220, 150, 130));
    fill_circle(fb, CENTER_X, CENTER_Y, 3, ST7789::rgb_to_rgb565(240, 235, 230));
    uint32_t circle_end = DWT->CYCCNT;
    uint32_t circle_cycles = circle_end - circle_start;
    
    // 每100帧打印一次性能分析
    static uint32_t frame_count = 0;
    if (++frame_count >= 100) {
        printf("[PERF] Copy: %u us | Pointers: %u us | Circles: %u us | Total: %u us\r\n",
               (unsigned int)(copy_cycles / 480),
               (unsigned int)(ptr_cycles / 480),
               (unsigned int)(circle_cycles / 480),
               (unsigned int)((copy_cycles + ptr_cycles + circle_cycles) / 480));
        frame_count = 0;
    }
}

void ClockApp::draw_to_buffer(uint16_t* fb) {
    // 兼容旧接口：直接调用优化版本
    draw_pointers_only(fb);
}

void ClockApp::run() {
    printf("[WAT] Stopwatch ready. Auto-started!\r\n");
    
    // 自动启动秒表
    start();
    
    // 初始化DWT用于微秒级计时
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // **预渲染静态表盘（只执行一次）**
    render_static_dial();
    
    // 初始显示
    uint16_t* back_buffer = buffer_[current_buffer_idx_];
    draw_pointers_only(back_buffer);  // 使用优化版本
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
            
            // 在后台缓冲区绘制（只绘制指针）
            back_buffer = buffer_[current_buffer_idx_];
            draw_pointers_only(back_buffer);  // ⭐ 使用优化版本
            
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
            
            // 计算这段时间内实际执行的帧数
            uint32_t frame_count = period_ms / 10; // 理论帧数（每10ms一帧）
            uint32_t avg_frame_us = frame_count > 0 ? (busy_time_us_ / frame_count) : 0;
            
            // CPU占用率 = (总忙碌微秒 / 周期微秒) * 100
            //          = (忙碌us / (周期ms * 1000)) * 100
            //          = (忙碌us * 100) / (周期ms * 1000)
            //          = 忙碌us / (周期ms * 10)
            // 为了小数点后1位精度，分子先乘10：
            uint32_t cpu_percent_x10 = (busy_time_us_ * 10) / (period_ms * 10);
            
            // 额外显示：总忙碌时间（毫秒）
            uint32_t busy_time_ms = busy_time_us_ / 1000;
            
            printf("[WAT] %5u.%03u s | CPU: %2u.%u%% | Draw: %5u us/frame (%u frames) | Busy: %u ms\r\n", 
                   (unsigned int)(elapsed_ms_ / 1000), 
                   (unsigned int)(elapsed_ms_ % 1000),
                   (unsigned int)(cpu_percent_x10 / 10),
                   (unsigned int)(cpu_percent_x10 % 10),
                   (unsigned int)avg_frame_us,
                   (unsigned int)frame_count,
                   (unsigned int)busy_time_ms);
            
            last_cpu_calc_tick_ = now;
            busy_time_us_ = 0;
        }
        
        HAL_Delay(1);
    }
}
