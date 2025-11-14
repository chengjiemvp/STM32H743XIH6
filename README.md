# STM32H743XIH6 嵌入式项目

基于STM32H743XIH6微控制器的嵌入式系统项目，采用C++23和HAL库开发。

## 🎯 项目特性

- **LCD显示**: ST7789 TFT显示屏，SPI接口，DMA加速传输
- **UART通信**: 基于单例模式的UART类，中断驱动环形缓冲区
- **LED控制**: PC13呼吸灯效果，软件PWM实现
- **内存管理**: 32MB SDRAM，带初始化序列
- **构建系统**: CMake + Ninja，gcc-arm-none-eabi工具链

## 📖 文档

### UART串口通信详细说明

项目包含了详细的UART回调函数调用顺序文档，解释从硬件中断到用户回调的完整流程：

- **[docs/UART_CALLBACK_FLOW.md](docs/UART_CALLBACK_FLOW.md)** - 完整的技术文档（中文，7700+字）
- **[docs/UART_CALLBACK_DIAGRAM.txt](docs/UART_CALLBACK_DIAGRAM.txt)** - 快速参考流程图（ASCII艺术）
- **[docs/README.md](docs/README.md)** - 文档索引

### 代码中的注释

关键代码文件包含详细的中文和英文注释：
- `Core/Inc/Uart.hpp` - UART类定义和使用说明
- `Core/Src/Uart.cpp` - UART实现和回调处理
- `Core/Src/app_callbacks.cpp` - C/C++桥接层
- `Core/Src/main.cpp` - 初始化顺序说明

## 🏗️ 构建项目

### 前置要求

- CMake 3.22+
- Ninja构建工具
- gcc-arm-none-eabi工具链
- STM32CubeMX（用于HAL库代码生成）

### 编译步骤

```bash
# Debug构建
cmake --preset Debug
cmake --build --preset Debug

# Release构建
cmake --preset Release
cmake --build --preset Release
```

构建输出位于 `build/Debug/` 或 `build/Release/` 目录。

## 🔌 硬件配置

- **MCU**: STM32H743XIH6 (480MHz, Cortex-M7)
- **SDRAM**: 32MB (FMC接口)
- **LCD**: ST7789 240x320 TFT (SPI5)
- **UART**: USART1 (115200 baud)
- **LED**: PC13 (呼吸灯)

## 🎨 架构特点

### 单例模式与Placement New
使用placement new在静态存储上分配单例对象，避免在SDRAM上动态分配：

```cpp
static unsigned char instance_storage_[256];
instance_ = new (&instance_storage_) Uart(huart);
```

### C/C++混合编程
使用`extern "C"`实现C语言HAL库与C++对象的无缝桥接：

```cpp
extern "C" {
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
        if (Uart::is_initialized()) {
            Uart::get_instance().isr_handler();
        }
    }
}
```

### 中断安全设计
- 环形缓冲区使用`volatile`变量
- 初始化顺序保护（先init后begin）
- 单生产者单消费者无锁设计

## 📊 UART数据流

UART支持两种数据访问方式：

### 1. 回调方式（实时处理）
```cpp
void uart_rx_callback(uint8_t byte) {
    printf("%c", byte);  // ISR上下文，保持简短
}

Uart::get_instance().set_rx_callback(uart_rx_callback);
```

### 2. 轮询方式（延迟处理）
```cpp
while (Uart::get_instance().available()) {
    int byte = Uart::get_instance().read();
    // 在主循环中慢慢处理
}
```

## 🔧 开发工具

- **IDE**: VSCode（推荐）或任何支持CMake的IDE
- **调试器**: ST-Link + OpenOCD / STM32CubeIDE
- **代码标准**: C++23, C11

## 📝 许可证

请参考STMicroelectronics HAL库的许可证条款。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📧 联系方式

如有问题，请在GitHub上提交Issue。

---

**注意**: 这是一个嵌入式项目，需要STM32H743开发板才能运行。详细的UART通信机制说明请参见 [docs/UART_CALLBACK_FLOW.md](docs/UART_CALLBACK_FLOW.md)。
