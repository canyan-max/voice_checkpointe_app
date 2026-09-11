# 项目记忆

## 2026-09-01：MP3 播放链路与 SRAM 约束

### 已验证状态

- 当前只实现一路 MP3 播放，同一时刻只允许一个解码和播放会话。
- 板上已验证 SD 卡 MP3 可经 minimp3 解码并通过 I2S/EDMA、CS4344 和 TPA3116 播放。
- 已验证 44.1 kHz 播放完成，日志为 `underrun=0`。
- 首次上电可能丢失语音开头；TPA3116 从系统初始化起保持使能且解除静音后，仍表现为只在首次冷启动播放丢开头，后续播放及等待 30 秒后播放均正常，初步指向 CS4344 冷启动。2026-09-10 新增单次诊断：播放器初始化时以 44.1 kHz 循环发送全零 PCM 500 ms 后停止 I2S；KEY1 正式播放仍无额外延迟，用于确认 CS4344 需要先送时钟和静音数据完成上电建立。
- 当前全局只有一个 `mp3_decoder_service_t` 实例：`audio_app_decoder_service`。

### 数据链路和缓冲区职责

```text
SD / QSPI / USB 等数据源
        |
        | audio_data_source_t::read()
        v
mp3_decoder_service_t::input_buffer[8192]
        | MP3 压缩字节，仅由解码服务管理
        v
minimp3
        |
        v
audio_app_pcm_blocks[4]
        | 解码任务与播放任务之间的 PCM 周转块池
        v
audio_player_service_t::dma_samples[]
        | 2 x 1152 帧立体声 PCM，约 9 KiB
        v
EDMA -> I2S -> CS4344 -> 模拟链路 -> TPA3116 -> 喇叭
```

- 外部数据源不得直接写 `input_buffer`，只需实现统一的 `open/read/seek/close` 接口。
- SD、QSPI、USB 切换时复用同一个解码器、输入缓存、PCM 块池和 EDMA 缓冲区，不为每种来源复制一整套音频缓存。
- minimp3 每次向一个空闲 PCM 块解码；完成后送入 ready queue，播放任务复制到当前可写的 EDMA 半区，随后立即归还 PCM 块。
- `audio_app_startup_silence_block` 当前只用于极短 MP3 在 EOF 时不足两个启动块的情况，为 EDMA 第二半区提供静音占位。
- 正式播放采样率来自 minimp3 的 `info.hz`，不是固定 44.1 kHz；MCU I2S 适配层已覆盖 MP3 标准采样率 8/11.025/12/16/22.05/24/32/44.1/48 kHz。开机预热使用的 44.1 kHz 只是一组静音启动时钟。

### 当前音频静态缓冲估算

| 项目 | 约占用 |
| --- | ---: |
| MP3 输入缓存 | 8 KiB |
| 4 个 PCM 周转块 | 18 KiB |
| 独立启动静音块 | 4.5 KiB |
| EDMA 双缓冲 | 9 KiB |
| 合计（不含任务栈和 minimp3 状态） | 约 39.5 KiB |

一个 PCM 块最多保存 1152 帧、双声道、16 bit 数据，约 4608 字节。在 44.1 kHz 下对应约 26 ms 音频；EDMA 双缓冲本身对应约 52 ms 音频。

### 当前工程 SRAM 基线

2026-09-02 将 FreeRTOS heap 从 128 KiB 调整为 64 KiB 后，Keil 编译结果：

```text
RW-data =   1,232 B
ZI-data = 148,360 B
```

- RW + ZI 约 146.09 KiB，SRAM 总量为 384 KiB，链接层面尚余 243,624 B（约 237.91 KiB）。
- FreeRTOS 的 64 KiB heap 已包含在 ZI 中；这是预留区域，不代表运行时已经全部使用。
- 当前音频任务使用静态任务栈、静态队列和静态音频缓存，不会再从 FreeRTOS heap 重复分配这些空间。
- 上述数值只是当前基线；每次加入通信、USB、QSPI 等模块后，应以新的 Keil map/size 结果为准。

### 暂缓执行的优化方案

当前播放稳定，暂不修改。后续 SRAM 紧张时按以下顺序评估：

1. 删除独立的 `audio_app_startup_silence_block`，第二启动块不存在时直接清零 EDMA 第二半区，可省约 4.5 KiB。
2. 将 PCM 周转块由 4 个降为 3 个并做完整播放和并发压力测试；仍为 `underrun=0` 后再考虑降为 2 个。
3. 任务栈只能依据 `uxTaskGetStackHighWaterMark()` 实测结果调整。
4. FreeRTOS heap 大小只能依据 `xPortGetMinimumEverFreeHeapSize()` 和后续模块需求调整。
5. `input_buffer` 的 8 KiB 暂时保留，避免在没有多格式、坏帧和跨读取边界验证前缩小 MP3 解码窗口。

