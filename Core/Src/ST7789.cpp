#include "ST7789.hpp"
#include "main.hpp"
#include <stdio.h>

#define TFT_W 240
#define TFT_H 280

volatile bool spi_dma_busy = false;

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
    if (spi_dma_busy) return; // 上一次DMA未完成，直接返回或等待

    set_addr_window(0, 0, TFT_W - 1, TFT_H - 1);

    ensure_spi_enabled(hspi_);
    HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_RESET);
    uint8_t cmd = 0x2C;
    HAL_SPI_Transmit(hspi_, &cmd, 1, 50);
    HAL_GPIO_WritePin(dc_port_, dc_pin_, GPIO_PIN_SET);

    static uint8_t* fullBuf = nullptr;
    static size_t bufSize = TFT_W * TFT_H * 2;
    if (!fullBuf) {
        fullBuf = (uint8_t*)malloc(bufSize);
    }
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    for (size_t i = 0; i < bufSize; i += 2) {
        fullBuf[i] = hi;
        fullBuf[i + 1] = lo;
    }

    spi_dma_busy = true;
    HAL_StatusTypeDef ret = HAL_SPI_Transmit_DMA(hspi_, fullBuf, bufSize);
    if (ret != HAL_OK) {
        printf("SPI DMA TX fail, ret=%d\n", ret);
        spi_dma_busy = false;
        HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET);
    }
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
    HAL_Delay(150);

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

    ///////////////////////////////////////////////////////
    MX_SPI5_Init();               // 初始化SPI和控制引脚
   
    HAL_Delay(10);               	// 屏幕刚完成复位时（包括上电复位），需要等待至少5ms才能发送指令
 	LCD_WriteCommand(0x36);       // 显存访问控制 指令，用于设置访问显存的方式
	LCD_WriteData_8bit(0x00);     // 配置成 从上到下、从左到右，RGB像素格式

	LCD_WriteCommand(0x3A);			// 接口像素格式 指令，用于设置使用 12位、16位还是18位色
	LCD_WriteData_8bit(0x05);     // 此处配置成 16位 像素格式

    // 接下来很多都是电压设置指令，直接使用厂家给设定值
 	LCD_WriteCommand(0xB2);			
	LCD_WriteData_8bit(0x0C);
	LCD_WriteData_8bit(0x0C); 
	LCD_WriteData_8bit(0x00); 
	LCD_WriteData_8bit(0x33); 
	LCD_WriteData_8bit(0x33); 			

	LCD_WriteCommand(0xB7);		   // 栅极电压设置指令	
	LCD_WriteData_8bit(0x35);     // VGH = 13.26V，VGL = -10.43V

	LCD_WriteCommand(0xBB);			// 公共电压设置指令
	LCD_WriteData_8bit(0x19);     // VCOM = 1.35V

	LCD_WriteCommand(0xC0);
	LCD_WriteData_8bit(0x2C);

	LCD_WriteCommand(0xC2);       // VDV 和 VRH 来源设置
	LCD_WriteData_8bit(0x01);     // VDV 和 VRH 由用户自由配置

	LCD_WriteCommand(0xC3);			// VRH电压 设置指令  
	LCD_WriteData_8bit(0x12);     // VRH电压 = 4.6+( vcom+vcom offset+vdv)
				
	LCD_WriteCommand(0xC4);		   // VDV电压 设置指令	
	LCD_WriteData_8bit(0x20);     // VDV电压 = 0v

	LCD_WriteCommand(0xC6); 		// 正常模式的帧率控制指令
	LCD_WriteData_8bit(0x0F);   	// 设置屏幕控制器的刷新帧率为60帧    

	LCD_WriteCommand(0xD0);			// 电源控制指令
	LCD_WriteData_8bit(0xA4);     // 无效数据，固定写入0xA4
	LCD_WriteData_8bit(0xA1);     // AVDD = 6.8V ，AVDD = -4.8V ，VDS = 2.3V

	LCD_WriteCommand(0xE0);       // 正极电压伽马值设定
	LCD_WriteData_8bit(0xD0);
	LCD_WriteData_8bit(0x04);
	LCD_WriteData_8bit(0x0D);
	LCD_WriteData_8bit(0x11);
	LCD_WriteData_8bit(0x13);
	LCD_WriteData_8bit(0x2B);
	LCD_WriteData_8bit(0x3F);
	LCD_WriteData_8bit(0x54);
	LCD_WriteData_8bit(0x4C);
	LCD_WriteData_8bit(0x18);
	LCD_WriteData_8bit(0x0D);
	LCD_WriteData_8bit(0x0B);
	LCD_WriteData_8bit(0x1F);
	LCD_WriteData_8bit(0x23);

	LCD_WriteCommand(0xE1);      // 负极电压伽马值设定
	LCD_WriteData_8bit(0xD0);
	LCD_WriteData_8bit(0x04);
	LCD_WriteData_8bit(0x0C);
	LCD_WriteData_8bit(0x11);
	LCD_WriteData_8bit(0x13);
	LCD_WriteData_8bit(0x2C);
	LCD_WriteData_8bit(0x3F);
	LCD_WriteData_8bit(0x44);
	LCD_WriteData_8bit(0x51);
	LCD_WriteData_8bit(0x2F);
	LCD_WriteData_8bit(0x1F);
	LCD_WriteData_8bit(0x1F);
	LCD_WriteData_8bit(0x20);
	LCD_WriteData_8bit(0x23);
	LCD_WriteCommand(0x21);       // 打开反显，因为面板是常黑型，操作需要反过来

    // 退出休眠指令，LCD控制器在刚上电、复位时，会自动进入休眠模式 ，因此操作屏幕之前，需要退出休眠  
	LCD_WriteCommand(0x11);       // 退出休眠 指令
    HAL_Delay(120);               // 需要等待120ms，让电源电压和时钟电路稳定下来

    // 打开显示指令，LCD控制器在刚上电、复位时，会自动关闭显示 
	LCD_WriteCommand(0x29);       // 打开显示   	
	
    // 以下进行一些驱动的默认设置
    LCD_SetDirection(Direction_V);  	      //	设置显示方向
	LCD_SetBackColor(LCD_BLACK);           // 设置背景色
 	LCD_SetColor(LCD_WHITE);               // 设置画笔色  
	LCD_Clear();                           // 清屏

    LCD_SetAsciiFont(&ASCII_Font24);       // 设置默认字体
    LCD_ShowNumMode(Fill_Zero);	      	// 设置变量显示模式，多余位填充空格还是填充0
}

