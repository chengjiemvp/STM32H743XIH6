#pragma once
#include <cstdint>
#include "main.hpp"

namespace test {
    
    /// @brief  test sdram
    /// @param  void
    /// @return int 0: success
    /// @note   this function will test all 32mb sdram space
    int sdram_test(void);

    /// @brief run sdram test and flash led if success
    /// @note  this function will test all 32mb sdram space
    void run_sdram_test(void);
}