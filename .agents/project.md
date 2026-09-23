# Voice Checkpoint 工程状态

> 更新时间：2026-09-23
>
> 当前阶段：基础驱动、音频播放、字幕显示和外部存储通路初步完成
>
> 版本基线：外部存储可用，RS485 DMA 接收与板级收发通路已验证

本文用于快速了解工程当前已经具备的能力、板上验证结果和后续边界。历史分析、内存估算和设计过程见 `.agents/memory.md`。

## 1. 工程基础

| 项目 | 当前配置 |
| --- | --- |
| MCU | AT32F435VGT7，Cortex-M4，144 MHz |
| RTOS | FreeRTOS V10.4.3，`heap_4.c`，堆 64 KiB |
| 工具链 | Keil MDK 5，Arm Compiler 6.22 |
| 音频输出 | I2S/EDMA → CS4344 → CAT5171 → TPA3116 → 喇叭 |
| 语音合成 | UART → VTX316 → CAT5171 → TPA3116 → 喇叭 |
| 文件系统 | SDIO + FatFs；QSPI Flash + LittleFS |
| 当前构建结果 | `0 Error(s), 0 Warning(s)` |

工程分层保持为：

```text
01_App -> 02_Service -> 03_Platform/bsp
                              |-> 03_Platform/mcu_interface -> 04_Impl
                              `-> 06_Component/ExternalChip
```

- App 持有 FreeRTOS 任务、队列、通知和触发编排。
- Service 不依赖 RTOS，处理通用状态和业务规则。
- BSP 负责板级资源与具体器件驱动的粘合。
- ExternalChip 驱动保留芯片协议和可移植 context。

## 2. 已实现和验证的基础能力

### GPIO 与按键

- GPIO 输入输出资源已加入逻辑资源表。
- `button.c` 已用于按键消抖和按下事件检测。
- 资源表中物理丝印映射如下：

| 板上丝印 | GPIO 资源 | 当前测试功能 |
| --- | --- | --- |
| SW2 | `BOARD_GPIO_KEY1` | 触发默认 MP3 播放 |
| SW3 | `BOARD_GPIO_KEY2` | 音量减 5%，成功后 VTX316 播报“音量减小” |
| SW4 | `BOARD_GPIO_KEY3` | 音量加 5%，成功后 VTX316 播报“音量增加” |
| SW5 | `BOARD_GPIO_KEY4` | 触发 VTX316 测试文本播放 |

- 按键继续由 `start_or_test_f` 每 10 ms 轮询，不为低频音量操作单独创建任务。
- 紧急喊话接口仍保留，但当前没有按键测试入口。

### SD 卡与 FatFs

- 已验证 SD 卡初始化、扇区读取和 FAT 文件系统挂载。
- 已验证根目录枚举、文件创建、写入、同步、关闭、重新打开和回读校验。
- 当前 SDIO 测试频率可正常读取和写入音频文件。
- 测试代码采用轮询版本；SD 卡基础读写通路可用。

### QSPI Flash 与 LittleFS

- 板载存储芯片按实际丝印确认为 HG25Q128B，容量16 MiB，JEDEC ID为`C2 20 18`。
- QSPI1当前工作频率48 MHz（144 MHz / 3）。
- 普通命令、编程和擦除采用1-1-1；数据读取采用1-1-4，Quad Enable掉电保持已验证。
- 已验证整扇区16页编程、4 MiB重复读取校验和测试扇区擦除恢复。
- 已建立从`plat_qspi`、HG25Q128B驱动到`bsp_storage_flash`的完整阻塞式访问链路。
- LittleFS保持第三方源码不修改，通过独立`lfs_config.h`关闭动态内存、调试输出和断言。
- `storage_filesystem_service`持有LittleFS实例、静态缓存、挂载状态及单个打开文件，不依赖RTOS；BSP只负责裸Flash读、写和擦除。
- 当前外部Flash分区固定为：

```text
0x000000～0xDFFFFF：LittleFS，14 MiB，3584个4 KiB Sector
0xE00000～0xFFFFFF：OTA预留，2 MiB，暂不参与管理
```

- 已板测首次格式化、8 KiB文件写入、卸载重挂载、读取校验和掉电持久化；第二次启动能够直接挂载且`formatted=0`。
- 验证完成后已删除`start_or_test_f`中的QSPI/LittleFS测试代码，系统启动阶段当前不访问外部Flash。
- 参数文件格式、目录、原子替换和OTA业务尚未设计，等待实际需求明确后补充。

### UART DMA 与 RS485

- USART1/RS485 使用 DMA1 Channel2 接收，9600、8N1；PA12 控制收发方向。USART3/VTX316 使用 DMA1 Channel3 接收。
- UART 底层为每个串口持有一块 DMA 环形存储：远程 4G 512 字节、RS485 1024 字节、VTX316 64 字节。kfifo 只维护同一块存储的读写索引，不再从 DMA 缓冲区复制到 BSP 的第二个接收 FIFO。
- `bsp_rs485` 提供初始化、空闲分隔数据的轮询读取及切换方向发送；尚未建立正式 RS485 协议或业务入口。
- 板测发现 RS485 RX 无外部上拉时空闲电平不稳定，会产生额外乱码；工程配置已为 USART1 RX/TX 设置内部上拉。VTX316 播放在 UART DMA 改动后仍正常。
- 使用 COM14 测 RS485 回显、COM11 观察板上日志：1～1024 字节共 21 组边界长度均逐字节一致，包含 511/512/513、1023/1024；125 字节另重复 40 次，均无多余回传。
- 连续发送 1025 字节两次，均报 `ret=2, received=0` 且无回显；每次之后的 37 字节及一次 1024 字节仍可正常回显。验证后已删除临时回显 Service 和 `start_or_test_f` 测试入口。
- 当前溢出处理会丢弃受损缓存并报错，但 DMA 继续接收；底层尚不具备协议帧边界，不能保证把任意超长业务消息作为整体拒绝。正式接入协议时需定义帧长、校验及溢出后的丢弃/重新同步规则。

### LED

- 已建立板级 LED BSP 和指示 App。
- `LED1～LED4` 为低电平点亮逻辑的反向配置，当前默认熄灭。
- `LED4_RUN` 当前按 1 秒周期闪烁。
- `LED_OUT_R/B` 为高电平点亮；VTX316语音指示开始时红灯先亮，之后红/蓝每5秒交替，HUB字幕完整滚完时与屏幕同时熄灭。停止、异常和紧急打断会立即熄灭。
- `voice_presentation_app` 负责统一编排VTX字幕与红蓝灯；`led_indicator_app` 只管理LED，`hub_display_app` 只管理显示，二者不再直接依赖。
- `LED_POWER_CS` 属于其他器件电源控制，不纳入普通指示灯逻辑。

### HUB显示与GT20L16S1Y字库

- 已复现厂家HUB12单色屏1/4扫描显示与纵向滚动效果。
- HUB扫描由RTOS软件定时器周期驱动，字幕滚动与红蓝LED生命周期由`voice_presentation_app`统一编排。
- 已接入GT20L16S1Y字库芯片，SPI读出的多个GB2312 16×16字模均通过重复校验。
- VTX316测试文本可流式读取字模并滚动显示，不再要求一次性缓存全部字幕。
- 当前仅完成厂家屏功能和语音字幕联动；HUB08/HUB75等多屏类型尚未实现。

## 3. MP3 播放通路

当前链路：

```text
SD/FatFs
   -> audio_data_source_t
   -> minimp3
   -> PCM 周转块
   -> I2S/EDMA 双缓冲
   -> CS4344
   -> CAT5171
   -> TPA3116
   -> 喇叭
