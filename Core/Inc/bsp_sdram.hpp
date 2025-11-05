#pragma once
#include "main.hpp"

namespace bsp {
namespace sdram {

/**
 * @brief  执行 SDRAM 初始化命令序列
 * @param  hsdram: 指向 SDRAM 句柄的指针
 * @retval HAL_StatusTypeDef HAL_OK 表示成功
 */
HAL_StatusTypeDef init_sequence(SDRAM_HandleTypeDef *hsdram);

} // namespace sdram
} // namespace bsp