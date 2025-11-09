#include "ST7789.hpp"
#include "stm32h7xx_hal.h"

#define TFT_W 240
#define TFT_H 280

static inline void ensure_spi_enabled(SPI_HandleTypeDef* h) {
    if ((h->Instance->CR1 & SPI_CR1_SPE) == 0) {
        __HAL_SPI_ENABLE(h);
    }
}

ST7789::ST7789(SPI_HandleTypeDef* hspi,
               GPIO_TypeDef* cs_port,  uint16_t cs_pin,
               GPIO_TypeDef* dc_port,  uint16_t dc_pin,
               GPIO_TypeDef* bl_port,  uint16_t bl_pin,
               GPIO_TypeDef* rst_port, uint16_t rst_pin)
    : hspi_(hspi),
      cs_port_(cs_port), cs_pin_(cs_pin),
      dc_port_(dc_port), dc_pin_(dc_pin),
      bl_port_(bl_port), bl_pin_(bl_pin),
      rst_port_(rst_port), rst_pin_(rst_pin) {}

void ST7789::write_cmd(uint8_t cmd) {
    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_, &cmd, 1, 100);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void ST7789::write_data(uint8_t data) {
    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);
    HAL_SPI_Transmit(hspi_, &data, 1, 100);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void ST7789::write_data_buf(const uint8_t* buf, uint32_t len) {
    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);
    HAL_SPI_Transmit(hspi_, const_cast<uint8_t*>(buf), len, 2000);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void ST7789::software_reset() {
    write_cmd(0x01);      // SWRESET
    HAL_Delay(150);       // 推荐 >=120ms
}

void ST7789::set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // CASET
    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    uint8_t cmd = 0x2A;
    HAL_SPI_Transmit(hspi_, &cmd, 1, 50);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);
    uint8_t col[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };
    HAL_SPI_Transmit(hspi_, col, 4, 50);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);

    // RASET
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    cmd = 0x2B;
    HAL_SPI_Transmit(hspi_, &cmd, 1, 50);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);
    uint8_t row[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };
    HAL_SPI_Transmit(hspi_, row, 4, 50);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void spi_dump_state(SPI_HandleTypeDef* h) {
    printf("CR1=0x%08lX SR=0x%08lX ERR=0x%lX\n",
        h->Instance->CR1, h->Instance->SR, h->ErrorCode);
}

void ST7789::fill_screen(uint16_t color) {
    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);

    // RAMWR
    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    uint8_t cmd = 0x2C;
    HAL_SPI_Transmit(hspi_, &cmd, 1, 50);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);

    // 一行缓冲（栈大小可接受则保留；不够就改成更小块）
    static uint8_t lineBuf[240 * 2];
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    for (uint32_t i = 0; i < sizeof(lineBuf); i += 2) {
        lineBuf[i] = hi;
        lineBuf[i + 1] = lo;
    }
    for (uint16_t y = 0; y < TFT_H; ++y) {
        HAL_SPI_Transmit(hspi_, lineBuf, sizeof(lineBuf), 1000);
    }
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void ST7789::draw_single_test_pixel() {
    uint16_t x = TFT_W / 2;
    uint16_t y = TFT_H / 2;
    set_addr_window(x, y, x, y);

    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    uint8_t cmd = 0x2C;
    HAL_SPI_Transmit(hspi_, &cmd, 1, 50);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);

    uint16_t pixel = 0xF81F; // 洋红色更醒目
    uint8_t p[2] = { (uint8_t)(pixel >> 8), (uint8_t)pixel };
    HAL_SPI_Transmit(hspi_, p, 2, 50);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
}

void ST7789::init_basic() {
    // 背光开
    HAL_GPIO_WritePin(bl_port_, bl_pin_, GPIO_PIN_SET);

    // 软件复位
    software_reset();

    // Sleep Out
    write_cmd(0x11);
    HAL_Delay(120);

    // COLMOD = 16bit
    write_cmd(0x3A);
    write_data(0x55);

    // MADCTL = 0x00
    write_cmd(0x36);
    write_data(0x00);

    // Display ON
    write_cmd(0x29);
    HAL_Delay(20);
}