```

已实现：

- SD 卡 MP3 数据源与统一 `audio_data_source_t` 接口。
- minimp3 解码服务和单实例输入缓存。
- PCM 块池、播放器任务和 EDMA 双缓冲输出。
- 从 MP3 帧信息获取采样率并动态配置 I2S。
- 当前覆盖 8/11.025/12/16/22.05/24/32/44.1/48 kHz。
- 板上已验证完整 MP3 播放，观察到 `underrun=0`。
- CS4344 上电后发送 500 ms 静音 PCM 预热，解决首次冷启动吞掉开头的问题。

当前 MP3、VTX316 和紧急喊话共用同一物理输出通路。普通播放忙时继续采用“拒绝新请求、不排队、不打断”的策略。

## 4. VTX316 语音合成

- 已完成 UART 命令帧发送和 GBK 文本播放。
- 已处理命令接收成功、失败、播音中和空闲回传。
- 已结合 R/B 忙闲状态形成基本闭环。
- BSP 对上使用 `bsp_voice_synthesis_*` 通用能力命名，VTX316 私有类型未穿透到 App。
- 当前可由 SW5 触发测试文本，也可用于音量加减后的语音提示。
- 播放器忙时，音量操作仍然生效，但对应 VTX316 提示可能被拒绝。

## 5. TPA3116 功放控制

- 已完成 SDZ、MUTE 和 FAULTZ GPIO 控制。
- GPIO 初始状态为 `SDZ=低、MUTE=高`，保证 MCU 上电阶段处于关闭和静音状态。
- 播放服务初始化后使能 TPA3116，但保持静音。
- MP3 或 VTX316 正式播放前解除静音。
- 播放完成、停止、命令拒绝或异常退出后恢复静音。
- 进入紧急喊话状态时保持功放开声，退出紧急状态后恢复静音。

该策略用于降低板子上电和停止音频链路时的爆破音，仍需继续进行板上听感验证。

## 6. CAT5171 音量控制

- I2C2 当前为 100 kHz，CAT5171 基础写入和回读已板测通过。
- 当前 CAT5171 安全测试区间：

```text
BSP_AUDIO_VOLUME_POSITION_MIN = 5
BSP_AUDIO_VOLUME_POSITION_MAX = 245
```

- BSP 对上提供 `0～100%` 音量接口，内部线性映射到 D 值：

```text
D = D_MIN + round(volume * (D_MAX - D_MIN) / 100)
```

- `0%` 只表示当前允许 D 区间的最小位置，不等于硬件静音；真正静音由 TPA3116 MUTE 控制。
- 原始 position 接口保留用于板级诊断。
- `audio_volume_service` 不依赖 RTOS，负责：
  - 保存当前逻辑音量；
  - 加减和 `0～100%` 限幅；
  - 调用 BSP 写入；
  - 回读并验证结果；
  - I2C 异常后标记为需要重新初始化。
- 阻塞式 I2C 当前仍在 `start_or_test_f` 上下文执行，不进入实时播放器任务，因此不会阻塞 MP3 DMA 补数。

当前映射只要求“音量有可用变化”。人耳响度曲线、非线性查表和最终安全范围待后续听感测试后决定。

## 7. 当前播放架构

```text
外部触发/按键
       |
       v
