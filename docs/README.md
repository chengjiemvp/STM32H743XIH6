# STM32H743XIH6 项目文档

本目录包含项目的技术文档和详细说明。

## 文档列表

### [UART_CALLBACK_FLOW.md](UART_CALLBACK_FLOW.md)
**串口接收消息回调函数调用顺序详解**

详细解释了UART串口接收数据时的完整调用链，从硬件中断到用户回调函数的执行流程。

**内容包括**:
- 完整的9层调用流程图
- 每一层的详细说明和源代码位置
- 初始化顺序说明
- 两种数据访问方式（回调和轮询）
- 关键设计模式（单例、C/C++互操作、环形缓冲区）
- 中断安全性分析
- 性能特性和常见问题排查

**适用于**:
- 调试UART通信问题
- 理解嵌入式中断处理机制
- 学习C/C++混合编程
- 了解STM32 HAL库的工作原理

---

## 快速链接

- [主README](../README.md) - 项目整体说明（如果存在）
- [Core/Inc/Uart.hpp](../Core/Inc/Uart.hpp) - UART类定义（包含简要流程说明）
- [Core/Src/Uart.cpp](../Core/Src/Uart.cpp) - UART类实现
- [Core/Src/app_callbacks.cpp](../Core/Src/app_callbacks.cpp) - 回调函数桥接层
