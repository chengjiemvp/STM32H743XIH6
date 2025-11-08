#include <memory>
#include "main.hpp"
#include "test.hpp"
#include "led.hpp"
#include "inttypes.h" // for HAL_RCC_GetSysClockFreq print, UNSIGNED LONG


int test::sdram_test(void) {

    uint32_t *pSDRAM = (uint32_t *)0xC0000000; // SDRAM 起始地址
    const uint32_t SDRAM_SIZE_WORDS = (32 * 1024 * 1024) / 4; // 32MB SDRAM，转换为 32位字 的数量
    uint32_t i = 0;

    // --- 模式 1: 写入并校验 0xAAAAAAAA ---
    // 这个模式可以检查数据位是否卡在 0
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        pSDRAM[i] = 0xAAAAAAAA;
    }
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        if (pSDRAM[i] != 0xAAAAAAAA) {
            return 1; // 失败
        }
    }

    // --- 模式 2: 写入并校验 0x55555555 ---
    // 这个模式可以检查数据位是否卡在 1
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        pSDRAM[i] = 0x55555555;
    }
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        if (pSDRAM[i] != 0x55555555) {
        return 2; // 失败
        }
    }

    // --- 模式 3: 将地址作为值写入并校验 ---
    // 这个模式非常有效，可以检查地址线和数据线是否有短路或断路
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        pSDRAM[i] = (uint32_t)&pSDRAM[i];
    }
    for (i = 0; i < SDRAM_SIZE_WORDS; i++) {
        if (pSDRAM[i] != (uint32_t)&pSDRAM[i]) {
            return 3; // 失败
        }
    }

    return 0; // 所有测试通过
}

void test::run_sdram_test(void) {
    int result = test::sdram_test();
    std::unique_ptr<Led> led_pc13 = std::make_unique<Led>();
    if (result == 0) {
        while(1) {
            led_pc13->flash_irregular();
        }
    }
}

void test::run_uart_test(void) {
    printf("\n\n--- System Initialized ---\n");
    printf("SYSCLK is running at %" PRIu32 " Hz\n", HAL_RCC_GetSysClockFreq());
    printf("HCLK is running at %" PRIu32 " Hz\n", HAL_RCC_GetHCLKFreq());
    printf("SDRAM heap is ready.\n");
    return;
}