App 请求接口和播放器命令队列
       |
       v
唯一播放器任务
       |
       v
Audio Player Service
   |-- MP3 -> 解码 -> PCM -> I2S/EDMA -> CS4344
   `-- Voice Synthesis -> BSP -> VTX316

音量按键
   -> start_or_test_f
   -> Audio Volume Service
   -> BSP -> CAT5171
   -> 成功后向播放器提交 VTX316 提示
```

- 播放器任务是 MP3、VTX316 和紧急喊话状态的唯一管理者。
- 音量控制可以在播放期间改变模拟链路增益，但阻塞 I2C 不进入播放器任务。
- 后续如果 4G、RS485、EEPROM 等产生多个低速控制来源，再考虑建立通用控制任务或共享 I2C 串行化机制，不为单个低频器件提前创建任务。

## 8. 当前资源与内存关注点

- FreeRTOS heap 当前配置为 64 KiB。
- 音频任务、队列、PCM 块池和 DMA 缓冲区主要采用静态分配。
- 当前音频静态大缓冲约 39.5 KiB，详细构成见 `.agents/memory.md`。
- 后续通信、ADC、USB、QSPI 等功能接入后，需要重新检查：
  - Keil RW/ZI 使用量；
  - 各任务栈高水位；
  - FreeRTOS heap 历史最低余量；
  - MP3 播放 `underrun` 计数。

## 9. 尚未完成

- EEPROM 驱动及其与 CAT5171 共用 I2C2 的并发策略。
- ADC 紧急喊话检测和正式硬件触发。
- 4G、RS485 正式协议与事件触发接入。
- QSPI LittleFS和USB音频数据源适配。
- OTA镜像下载、校验和升级流程；当前仅预留2 MiB物理区域。
- LittleFS参数文件格式、原子更新及正式挂载生命周期。
- 正式的播放优先级、待播放队列和抢占策略。
- VTX316 播放完成超时保护。
- 音量默认值持久化、最终安全 D 区间和听感曲线。
- 将 `start_or_test_f` 中的测试触发迁移为正式产品输入 App。

## 10. 当前板上验收重点

1. 上电时 TPA3116 保持静音，无明显爆破音。
2. SW2 首次和后续播放 MP3 均不吞开头，播放结束后恢复静音。
3. SW5 可正常播放 VTX316 文本，结束后恢复静音。
4. SW3/SW4 以 5% 步进调节音量，回读值一致。
5. 空闲时音量调整后可以听到对应提示；播放忙时允许提示被拒绝。
6. MP3 播放期间调整音量不产生 `underrun` 或明显断音。
7. 持续记录任务栈高水位、功放 FAULT 和 I2C 异常日志。
