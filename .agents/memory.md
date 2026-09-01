# 项目记忆

## 2026-09-01：MP3 播放链路与 SRAM 约束

### 已验证状态

- 当前只实现一路 MP3 播放，同一时刻只允许一个解码和播放会话。
- 板上已验证 SD 卡 MP3 可经 minimp3 解码并通过 I2S/EDMA、CS4344 和 TPA3116 播放。
- 已验证 44.1 kHz 播放完成，日志为 `underrun=0`。
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
- `audio_app_startup_silence_block` 只用于极短 MP3 在 EOF 时不足两个启动块的情况，为 EDMA 第二半区提供静音占位。

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

最近一次已记录的 Keil 编译结果：

```text
RW-data =   1,232 B
ZI-data = 213,896 B
```

- RW + ZI 约 210 KiB，SRAM 总量为 384 KiB，链接层面尚余约 174 KiB。
- FreeRTOS 的 128 KiB heap 已包含在 ZI 中；这是预留区域，不代表运行时已经全部使用。
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
