#include "ST7789.hpp"
#include "main.hpp"
#include <stdio.h>

#define TFT_W 240
#define TFT_H 280

ST7789::ST7789(SPI_HandleTypeDef* hspi) : hspi_(hspi) {}

void ST7789::init() {
   
    HAL_Delay(10);               	// 屏幕刚完成复位时（包括上电复位），需要等待至少5ms才能发送指令
 	write_cmd(0x36);       // 显存访问控制 指令，用于设置访问显存的方式
	write_data(0x00);     // 配置成 从上到下、从左到右，RGB像素格式

	write_cmd(0x3A);			// 接口像素格式 指令，用于设置使用 12位、16位还是18位色
	write_data(0x05);     // 此处配置成 16位 像素格式

    // 接下来很多都是电压设置指令，直接使用厂家给设定值
 	write_cmd(0xB2);			
	write_data(0x0C);
	write_data(0x0C); 
	write_data(0x00); 
	write_data(0x33); 
	write_data(0x33); 			

	write_cmd(0xB7);		   // 栅极电压设置指令	
	write_data(0x35);     // VGH = 13.26V，VGL = -10.43V

	write_cmd(0xBB);			// 公共电压设置指令
	write_data(0x19);     // VCOM = 1.35V

	write_cmd(0xC0);
	write_data(0x2C);

	write_cmd(0xC2);       // VDV 和 VRH 来源设置
	write_data(0x01);     // VDV 和 VRH 由用户自由配置

	write_cmd(0xC3);			// VRH电压 设置指令  
	write_data(0x12);     // VRH电压 = 4.6+( vcom+vcom offset+vdv)
				
	write_cmd(0xC4);		   // VDV电压 设置指令	
	write_data(0x20);     // VDV电压 = 0v

	write_cmd(0xC6); 		// 正常模式的帧率控制指令
	write_data(0x0F);   	// 设置屏幕控制器的刷新帧率为60帧    

	write_cmd(0xD0);			// 电源控制指令
	write_data(0xA4);     // 无效数据，固定写入0xA4
	write_data(0xA1);     // AVDD = 6.8V ，AVDD = -4.8V ，VDS = 2.3V

	write_cmd(0xE0);       // 正极电压伽马值设定
	write_data(0xD0);
	write_data(0x04);
	write_data(0x0D);
	write_data(0x11);
	write_data(0x13);
	write_data(0x2B);
	write_data(0x3F);
	write_data(0x54);
	write_data(0x4C);
	write_data(0x18);
	write_data(0x0D);
	write_data(0x0B);
	write_data(0x1F);
	write_data(0x23);

	write_cmd(0xE1);      // 负极电压伽马值设定
	write_data(0xD0);
	write_data(0x04);
	write_data(0x0C);
	write_data(0x11);
	write_data(0x13);
	write_data(0x2C);
	write_data(0x3F);
	write_data(0x44);
	write_data(0x51);
	write_data(0x2F);
	write_data(0x1F);
	write_data(0x1F);
	write_data(0x20);
	write_data(0x23);
	write_cmd(0x21);       // 打开反显，因为面板是常黑型，操作需要反过来

    // 退出休眠指令，LCD控制器在刚上电、复位时，会自动进入休眠模式 ，因此操作屏幕之前，需要退出休眠  
	write_cmd(0x11);       // 退出休眠 指令
    HAL_Delay(120);        // 需要等待120ms，让电源电压和时钟电路稳定下来

    // 打开显示指令，LCD控制器在刚上电、复位时，会自动关闭显示 
	write_cmd(0x29);       // 打开显示   	
	
    // 以下进行一些驱动的默认设置
    LCD_SetDirection(Direction_V);  	      //	设置显示方向
	LCD_SetBackColor(LCD_BLACK);           // 设置背景色
 	LCD_SetColor(LCD_WHITE);               // 设置画笔色  
	LCD_Clear();                           // 清屏

    LCD_SetAsciiFont(&ASCII_Font24);       // 设置默认字体
    LCD_ShowNumMode(Fill_Zero);	      	// 设置变量显示模式，多余位填充空格还是填充0
}

void ST7789::write_cmd(unit8_t cmd) {
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi_, &cmd, 1, 1000);
}

void ST7789::write_data(uint8_t data) {
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi_, &data, 1, 1000); // 启动SPI传输
}

void set_addr_window() {
    LCD_WriteCommand(0x36);    		// 显存访问控制 指令，用于设置访问显存的方式
    LCD_WriteData_8bit(0x00);        // 垂直显示
    LCD.X_Offset   = 0;              // 设置控制器坐标偏移量
    LCD.Y_Offset   = 20;     
    LCD.Width      = LCD_Width;		// 重新赋值长、宽
    LCD.Height     = LCD_Height;		
}