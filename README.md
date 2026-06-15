# BH1750 智能补光控制器

基于 STM32L151C8Tx 的智能光照检测与自动补光系统，具备 OLED 菜单交互、串口日志输出、掉线自恢复和低功耗休眠功能。

## 系统概述

系统以 STM32 为核心，通过 I2C2 总线连接 BH1750 光照传感器采集环境光照度，OLED 屏幕显示菜单和实时数据，三个按键完成人机交互，一颗 LED 充当补光灯，并通过 USART1+DMA 向串口输出系统状态日志。

## 硬件连接

| 功能 | 引脚 | 说明 |
|------|------|------|
| BH1750 SCL | PB10 | I2C2 时钟线 |
| BH1750 SDA | PB11 | I2C2 数据线 |
| OLED CS | PB12 | 软件 SPI 片选 |
| OLED SCLK | PB13 | 软件 SPI 时钟 |
| OLED DC | PB14 | 数据/命令选择 |
| OLED SDIN | PB15 | 软件 SPI 数据 |
| LED 补光灯 | PA3 | TIM2_CH4 PWM 输出，低电平点亮 |
| 按键-上移 | PC13 | EXTI 下降沿触发 |
| 按键-下移/返回 | PB1 | EXTI 下降沿触发 |
| 按键-确认 | PB5 | EXTI 下降沿触发 |
| 串口 TX | PA9 | USART1 115200baud |
| 串口 RX | PA10 | USART1 |

## 软件架构

采用前后台多任务架构，中断只设置标志位，阻塞操作全部在主循环中执行。

```
┌──────────────┐    标志位     ┌───────────────┐
│  TIM7 1s     │──────────→  │  读取 BH1750   │
│  TIM6 10ms   │──────────→  │  调光算法       │→ TIM2 PWM → LED
│  EXTI 按键   │──────────→  │  菜单/状态机    │→ OLED 刷新
│  DMA UART    │──────────→  │  串口日志       │
└──────────────┘             └───────────────┘
```

## 功能特性

### 两级菜单交互

- **一级菜单**：标题「BH1750 Reader」，两个选项 SYS START / SYS OFF，光标用 `->` 标记
- **二级菜单**：实时显示当前光照度（Lux）和补光灯状态（ON/OFF）

### 自动补光控制

- 300 Lux 为全暗阈值，光照越暗 LED 越亮
- 一阶滞后滤波（α=0.05）实现呼吸灯式平滑渐变，避免光线突变刺眼
- 光照 ≥ 300 Lux 时 LED 自动熄灭

### 状态机调度

| 状态 | 触发条件 | 行为 |
|------|---------|------|
| IDLE | 上电 / 返回 | 显示菜单，等待操作 |
| RUNNING | 选择 SYS START | 开启传感器、定时器、PWM，实时测光补光 |
| ERROR | 传感器掉线 | 显示 NO DATA，LED 关闭，触发 I2C 总线恢复 |
| OFF | 选择 SYS OFF / 返回键 | 传感器断电、OLED 灭屏、定时器停止，低功耗休眠 |

### 掉线自恢复

传感器断开时自动执行 I2C 总线恢复（拉低 SCL 9 个时钟释放总线），重新接好后自动恢复正常测量与补光。

### 串口日志

通过 USART1+DMA 以 115200 波特率输出系统事件：

```
SYSTEM_START           — 系统启动
SYSTEM_OFF             — 系统关闭
SENSOR_DISCONNECT      — 传感器断开
```

## 按键功能

| 按键 | 菜单界面 | 运行界面 | 休眠状态 |
|------|---------|---------|---------|
| PC13 ↑ | 光标上移 | — | — |
| PB1 ↓ | 光标下移 | 返回菜单 | — |
| PB5 OK | 确认选择 | 返回菜单 | 唤醒 |

## 外设配置

| 外设 | 配置 |
|------|------|
| I2C2 | 100kHz，7-bit 地址 0x23（HAL 地址 0x46） |
| TIM2 CH4 | PWM 1kHz，PA3 低电平有效 |
| TIM6 | 10ms 定时中断，驱动调光算法 |
| TIM7 | 1s 定时中断，驱动传感器读取 |
| USART1 | 115200 8N1，DMA 发送 |
| OLED | 128x64 SSD1306，软件 SPI |

## 项目结构

```
Core/
  Inc/
    app.h              — 应用层头文件（状态机、全局变量）
    bh1750.h           — BH1750 驱动头文件
    tim.h              — 定时器头文件
    XMF_OLED_STM32Cube.h — OLED 驱动头文件
  Src/
    main.c             — 系统初始化与主循环
    app.c              — 应用逻辑（菜单、调光、按键、I2C 恢复）
    bh1750.c           — BH1750 I2C 驱动
    tim.c              — TIM2/6/7 初始化（CubeMX 生成）
    usart.c            — USART1+DMA 初始化（CubeMX 生成）
    i2c.c              — I2C2 初始化（CubeMX 生成）
    gpio.c             — GPIO 初始化（CubeMX 生成）
    dma.c              — DMA 初始化（CubeMX 生成）
    stm32l1xx_it.c     — 中断服务函数
    stm32l1xx_hal_msp.c — HAL 底层初始化（按键 EXTI）
    XMF_OLED_STM32Cube.c — OLED 驱动实现
Drivers/
    CMSIS/             — ARM CMSIS 头文件
    STM32L1xx_HAL_Driver/ — ST HAL 驱动库
```

## 构建工具

- STM32CubeMX — 外设配置与代码生成
- CMake + Ninja — 构建系统
- GNU ARM Toolchain — 交叉编译器
