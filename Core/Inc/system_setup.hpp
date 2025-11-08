#pragma once
#include "main.h" // for HAL definitions

extern "C" {

    // 声明所有系统配置相关的函数
    void SystemClock_Config(void);
    void MPU_Config(void);
    void Error_Handler(void);

    // assert_failed 通常在 main.h 中通过宏定义，但如果不在，也应在这里声明
    #ifdef USE_FULL_ASSERT
    void assert_failed(uint8_t *file, uint32_t line);
    #endif

}