不建议让 minimp3 直接写 EDMA 半区。这样虽然能删除 PCM 周转层，但会把解码、存储读取和 DMA 实时时序紧耦合，SD 或任务调度抖动会直接造成断音。

### 后续验收条件

- 完整播放不同码率、不同长度的 MP3，输出采样率正确。
- 播放结束日志保持 `underrun=0`。
- 同时运行后续通信任务时无断音、无 EDMA 错误。
- 记录所有任务的最小栈余量和 FreeRTOS heap 历史最低余量。
- 产品收敛阶段为未预见功能保留合理 SRAM 余量，目标不少于总 SRAM 的约 20%。

## 2026-09-11：基础驱动与统一播放框架阶段总结

### 阶段定位

当前完成的是“基础硬件驱动、两类音频播放能力和可扩展播放框架”，尚未达到完整产品业务状态。后续开发应在现有统一播放入口上补功能，不再为每种音源各自建立不受控的播放任务。

### 已完成和板上验证

- GPIO 输入输出资源映射，以及 KEY1～KEY3 的按键消抖和触发。
- SD/FatFs 挂载、目录读取、文件写入和回读校验。
- TPA3116 使能、静音控制和 FAULT 状态读取。
- CS4344 的 I2S/EDMA 双缓冲输出、1 kHz 正弦波和 MP3 PCM 播放。
- CS4344 上电后先发送 500 ms 静音 PCM 进行预热，已用于解决首次播放吞开头的问题。
- minimp3 解码、采样率动态配置和 SD 卡 MP3 完整播放，已观察到 `underrun=0`。
- VTX316 UART 合成命令发送、GBK 文本播报、R/B 忙闲检测，以及 `0x41/0x45/0x4E/0x4F` 回传处理。

### 当前播放架构与既定策略

```text
外部触发
   |
   v
App 播放请求队列与唯一播放器任务
   |
   v
Audio Player Service（状态与互斥）
   |-- MP3 -> minimp3 -> PCM -> EDMA/I2S -> CS4344
   `-- 语音合成 -> BSP -> VTX316
```

- KEY1 模拟 MP3 请求，KEY2 模拟语音合成请求，KEY3 模拟紧急喊话状态切换。
- MP3 与语音合成共用同一物理输出通路，同一时刻只允许一路工作。
- 当前忙时策略是拒绝并丢弃新请求，不排队补播，也不打断当前普通播放。
- KEY3 第一次按下进入紧急状态并终止当前 MP3 或语音合成；功放保持非静音，供硬件紧急喊话通路使用。再次按下退出紧急状态，被终止内容不自动续播。
- App 持有 FreeRTOS 任务、队列、通知和轮询；Service 不依赖 RTOS，只管理播放器业务状态；BSP 只负责板级粘合。
- BSP 对外按“语音合成”能力命名。`vtx316_*` 协议类型只允许存在于 `bsp_voice_synthesis.c` 内部和 `06_Component/ExternalChip/VTX316`。
- MP3 数据源继续使用 `audio_data_source_t`，后续 SD、QSPI 和 USB 必须复用同一解码器和播放链路。

### 已发现并修复的状态问题

- “请求入队成功”和“实际开始播放”已使用不同日志语义；请求入队返回 OK 不代表硬件已经开始播放。
- 紧急状态或 VTX316 播放时不得无条件通知 MP3 解码任务停止，否则停止通知会残留并终止下一次 MP3，使播放器卡在 PREPARING。现已限定为仅在 PREPARING 或 PLAYING_CS4344 状态发送解码停止通知。
- 播放器任务已拆为命令处理、MP3 事件处理、语音合成处理和紧急处理等私有函数，任务主循环只保留调度骨架。

### 后续待办

1. 下载并组合验证最后一轮 KEY3、残留停止通知修复和语音合成 BSP 重命名。
2. 增加 VTX316 等待完成超时，避免丢失 `0x4F` 后长期占用播放器状态。
3. 实现 CAT5171TBI 音量控制。
4. 接入 ADC 紧急喊话检测，并替换 KEY3 临时触发。
5. 实现 QSPI、USB 音频数据源，以及 4G/RS485 正式业务触发。
6. 产品策略明确后再决定播放请求优先级、排队和抢占规则；当前继续保持忙时拒绝。
7. 后续模块加入后重新评估 PCM 块、任务栈和 FreeRTOS heap，按实测高水位优化 SRAM。

### 当前版本

- 本阶段代码提交：`961b023 feat: integrate voice synthesis playback control`。
- 该提交前完整 Keil 编译结果为 `0 Error(s), 0 Warning(s)`。
