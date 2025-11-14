# STM32H743XIH6 项目文档

本目录包含项目的技术文档和详细说明。

## 📚 文档列表

### [UART_CALLBACK_FLOW.md](UART_CALLBACK_FLOW.md) ⭐
**串口接收消息回调函数调用顺序详解（完整版）**

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

### [UART_CALLBACK_DIAGRAM.txt](UART_CALLBACK_DIAGRAM.txt) 📊
**UART回调流程快速参考图（ASCII艺术）**

简洁的ASCII艺术流程图，适合快速查阅和打印。包含：
- 9层调用链可视化
- 初始化顺序清单
- 两种数据访问方式对比
- 中断安全设计要点

**使用场景**:
- 快速查阅调用顺序
- 在终端中直接查看
- 打印作为参考资料

---

## 🚀 快速开始

### 查看UART回调流程
```bash
# 快速查看流程图
cat docs/UART_CALLBACK_DIAGRAM.txt

# 阅读完整文档
cat docs/UART_CALLBACK_FLOW.md
```

### 代码中的注释
项目代码中也包含详细的内联注释，解释调用流程：
- [Core/Inc/Uart.hpp](../Core/Inc/Uart.hpp) - UART类定义（包含简要流程说明）
- [Core/Src/Uart.cpp](../Core/Src/Uart.cpp) - UART类实现（每个函数的详细注释）
- [Core/Src/app_callbacks.cpp](../Core/Src/app_callbacks.cpp) - 回调函数桥接层
- [Core/Src/stm32h7xx_it.c](../Core/Src/stm32h7xx_it.c) - 中断服务程序入口

---

## 🔗 相关链接

- 主README（项目根目录）
- [STM32H7 HAL用户手册](https://www.st.com/resource/en/user_manual/um2217-description-of-stm32h7-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [STM32H743 数据手册](https://www.st.com/resource/en/datasheet/stm32h743xi.pdf)
