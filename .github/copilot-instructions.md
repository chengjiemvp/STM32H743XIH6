# STM32H743XIH6 Project - AI Coding Agent Instructions

## Project Overview
This is an embedded systems project for STM32H743XIH6 microcontroller featuring:
- **LCD Display**: ST7789 TFT display with DMA-accelerated SPI transfers
- **UART Communication**: Singleton-based UART with interrupt-driven ring buffer
- **LED Control**: PC13 breathing light effect via software PWM
- **Memory**: 32MB SDRAM with explicit initialization sequence
- **Build System**: CMake with Ninja generator, targeting gcc-arm-none-eabi

## Critical Architecture Patterns

### 1. Singleton Pattern with Placement New (Memory Safety)
**Problem**: Embedded systems must avoid dynamic allocation in SDRAM during initialization.
**Solution**: Use placement new on static storage to allocate singletons in RAM/flash, not SDRAM.

**Examples**:
- `Uart::init()` in `Core/Inc/uart.hpp` uses `new (&instance_storage_) Uart(huart)` 
- `Led` instance in `Core/Src/main.cpp` uses `new (&led_storage) Led()`
- Static storage: `unsigned char instance_storage_[256]` or similar

**When adding new singletons**: Always use placement new with pre-allocated `static` storage arrays.

### 2. Interrupt Initialization Order (Critical)
**Order matters for system stability**. See `Core/Src/main.cpp`:
1. Initialize peripherals (GPIO, DMA, FMC for SDRAM, SPI, UART)
2. Initialize application objects (LED, LCD)
3. **LAST**: Register callbacks and start interrupts
   - Reason: Prevents ISRs from firing during object construction

**Key sequence in main()**:
```cpp
// 1. HAL peripherals
MX_GPIO_Init(); MX_DMA_Init(); MX_FMC_Init(); 
bsp::sdram::init_sequence(&hsdram1); // CRITICAL: after FMC
MX_USART1_UART_Init(); MX_TIM6_Init(); MX_TIM7_Init(); MX_SPI5_Init();

// 2. App objects
led_pc13_ptr = new (&led_storage) Led();
Uart::init(&huart1);
ST7789 lcd(...);

// 3. THEN start interrupts
Uart::get_instance().set_rx_callback(uart_rx_callback);
Uart::get_instance().begin();
HAL_TIM_Base_Start_IT(&htim6); HAL_TIM_Base_Start_IT(&htim7);
```

### 3. ISR Callback Bridges (C ↔ C++ Interop)
**Pattern**: `Core/Src/app_callbacks.cpp` contains HAL callbacks (`extern "C"`), which delegate to C++ objects.

```cpp
extern "C" {
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (Uart::is_initialized()) {  // Safety check!
            Uart::get_instance().isr_handler();
        }
    }
    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
        if (htim->Instance == TIM7 && led_pc13_ptr) {
            led_pc13_ptr->breathing();  // 1ms tick
        }
    }
    void HAL_DMA_TxCpltCallback(DMA_HandleTypeDef *hdma) {
        if (hdma->Instance == DMA1_Stream0 && g_lcd_ptr) {
            g_lcd_ptr->dma_tx_cplt_callback();
        }
    }
}
```

**When adding new ISRs**: Use global pointer registration pattern (see `register_lcd_instance()` in `main.cpp`).

### 4. Ring Buffer for Interrupt-Safe Data
`Core/Inc/RingBuffer.hpp`: Template circular buffer for thread-safe UART Rx.
- Head/tail are `volatile uint32_t` (modified by ISR)
- Used: `RingBuffer<uint8_t, UART_RX_BUFFER_SIZE>` in Uart class
- Methods: `push()`, `pop()`, `is_empty()`, `count()`

## Key Components

### UART (Core/Inc/uart.hpp, Core/Src/uart.cpp)
- **Singleton with lazy init**: `Uart::init()` must be called before `get_instance()`
- Interrupt-driven Rx into ring buffer
- Optional Rx callback for byte-level processing
- Example: `Uart::get_instance().begin()` starts interrupt, ISR fills buffer

### LED (Core/Inc/led.hpp, Core/Src/led.cpp)
- Software PWM breathing effect (called from TIM7 at 1ms intervals)
- PC13 pin (active low on this board)
- Brightness range: 0-100

### ST7789 LCD (Core/Inc/ST7789.hpp, Core/Src/ST7789.cpp)
- 240x320 TFT display over SPI5
- DMA-driven transfers (SPI5 TX uses DMA1_Stream0)
- Double buffering support
- `register_lcd_instance()` used to link ISR callback
- Entry point demo: `color_cycle_loop()` (infinite loop with color animation)

### SDRAM (Core/Src/bsp_sdram.cpp)
- Must initialize after FMC: `bsp::sdram::init_sequence(&hsdram1)`
- 32MB total space
- Test function: `test::run_sdram_test()` validates full range

## Build & Development

### CMake Presets
- **Debug**: `cmake --preset Debug && cmake --build --preset Debug`
- **Release**: `cmake --preset Release && cmake --build --preset Release`
- Toolchain: `cmake/gcc-arm-none-eabi.cmake`
- Output: `build/Debug/` or `build/Release/` directories

### Important Build Details
- C++ standard: C++23 (with `-std=c++23`)
- C standard: C11
- **Linker script**: `STM32H743XX_FLASH.ld`
- **STM32CubeMX**: Auto-generated sources in `cmake/stm32cubemx/`, manually added sources in `CMakeLists.txt`
- Startup: `startup_stm32h743xx.s` (ASM)

## C/C++ Interop Conventions

1. **Extern "C" blocks** wrap all HAL callbacks and C functions
2. **C headers** (.h files) are C++11-safe with `#ifdef __cplusplus` guards
3. **main.hpp** wraps `main.h` (C code) for C++ consumption
4. **Global pointers** for ISR access: `extern Led* led_pc13_ptr; extern ST7789* g_lcd_ptr;`

## Data Flow Example: UART Receive
1. ISR triggers on UART byte
2. `HAL_UART_RxCpltCallback()` → `Uart::isr_handler()`
3. `isr_handler()` pushes byte to `rx_buffer_` ring buffer
4. Optional callback fires: `rx_callback_(byte)` (prints to stdio in this app)
5. Main loop can poll `Uart::get_instance().available()` and `read()`

## Code Style & Testing
- **Namespaces**: Used in `bsp::sdram` and `test` for organization
- **Type safety**: `constexpr size_t` for buffer sizes (compile-time checks)
- **Doxygen comments**: `/// @brief`, `@param`, `@return`, `@note`
- **Test module**: `Core/Inc/test.hpp` has `test::sdram_test()`, `test::run_uart_test()`

## Common Tasks for AI Agents

### Adding a new peripheral driver
1. Create header in `Core/Inc/my_device.hpp` (C++)
2. Create implementation in `Core/Src/my_device.cpp`
3. Add to `CMakeLists.txt` source list
4. If ISR needed: add callback in `app_callbacks.cpp`, register via global pointer
5. Follow singleton pattern if stateful

### Debugging ISR issues
- Check ISR order in `main()` — must start after all init
- Verify global pointer registration: `register_lcd_instance()`
- Check safety guards: `if (obj_ptr != nullptr)` before calling
- Use `printf()` in ISR carefully (serialization risk with UART TX)

### LCD/Display tasks
- All drawing must happen before `color_cycle_loop()` (it never returns)
- Use DMA for bulk transfers; check `is_transmitting_` flag
- Coordinate SPI5 usage: LCD has exclusive access
