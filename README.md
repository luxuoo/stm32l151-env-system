# STM32 Sensor Monitor

基于 STM32L151 的环境传感器监测系统，驱动 SSD1306 OLED 显示屏实时展示温度、湿度、光照数据。

## 硬件平台

| 组件 | 型号 | 说明 |
|------|------|------|
| MCU | STM32L151xB | Cortex-M3, 32MHz, 64KB Flash, 10KB RAM |
| OLED | SSD1306 | 128x64, SPI 四线接口 (PB12-PB15) |
| 温湿度 | AHT20 | I2C 地址 0x38, 精度 +/-0.3°C / +/-2%RH |
| 光照 | BH1750 | I2C 地址 0x23, 量程 1-65535 Lux |
| I2C总线 | I2C1 | PB6(SCL) / PB7(SDA), 100kHz |

## 项目结构

```
STM32HL/
├── Core/
│   ├── Inc/
│   │   ├── oled.h          # OLED 驱动头文件
│   │   ├── bh1750.h        # BH1750 光照传感器
│   │   ├── aht20.h         # AHT20 温湿度传感器
│   │   ├── main.h
│   │   ├── i2c.h
│   │   └── gpio.h
│   └── Src/
│       ├── oled.c          # OLED 驱动 (SPI, 像素级绘图, 缓冲区)
│       ├── bh1750.c        # BH1750 驱动
│       ├── aht20.c         # AHT20 驱动 (含 CRC 校验)
│       ├── main.c          # 主程序 (开机动画, 传感器轮询, UI)
│       ├── i2c.c
│       └── gpio.c
├── Drivers/                 # STM32 HAL 库
├── cmake/                   # CMake 构建配置
├── STM32HL.ioc              # STM32CubeMX 工程
└── CMakeLists.txt
```

## 软件架构

### OLED 驱动 (`oled.c` / `oled.h`)

- **接口**: SPI 四线 (CS, DC, SCK, SDA)
- **缓冲区**: 128x64 显存, 脏标志按需刷新, 消除闪烁
- **字体**: 内置 6x8 和 8x16 两套 ASCII 字库
- **绘图 API**:
  - `OLED_SetPixel` / `OLED_ClearPixel` — 像素级操作
  - `OLED_DrawString` / `OLED_Printf` — 字符串与格式化输出
  - `OLED_DrawHLine` / `OLED_DrawVLine` / `OLED_DrawRect` — 线条与矩形
  - `OLED_FillRect` / `OLED_DrawProgressBar` — 填充与进度条
  - `OLED_InvertPageRect` — 区域反白 (用于报警闪烁)

### BH1750 驱动 (`bh1750.c`)

```c
BH1750_Init(&dev, &hi2c1, BH1750_ADDR);  // 上电 -> 复位 -> 连续高分辨率
BH1750_ReadLight(&dev, &lux);             // 返回 0=成功, lux 单位 Lux
```

### AHT20 驱动 (`aht20.c`)

```c
AHT20_Init(&dev, &hi2c1);                 // 软复位 -> 检查校准位 -> 初始化
AHT20_Read(&dev, &temp, &humi);           // 返回 0=成功, 4=CRC 不对但数据可用
```

- 上电后先发软复位 (0xBA), 再检查校准位 bit3
- 测量后轮询忙位, 最多等 100ms
- CRC 校验失败返回 4, 但仍解析数据 (兼容部分劣质模块)

### 主程序 (`main.c`)

1. **I2C 总线复位**: 初始化前手动拨 9 个时钟脉冲, 解锁可能卡死的从机
2. **开机动画**: 标题逐字显现 + 下划线展开 + 进度条 + 水平擦除过渡
3. **传感器轮询**: 每 1.5s 读取, 实时探测 I2C 设备是否在线
4. **显示刷新**: 每 200ms 更新, 含温度/湿度进度条、光照值、报警闪烁
5. **I2C 容错**: 检测到总线错误自动 DeInit -> BusReset -> ReInit
6. **强光报警**: 光照 >= 1000 Lux 时文字反白闪烁

## 构建

本项目使用 CMake + ARM GCC 工具链, 可通过 VS Code 的 STM32 扩展或命令行构建:

```bash
# VS Code: Ctrl+Shift+P -> CMake: Delete Cache and Reconfigure -> Build

# 命令行 (需安装 STM32CubeCLT)
cmake -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```

生成文件: `build/Debug/STM32HL.elf`

## 接线参考

```
STM32L151          BH1750         AHT20         OLED (SPI)
─────────          ──────         ─────         ──────────
PB6 (SCL)  ─────── SCL ────────── SCL
PB7 (SDA)  ─────── SDA ────────── SDA
PB12 ──────────────────────────────────────── CS
PB13 ──────────────────────────────────────── SCK (D0)
PB14 ──────────────────────────────────────── DC
PB15 ──────────────────────────────────────── SDA (D1)
3.3V ──────────── VCC ────────── VCC ──────── VCC
GND  ──────────── GND ────────── GND ──────── GND
```

注意: I2C 总线 (SCL/SDA) 需要外接 4.7k 上拉电阻到 3.3V。
