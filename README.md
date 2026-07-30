# LMX2572 PLL 射频源

本仓库包含一块基于 TI LMX2572 的 PLL/RF 频率源硬件原理图，以及配套
CH32V307VCT6 固件工程。

当前固件通过 SPI 配置 LMX2572，并使用 TICS Pro 导出的寄存器表，在
RFOUTA 输出固定 `45 MHz` 信号。

## 项目特性

- 使用 CH32V307VCT6 控制 TI LMX2572
- 基于 MounRiver Studio 的 RISC-V 工程
- 使用 SPI1 写入 LMX2572 寄存器
- `CSB` 使用普通 GPIO 软件控制
- `MUXout` 用作 Lock Detect 状态输入
- 当前默认启用 RFOUTA，关闭 RFOUTB
- 支持 USB-C 5 V 或 2 Pin 外部 5 V 供电
- 板上包含低噪声 3.3 V LDO，分别给数字/控制和 RF 电源域供电

## 硬件说明

主要器件和接口：

| 标号 | 器件/接口 | 说明 |
| --- | --- | --- |
| `U2` | `LMX2572RHAR` | PLL/RF synthesizer |
| `U1` | SMA | RFOUTA 输出 |
| `U3` | SMA | RFOUTB 输出 |
| `U6` | SMA | 外部参考时钟输入 |
| `U4` | TCXO 预留位 | 板载参考时钟，可选焊接 |
| `U5`, `U7` | `TPS7A2033` | 低噪声 3.3 V LDO |
| `USB1` | USB-C | 5 V 电源输入 |
| `CN1` | 2 Pin 接口 | 辅助 5 V 电源输入 |
| `H1` | 2.54 mm 排针 | LMX2572 控制接口 |
| `CN2` | MCU 控制接口 | SPI、MUX、CE、SYNC 等信号 |

注意：原理图中标注 USB-C 5 V 输入和 `CN1` 外部 5 V 输入没有做电源隔离，
不要同时接入两路电源。

## MCU 引脚连接

当前固件使用的 CH32V307 引脚如下：

| LMX2572 信号 | CH32V307 引脚 | 方向 | 用途 |
| --- | --- | --- | --- |
| `SDI` | `PA7 / SPI1_MOSI` | MCU 到 LMX2572 | SPI 数据 |
| `SCK` | `PA5 / SPI1_SCK` | MCU 到 LMX2572 | SPI 时钟 |
| `CSB` | `PA4 / GPIO` | MCU 到 LMX2572 | SPI 片选 |
| `MUXout` | `PB1 / GPIO 输入` | LMX2572 到 MCU | Lock Detect |
| `CE` | 硬件上拉 | 输入到 LMX2572 | 芯片使能 |

`SYNC`、`SYSREFREQ`、`RAMPCLK`、`RAMPDIR` 等信号在硬件上已经引出，
但当前固件暂未使用。

## RF 输出结构

LMX2572 的 RF 输出为差分结构，本板将其转换为单端 SMA 测试接口：

- RFOUTA：
  - `RFOUTAP` 通过匹配/隔直网络连接到 SMA
  - `RFOUTAM` 通过隔直电容后 50 ohm 到地端接
- RFOUTB：
  - `RFOUTBP` 通过匹配/隔直网络连接到 SMA
  - `RFOUTBM` 通过隔直电容后 50 ohm 到地端接

当前固件启用 RFOUTA，关闭 RFOUTB。

## 固件说明

工程信息：

| 项目 | 内容 |
| --- | --- |
| IDE | MounRiver Studio |
| MCU | CH32V307VCT6 |
| 工具链 | MounRiver Studio 自带 RISC-V Embedded GCC |
| 主函数 | `User/main.c` |
| LMX2572 驱动 | `User/lmx2572.c`, `User/lmx2572.h` |

关键源码文件：

```text
User/main.c
User/lmx2572.c
User/lmx2572.h
User/system_ch32v30x.c
User/ch32v30x_conf.h
Ld/Link.ld
Startup/startup_ch32v30x_D8C.S
```

当前启动流程：

