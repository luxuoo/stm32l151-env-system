# STM32L151 环境监测与补光控制系统

STM32L151 平台的传感器系统集合，由原 `SHIXUNSTM32`（补光控制）与 `STM32hl`（环境监测）两个仓库合并而来，保留各自提交历史。

## 固件变体

| 目录 | 来源仓库 | 功能 | 传感器 |
|------|----------|------|--------|
| `firmware/complement-light/` | SHIXUNSTM32 | 自动补光控制 | BH1750 光照 + OLED |
| `firmware/env-monitor/` | STM32hl（main 主干） | 环境监测显示 | AHT20 温湿度 + BH1750 光照 + OLED |

## 队友分支归档

`archive/stm32hl-team-branches/` 保存了原 STM32hl 仓库中未合入 main 的队友开发分支完整快照（含提交后代码，分支间存在代码差异，属独立实验线，未自动合并以免冲突）：

- `master/` — 初始工程（106 文件）
- `志新分支/` — 开屏动画改版 + 卡片式数据布局（103 文件）
- `板载两口插/` — 传感器接 I2C1 + OLED 开屏动画 + 显示编码修复（103 文件）

如需将某个分支的改动正式合入 `firmware/env-monitor/`，需人工核对代码差异后合并。

## 相关仓库

- 显示终端：`esp32s3-env-monitor`（ESP32-S3 TFT 屏）
- 全栈架构：见 [`greenhouse-system`](https://github.com/luxuoo/greenhouse-system)

## License

MIT。
