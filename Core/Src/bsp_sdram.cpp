#include "bsp_sdram.hpp"

namespace bsp {
    namespace sdram {

        HAL_StatusTypeDef init_sequence(SDRAM_HandleTypeDef *hsdram) {

            FMC_SDRAM_CommandTypeDef command;
            uint32_t mode_reg = 0;

            // 步骤 1: Clock Configuration Enable
            command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
            command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
            command.AutoRefreshNumber      = 1;
            command.ModeRegisterDefinition = 0;
            if (HAL_SDRAM_SendCommand(hsdram, &command, 0x1000) != HAL_OK) {
                return HAL_ERROR;
            }
            HAL_Delay(1);

            // 步骤 2: PALL (Precharge All)
            command.CommandMode            = FMC_SDRAM_CMD_PALL;
            command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
            command.AutoRefreshNumber      = 1;
            command.ModeRegisterDefinition = 0;
            if (HAL_SDRAM_SendCommand(hsdram, &command, 0x1000) != HAL_OK) {
                return HAL_ERROR;
            }

            // 步骤 3: Auto-Refresh
            command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
            command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
            command.AutoRefreshNumber      = 8;
            command.ModeRegisterDefinition = 0;
            if (HAL_SDRAM_SendCommand(hsdram, &command, 0x1000) != HAL_OK) {
                return HAL_ERROR;
            }

            // 步骤 4: Load Mode Register (120MHz, CL3)
            mode_reg = 0x230;
            command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
            command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
            command.AutoRefreshNumber      = 1;
            command.ModeRegisterDefinition = mode_reg;
            if (HAL_SDRAM_SendCommand(hsdram, &command, 0x1000) != HAL_OK) {
                return HAL_ERROR;
            }

            // 步骤 5: 设置刷新率计数器 (120MHz)
            if (HAL_SDRAM_ProgramRefreshRate(hsdram, 938) != HAL_OK) {
                return HAL_ERROR;
            }

            return HAL_OK;
        }
    } // namespace sdram
} // namespace bsp