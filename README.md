# TestF103RE — MS31860T 驱动验证

基于 STM32F103RE + MS31860T（DRV8860 兼容）的 LED 驱动测试工程。

## 硬件平台

| 项目 | 说明 |
|------|------|
| MCU | STM32F103RET6 |
| 驱动芯片 | **MS31860T**（与 TI DRV8860 **Pin-to-Pin 兼容**） |
| 通信方式 | 软件 SPI（Daisy-Chain 级联） |
| 开发环境 | EIDE / Keil MDK-ARM + STM32CubeMX |

## 引脚连接

| 功能 | GPIO | 说明 |
|------|------|------|
| SPI_CLK | PA5 | 软件 SPI 时钟 |
| SPI_MOSI | PA7 | 主机输出 → 从机输入 |
| SPI_MISO | PA6 | 从机输出 → 主机输入（读回） |
| ENABLE | PB13 | 芯片使能，启动后保持稳定，勿在循环中翻转 |
| FAULT | PB12 | 故障检测输入，每周期读取并映射到 LED_ERR |

## Daisy-Chain 注意

级联时数据帧顺序为**反序**：`tx_data[chips-1]` 先到达链上第一颗芯片，编程时需注意。

## 已知问题

### MS31860T 关断漏电流导致 LED 微亮

- **现象**：芯片输出关闭后，LED 仍有微弱发光。
- **原因**：关闭状态下检测到约 **30 μA** 的漏电流流过 LED，足以让高亮 LED 在暗光环境中可见微亮。


## 目录结构

```
TestF103RE_MS31860T/
├── Core/
│   ├── Inc/          # 头文件 (main.h, stm32f1xx_hal_conf.h, stm32f1xx_it.h)
│   └── Src/          # 源文件 (main.c, stm32f1xx_hal_msp.c, stm32f1xx_it.c, system_stm32f1xx.c)
├── Drivers/
│   ├── CMSIS/        # Cortex-M3 核心外设驱动
│   └── STM32F1xx_HAL_Driver/  # HAL 库
├── MDK-ARM/          # Keil 工程文件
└── TestF103RE.ioc    # CubeMX 工程
```

## 构建 & 烧录

使用 EIDE 插件（VS Code）：

| 任务 | 说明 |
|------|------|
| **build** | 编译工程 |
| **rebuild** | 全量重新编译 |
| **flash** | 烧录到目标板 |
| **build and flash** | 编译 + 烧录 |
| **clean** | 清理构建产物 |
