# Voice Checkpoint 工程状态

> 更新时间：2026-09-16  
>
> 当前阶段：基础驱动、音频播放和音量调节通路初步完成  
>
> 版本基线：`b0e4beb feat: integrate runtime audio volume control`

本文用于快速了解工程当前已经具备的能力、板上验证结果和后续边界。历史分析、内存估算和设计过程见 `.agents/memory.md`。

## 1. 工程基础

| 项目 | 当前配置 |
| --- | --- |
| MCU | AT32F435VGT7，Cortex-M4，144 MHz |
| RTOS | FreeRTOS V10.4.3，`heap_4.c`，堆 64 KiB |
| 工具链 | Keil MDK 5，Arm Compiler 6.22 |
| 音频输出 | I2S/EDMA → CS4344 → CAT5171 → TPA3116 → 喇叭 |
| 语音合成 | UART → VTX316 → CAT5171 → TPA3116 → 喇叭 |
| 文件系统 | SDIO + FatFs |
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

### LED

- 已建立板级 LED BSP 和指示 App。
- `LED1～LED4` 为低电平点亮逻辑的反向配置，当前默认熄灭。
- `LED4_RUN` 当前按 1 秒周期闪烁。
- `LED_OUT_R/B` 为高电平点亮，已用于语音合成播放指示测试。
- `LED_POWER_CS` 属于其他器件电源控制，不纳入普通指示灯逻辑。

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
- 4G、RS485 等协议触发接入。
- QSPI、USB 音频数据源。
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