1. 配置中断优先级分组。
2. 更新系统时钟。
3. 初始化延时函数。
4. 初始化 USART1，波特率 `115200`。
5. 初始化 SPI1 和 LMX2572 控制 GPIO。
6. 按 `R125` 到 `R0` 的顺序写入完整 LMX2572 寄存器表。
7. 每秒通过串口打印 RFOUTA 目标频率和 Lock Detect 状态。

## 当前 LMX2572 配置

当前固件中的寄存器表由 TICS Pro 导出，对应配置如下：

```text
参考输入: 25 MHz
Multiplier: 4
PFD 频率: 100 MHz
VCO 频率: 5760 MHz
Channel Divider: 128
RFOUTA: 45 MHz
RFOUTB: Power Down
```

计算关系：

```text
25 MHz * 4 = 100 MHz
57 + 3 / 5 = 57.6
100 MHz * 57.6 = 5760 MHz
5760 MHz / 128 = 45 MHz
```

关键寄存器：

```text
R75 = 0x4B0B00
R45 = 0x2DC60F
R44 = 0x2C0FA3
R43 = 0x2B0003
R39 = 0x270005
R37 = 0x250305
R36 = 0x240039
R0  = 0x00211C
```

当前固件在初始化时直接写入 TICS Pro 导出的完整寄存器表。驱动文件中保留了
动态设置频率的辅助函数，便于后续开发，但当前固定 45 MHz 版本不会在初始化
后再次调用动态改频函数，避免覆盖 TICS Pro 的精确配置。

## 编译和下载

1. 使用 MounRiver Studio 打开工程目录：

   ```text
   E:\MounRiver\PLL
   ```

2. 执行 `Clean Project`。
3. 执行 `Build Project`。
4. 下载固件到 CH32V307VCT6。
5. 打开串口，参数为：

   ```text
   115200 8N1
   ```

正常启动后应看到类似输出：

```text
LMX2572 45MHz mid-power build 2026-07-30
LMX2572 register write complete
RFOUTA:45 MHz, LockDetect:1
```

## 修改输出频率

推荐使用 TICS Pro 重新生成寄存器表，而不是手动改单个寄存器。

流程：

1. 在 TICS Pro 中配置目标频率。
2. 确认 VCO 工作在有效范围内。
3. 需要低频输出时，使用 Channel Divider 分频。
4. 导出 LMX2572 寄存器表。
5. 将 `User/lmx2572.c` 中的 `lmx2572_reg_values` 数组替换为新表。
6. 确认寄存器顺序为 `R125` 到 `R0`。
7. Clean、Build、Download。

例如输出 45 MHz 时，不应让 VCO 等于 45 MHz，而应使用：

```text
VCO = 5760 MHz
Channel Divider = 128
RFOUTA = 45 MHz
```

## 测试建议

频谱仪测试 45 MHz 时，建议使用窄扫：

```text
Center = 45 MHz
Span = 5 MHz 或 10 MHz
RBW = 10 kHz
Ref Level = -10 dBm 或 -20 dBm
Input = 50 ohm
```

如果锁定但测不到 45 MHz，可以同时检查：

```text
45 MHz
90 MHz
5760 MHz
```

这有助于判断输出是否真的走了 Channel Divider。

## 开源前建议

建议不要提交 MounRiver 生成的编译产物。可以在 `.gitignore` 中加入：

```text
obj/
.mrs/
*.elf
*.hex
*.map
*.lst
*.o
*.d
```

建议提交：

- 固件源码
- 原理图 PDF
- 关键寄存器导出表
- README
- 许可证文件

## 推荐仓库结构

```text
.
+-- Core/
+-- Debug/
+-- Ld/
+-- Peripheral/
+-- Startup/
+-- User/
+-- hardware/
|   +-- SCH_PLL_LMX2572_2026-07-30.pdf
+-- registers/
|   +-- 45Mhz.txt
+-- README.md
```

## License

开源前请根据用途选择许可证。

可选方案：

- 固件示例：MIT License
- 硬件设计：CERN-OHL-S 或 CERN-OHL-P

如果仓库中包含 WCH 官方外设库文件，请保留原文件头部的版权和许可说明。
